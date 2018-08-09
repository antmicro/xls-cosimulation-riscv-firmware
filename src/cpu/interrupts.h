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

#ifndef CPU_INTERRUPTS_H_
#define CPU_INTERRUPTS_H_

#include <stddef.h>
#include <stdint.h>
#include "sys/_stdint.h"

void interrupt_init_external(void);
void interrupt_enable_external(uint32_t irq);
void interrupt_disable_external(uint32_t irq);

uint32_t interrupt_priority_count(void);
void interrupt_set_priority(uint32_t irq, uint32_t prio);

void isr(uint32_t irq);

#endif /* CPU_INTERRUPTS_H_ */
