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

#include "cpu/interrupts.h"
#include "cpu/riscv_csr.h"

#define U54_MC_PLIC_PRIORITY   0x0C000000
#define U54_MC_PLIC_PENDING    0x0C001000
#define U54_MC_PLIC_ENABLE     0x0C002000
#define U54_MC_PLIC_THRESHOLD  0x0C200000
#define U54_MC_PLIC_CLAIM      0x0C200004

#define U54_MC_PLIC_BANK_COUNT 15

static volatile uint32_t *u54mc_plic_priority =
  (volatile uint32_t *)U54_MC_PLIC_PRIORITY;
static volatile uint32_t *u54mc_plic_enable =
  (volatile uint32_t *)U54_MC_PLIC_ENABLE;
static volatile uint32_t *u54cm_plic_threshold =
  (volatile uint32_t *)U54_MC_PLIC_THRESHOLD;
static volatile uint32_t *u54mc_plic_claim =
  (volatile uint32_t *)U54_MC_PLIC_CLAIM;

void interrupt_init_external(void) {
  for (size_t bank = 0; bank < U54_MC_PLIC_BANK_COUNT; ++bank)
    u54mc_plic_enable[bank] = 0;
  *u54cm_plic_threshold = 0;
}

void interrupt_enable_external(uint32_t irq) {
  size_t bank = 0;
  while (irq >= 32) {
    ++bank;
    irq -= 32;
  }
  u54mc_plic_enable[bank] |= (uint32_t)1 << irq;

  /* Priority `0` means that the interrupt is effectively disabled. */
  if ((u54mc_plic_priority[irq] & 0x3) == 0)
    interrupt_set_priority(irq, 1);
}

void interrupt_disable_external(uint32_t irq) {
  size_t bank = 0;
  while (irq >= 32) {
    ++bank;
    irq -= 32;
  }
  u54mc_plic_enable[bank] &= ~((uint32_t)1 << irq);
}

void _isr_internal(void) {
  uint32_t irq = *u54mc_plic_claim;
  isr(irq);
  *u54mc_plic_claim = irq;
}

uint32_t interrupt_priority_count(void) {
  return 7;
}

void interrupt_set_priority(uint32_t irq, uint32_t prio) {
  const uint32_t mask = 0x3;

  uint32_t reg = u54mc_plic_priority[irq];
  reg = (reg & ~mask) | ((prio + 1) & mask);
  u54mc_plic_priority[irq] = reg;
}
