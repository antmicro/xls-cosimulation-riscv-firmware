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

.global main
.global _isr_internal

.global _start
.section .start
.align 3
_start:
    j crt_init
    .size  _start, .-_start


.global trap_entry
.section .start
.align 3
trap_entry:
    sw x1,  - 1*4(sp)
    sw x5,  - 2*4(sp)
    sw x6,  - 3*4(sp)
    sw x7,  - 4*4(sp)
    sw x10, - 5*4(sp)
    sw x11, - 6*4(sp)
    sw x12, - 7*4(sp)
    sw x13, - 8*4(sp)
    sw x14, - 9*4(sp)
    sw x15, -10*4(sp)
    sw x16, -11*4(sp)
    sw x17, -12*4(sp)
    sw x28, -13*4(sp)
    sw x29, -14*4(sp)
    sw x30, -15*4(sp)
    sw x31, -16*4(sp)
    addi sp,sp,-16*4
    csrr x31, mcause
    // if (mcause[31] == 0) handle_exception
    // else call _isr_internal
    srli x30, x31, 31
    beq x30,zero,handle_exception
    call _isr_internal
    lw x1 , 15*4(sp)
    lw x5,  14*4(sp)
    lw x6,  13*4(sp)
    lw x7,  12*4(sp)
    lw x10, 11*4(sp)
    lw x11, 10*4(sp)
    lw x12,  9*4(sp)
    lw x13,  8*4(sp)
    lw x14,  7*4(sp)
    lw x15,  6*4(sp)
    lw x16,  5*4(sp)
    lw x17,  4*4(sp)
    lw x28,  3*4(sp)
    lw x29,  2*4(sp)
    lw x30,  1*4(sp)
    lw x31,  0*4(sp)
    addi sp,sp,16*4
    mret
handle_exception:
    // handle exceptions by saving mcause and mbadaddr and rebooting
    csrr x31, mcause
    csrr x29, mbadaddr
    j _start

.section .data
.global _init_mcause
.global _init_mbadaddr
.align 3
_init_mcause:
    .word 0
_init_mbadaddr:
    .word 0

.global  _crt_init
.section .text
crt_init:
    // Zero ra so that backtrace is not surprised
    li ra, 0

    // Set stack and trap address
    la sp, _fstack + 4
    la a0, trap_entry
    //ori a0, a0, 1 // Uncomment to use vectored mode
    csrw mtvec, a0

    // Below code taken from:
    // https://github.com/bminor/newlib/blob/master/libgloss/riscv/crt0.S

    // Initialize global pointer
    .option push
    .option norelax
    1:auipc gp, %pcrel_hi(__global_pointer$)
    addi  gp, gp, %pcrel_lo(1b)
    .option pop
    // Clear the bss segment
    la      a0, __bss_start
    la      a2, _end
    sub     a2, a2, a0
    li      a1, 0
    call    memset

    // Move saved mcause and mbadaddr into symbol in .data
    sw x31, _init_mcause, a0
    sw x29, _init_mbadaddr, a0

    // Run global initialization functions
    call    __libc_init_array
    call    main
infinit_loop:
    wfi
    j infinit_loop
