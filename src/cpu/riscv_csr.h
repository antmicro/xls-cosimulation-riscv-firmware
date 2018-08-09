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

#ifndef CPU_RISCV_CSR_H_
#define CPU_RISCV_CSR_H_

#include <stdint.h>

extern unsigned int _init_mcause;
extern unsigned int _init_mbadaddr;

// Privileged specification 1.11
// https://courses.cs.washington.edu/courses/cse481a/20sp/readings/riscv-priv.pdf
#define CSR_MVENDORID (0xF11)
#define CSR_MARCHID (0xF12)
#define CSR_MIMPID (0xF13)
#define CSR_MHARTID (0xF14)
#define CSR_MSTATUS (0x300)
#define CSR_MISA (0x301)
#define CSR_MEDELEG (0x302)
#define CSR_MIDELEG (0x303)
#define CSR_MIE (0x304)
#define CSR_MTVEC (0x305)
#define CSR_MCOUNTEREN (0x306)
#define CSR_MSCRATCH (0x340)
#define CSR_MEPC (0x341)
#define CSR_MCAUSE (0x342)
#define CSR_MTVAL (0x343)
#define CSR_MIP (0x344)
#define CSR_MCYCLE (0xB00)
#define CSR_MINSTRET (0xB02)

static inline uint32_t rv32_csr_read(uint32_t csr_num) {
  int result;
  asm volatile("csrr %0, %1" : "=r"(result) : "i"(csr_num) : "memory");
  return result;
}

static inline void rv32_csr_write(uint32_t csr_num, uint32_t value) {
  asm volatile("csrw %0, %1" ::"i"(csr_num), "r"(value) : "memory");
}

#endif /* CPU_RISCV_CSR_H_ */
