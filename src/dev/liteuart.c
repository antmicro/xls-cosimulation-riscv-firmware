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

#include "liteuart.h"

#define UART_EV_TX 0x1
#define UART_EV_RX 0x2
#define UART_BASE 0xe0001800
#define CSR_UART_RXTX_ADDR (UART_BASE + 0x00)
#define CSR_UART_TXFULL_ADDR (UART_BASE + 0x04)
#define CSR_UART_RXEMPTY_ADDR (UART_BASE + 0x08)
#define CSR_UART_EV_PENDING_ADDR (UART_BASE + 0x10)
#define CSR_UART_EV_ENABLE_ADDR (UART_BASE + 0x14)

void uart_putc(unsigned char c) {
  unsigned char r;
  /* pending */
  r = *(volatile unsigned int*)CSR_UART_EV_PENDING_ADDR;
  *(volatile unsigned int*)CSR_UART_EV_PENDING_ADDR = r;

  /* enable */
  *(volatile unsigned int*)CSR_UART_EV_ENABLE_ADDR = UART_EV_TX;

  /* wait for space */
  do {
    r = *(volatile unsigned int*)CSR_UART_TXFULL_ADDR;
  } while (r);
  /* write */
  *(volatile unsigned int*)CSR_UART_RXTX_ADDR = c;
}

unsigned char uart_getc(void) {
  unsigned char r;
  /* pending */
  r = *(volatile unsigned int*)CSR_UART_EV_PENDING_ADDR;
  *(volatile unsigned int*)CSR_UART_EV_PENDING_ADDR = r;

  /* enable */
  *(volatile unsigned int*)CSR_UART_EV_ENABLE_ADDR = UART_EV_RX;

  /* wait for input */
  while (*(volatile unsigned int*)CSR_UART_RXEMPTY_ADDR)
    ;
  r = *(volatile unsigned int*)CSR_UART_RXTX_ADDR;
  return r;
}
