/*
 * Copyright (C) 2023-2024 Antmicro
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <newlib.h>
#include <sys/stat.h>
#include <sys/timeb.h>
#include <sys/times.h>
#include <unistd.h>
#include <utime.h>

#include "stdio.h"

#ifdef DEV_LITEUART
#include "dev/liteuart.h"
#endif
#ifdef DEV_SIMPLEUART
#include "dev/simpleuart.h"
#endif
#ifndef DEV_UART
#error No UART headers found for selected devices
#endif

#undef errno
extern int errno;

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

/* It turns out that older newlib versions use different symbol names which goes
 * against newlib recommendations. Anyway this is fixed in later version.
 */
#if __NEWLIB__ <= 2 && __NEWLIB_MINOR__ <= 5
#define _sbrk sbrk
#define _write write
#define _close close
#define _lseek lseek
#define _read read
#define _fstat fstat
#define _isatty isatty
#endif

#define SYSCALL_UNIMPL  do { \
  errno = ENOSYS; \
  return -1; \
} while (0)

static inline int is_io_file(int file) {
  return (file == STDIN_FILENO) || (file == STDOUT_FILENO)
          || (file == STDERR_FILENO);
}

extern char __heap_start;
extern char __heap_end;
static char* const heap_start = &__heap_start;
static char* const heap_end   = &__heap_end;
static char* brk              = heap_start;

ssize_t _write(int file, const void* ptr, size_t len);

void unimplemented_syscall(void) {
  static char msg[] = "[newlib]: WARNING - unimplemented syscall.\n";
  _write(STDERR_FILENO, msg, sizeof(msg));
}

int nanosleep(const struct timespec* rqtp, struct timespec* rmtp) {
  SYSCALL_UNIMPL;
}

int _access(const char* file, int mode) {
  SYSCALL_UNIMPL;
}

int _brk(void* addr) {
  brk = addr;
  return 0;
}

int _chdir(const char* path) { SYSCALL_UNIMPL; }

int _chmod(const char* path, mode_t mode) { SYSCALL_UNIMPL; }

int _chown(const char* path, uid_t owner, gid_t group) {
  SYSCALL_UNIMPL;
}

int _close(int file) {
  if (is_io_file(file)) {
    return 0;
  }
  errno = ENOENT;
  return -1;
}

int _execve(const char* name, char* const argv[], char* const env[]) {
  SYSCALL_UNIMPL;
}

void _exit(int exit_status) {
  asm volatile("wfi");
  while (1);
}

int _faccessat(int dirfd, const char* file, int mode, int flags) {
  SYSCALL_UNIMPL;
}

int _fork(void) { SYSCALL_UNIMPL; }

int _fstat(int file, struct stat* st) {
  if (is_io_file(file)) {
    st->st_mode = S_IFCHR;
    return 0;
  }
  errno = ENOENT;
  return -1;
}

int _fstatat(int dirfd, const char* file, struct stat* st, int flags) {
  SYSCALL_UNIMPL;
}

int _ftime(struct timeb* tp) { SYSCALL_UNIMPL; }

char* _getcwd(char* buf, size_t size) {
  errno = -ENOSYS;
  return NULL;
}

int _getpid(void) { return 1; }

int _gettimeofday(struct timeval* tp, void* tzp) {
  errno = -ENOSYS;
  return -1;
}

int _isatty(int file) { return (file == STDOUT_FILENO); }

int _kill(int pid, int sig) { SYSCALL_UNIMPL; }

int _link(const char* old_name, const char* new_name) { SYSCALL_UNIMPL; }

off_t _lseek(int file, off_t ptr, int dir) { SYSCALL_UNIMPL; }

int _lstat(const char* file, struct stat* st) { SYSCALL_UNIMPL; }

int _open(const char* name, int flags, int mode) {
  errno = ENOENT;
  return -1;
}

int _openat(int dirfd, const char* name, int flags, int mode) {
  SYSCALL_UNIMPL;
}

static unsigned char stdin_eof = 0;

ssize_t _read(int file, void* ptr, size_t len) {
  if (file != STDIN_FILENO) {
    errno = ENOENT;
    return -1;
  }

  size_t cnt;
  for (cnt = 0; cnt < len; ++cnt) {
    if (stdin_eof) {
      stdin_eof = 0;
      return cnt;
    }
    char c = (char)uart_getc();
    uart_putc(c);
    if (c == '\r') {
      uart_putc('\n');
      stdin_eof = 1;
    }
    *(char*)ptr++ = c;
  }
  return cnt;
}

void* _sbrk(ptrdiff_t incr) {
  char* old_brk = brk;

  if (heap_start == heap_end) {
    errno = ENOMEM;
    return NULL;
  }

  if ((brk + incr) <= heap_end) {
    brk += incr;
  } else {
    errno = ENOMEM;
    brk = heap_end;
  }

  return old_brk;
}

int _stat(const char* file, struct stat* st) {
  errno = ENOENT;
  return -1;
}

long _sysconf(int name) { SYSCALL_UNIMPL; }

clock_t _times(struct tms* buf) { SYSCALL_UNIMPL; }

int _unlink(const char* name) {
  errno = ENOENT;
  return -1;
}

int _utime(const char* path, const struct utimbuf* times) { SYSCALL_UNIMPL; }

int _wait(int* status) { SYSCALL_UNIMPL; }

ssize_t _write(int file, const void* ptr, size_t len) {
  if ((file != STDOUT_FILENO) && (file != STDERR_FILENO)) {
    errno = ENOENT;
    return -1;
  }

  const void* eptr = ptr + len;
  while (ptr != eptr) {
    if (*(char*)ptr == '\n') {
      uart_putc('\r');
    }
    uart_putc(*(char*)(ptr++));
  }
  return len;
}
