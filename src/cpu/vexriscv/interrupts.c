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

#include <stddef.h>
#include <stdio.h>

#include "cpu/riscv_csr.h"
#include "stdio.h"

// VexRiscV simple built-in interrupt controller
//
// Machine IRQs are wired to Renode GPIOs 0~31
// Supervisor IRQs are wired to Renode GPIOs 1000~1031
#define VEXRISCV_INTC_CSR_MMASK (0xBC0)  // Machine IRQ mask
#define VEXRISCV_INTC_CSR_MPEND (0xFC0)  // Machine IRQ pending
#define VEXRISCV_INTC_CSR_SMASK (0x9C0)  // Supervisor IRQ mask
#define VEXRISCV_INTC_CSR_SPEND (0xDC0)  // Supervisor IRQ pending

void interrupt_init_external(void) {
  rv32_csr_write(VEXRISCV_INTC_CSR_MMASK, 0);
}

void interrupt_enable_external(uint32_t irq) {
  uint32_t mask = rv32_csr_read(VEXRISCV_INTC_CSR_MMASK);
  mask |= (uint32_t)1 << irq;
  rv32_csr_write(VEXRISCV_INTC_CSR_MMASK, mask);
}

void interrupt_disable_external(uint32_t irq) {
  uint32_t mask = rv32_csr_read(VEXRISCV_INTC_CSR_MMASK);
  mask &=  ~((uint32_t)1 << irq);
  rv32_csr_write(VEXRISCV_INTC_CSR_MMASK, mask);
}

void isr(uint32_t irq);

void _isr_internal(void) {
  uint32_t mask = rv32_csr_read(VEXRISCV_INTC_CSR_MPEND);
  for(uint32_t irq = 0; irq < 32; ++irq) {
    if (((uint32_t)1 << irq) & mask) {
      isr(irq);
    }
  }
}

uint32_t interrupt_priority_count(void) {
  return 0;
}

void interrupt_set_priority(uint32_t, uint32_t prio) {
  printf("ERROR: %s - UNSUPPORTED", __func__);
}
