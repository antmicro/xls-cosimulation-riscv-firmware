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

#ifndef __XLS_STREAM_H__
#define __XLS_STREAM_H__

#include <stdint.h>

/* XLS STREAM CONTROL REGISTER */

#define TOKENCAT(x, y) x##y

typedef struct __attribute__((packed, aligned(8))) xls_stream {
  volatile uint64_t s_ctrl;
} xls_stream_t;

/* Casting `s_data` would result in compiler being unable to determine whether
 * the alignement fulfills restrictions. This will result in access being split
 * into multiple single-byte reads and writes. It works, but it's not how I want
 * to communicate with Renode. */
#define XLS_TYPED_STREAM(type)                       \
  typedef struct __attribute__((packed, aligned(8))) \
  TOKENCAT(xls_stream_, type) {                      \
    xls_stream_t s_stream;                           \
    volatile type s_data;                            \
  } TOKENCAT(xls_stream_, type)

/* RDY (ready) bit mask */
#define XLS_SCTRL_RDY 0x01
/* DOXFER (do transfer) bit mask */
#define XLS_SCTRL_DOXFER 0x02
/* DIR (direction) bit mask */
#define XLS_SCTRL_DIR 0x04
/* ERRXFER (transfer error) bit mask */
#define XLS_SCTRL_ERRXFER 0x08

static inline int xls_is_ready(const xls_stream_t* stream) {
  return stream->s_ctrl & XLS_SCTRL_RDY;
}

static inline void xls_poll_ready(const xls_stream_t* stream) {
  while (!xls_is_ready(stream))
    ;
}

static inline void xls_poll_and_transfer(xls_stream_t* stream) {
  xls_poll_ready(stream);
  stream->s_ctrl |= XLS_SCTRL_DOXFER;
}

#endif /* __XLS_STREAM_H__ */
