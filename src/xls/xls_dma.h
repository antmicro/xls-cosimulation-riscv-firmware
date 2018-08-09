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

#ifndef __XLS_DMA_H__
#define __XLS_DMA_H__

#include <stddef.h>
#include <stdint.h>

#include "sys/_types.h"
#include "sys/types.h"

/* NOTE: This file contains structures and methods that are suitable for use
 * ONLY with DMAs up to 64 channels. These will NOT work for DMAs with more
 * channels as the layout of registers for such DMAs differs from what's there.
 */

/* XLS DMA CHANNEL CONTROL REGISTER */

// clang-format off
#define XLS_DMAIRQ_TSFRDONE              0x01
#define XLS_DMAIRQ_TLAST                 0x02

#define XLS_DMACH_CTRL_IRQMASK_TSFRDONE  0x01
#define XLS_DMACH_CTRL_IRQMASK_LAST      0x02
#define XLS_DMACH_CTRL_TSFR              0x04
#define XLS_DMACH_CTRL_TSFRDONE          0x08
#define XLS_DMACH_CTRL_DIR               0x10
#define XLS_DMACH_CTRL_MODE              0x20
#define XLS_DMACH_CTRL_RDY               0x40

#define XLS_DMA_OK                          0
#define XLS_DMA_START_WRONGDIR              1
#define XLS_DMA_START_NOT_RDY               2
#define XLS_DMA_TIMEOUT                     3
#define XLS_DMA_NOMAN                       4
#define XLS_DMA_UNIMPLEMENTED              -1
// clang-format on

#define TOKENCAT(x, y) x##y

typedef struct __attribute__((packed, aligned(8))) xls_dma_chan {
  volatile uint64_t dmach_tsfr_base;
  volatile uint64_t dmach_tsfr_len;
  volatile uint64_t dmach_tsfr_donelen;
  volatile uint32_t dmach_ctrl;
  volatile uint32_t : 32;
  volatile uint32_t dmach_irqs;
  volatile uint32_t : 32;
  volatile uint64_t : 64;
  volatile uint64_t : 64;
  volatile uint64_t : 64;
} xls_dma_chan_t;

typedef struct __attribute__((packed, aligned(8))) xls_dma {
  volatile uint64_t dma_ch_cnt;
  volatile uint64_t dma_ch_first_offset;
  volatile uint64_t dma_irq_mask;
  volatile uint64_t dma_irqs;
  volatile uint64_t : 64;
  volatile uint64_t : 64;
  volatile uint64_t : 64;
  volatile uint64_t : 64;
  xls_dma_chan_t dma_chans[];
} xls_dma_t;

/* DEBUG-ONLY */
static inline int xls_dma_ok(xls_dma_t* dma) {
  return offsetof(xls_dma_t, dma_chans) == dma->dma_ch_first_offset;
}

typedef enum xls_tsfr_dir {
  XLS_TSFR_FROM_PERIPHERAL,
  XLS_TSFR_TO_PERIPHERAL,
} xls_tsf_dir_t;

struct xls_dma_tsfr;

typedef struct xls_dma_man {
  uint64_t dman_complete;
  uint64_t dman_tlast;
  struct {
    struct xls_dma_tsfr* dmanch_tsfr;
  } dman_chan_data[];
} xls_dma_man_t;

typedef void (*xls_dma_tsfr_callback_t)(struct xls_dma_tsfr*);

typedef struct xls_dma_tsfr {
  xls_dma_t* tsfr_dma;       /* DMA to use for the transfer */
  uint64_t tsfr_chan;        /* Index of a DMA channel to use */
  void* tsfr_data;           /* Pointer to source/destination memory */
  uint64_t tsfr_len;         /* Desired number of bytes to transfer*/
  xls_tsf_dir_t tsfr_dir;    /* Transfer direction (decides whether tsfr_data
                              * is a source or destination)*/
  unsigned char tsfr_ignore; /* Ignore the incoming/outcoming
                              * data if set to a non-zero value */
  volatile unsigned char tsfr_done;         /* 1 if the transfer is complete */
  volatile uint64_t tsfr_transferred_bytes; /* Number of bytes transferred
                                             * after completion. */
  unsigned int tsfr_polling;                /* Use polling instead of
                                             * interrupts, if set to a non-zero
                                             * value */
  xls_dma_man_t* tsfr_dma_man;              /* Pointer to a DMA manager,
                                             * required if tsfr_polling == 0 */
  void* tsfr_ctx; /* Context for completion callback */
  xls_dma_tsfr_callback_t
      tsfr_callback_isr; /* Completion callback (called inside of an ISR!) */
} xls_dma_tsfr_t;

typedef enum xls_dma_irq {
  XLS_DMAINT_NONE     = 0x0,
  XLS_DMAINT_TSFRDONE = 0x1,
  XLS_DMAINT_RCVTLAST = 0x2,
} xls_dma_irq_t;

void xls_dma_poll_ready(const xls_dma_tsfr_t* tsfr);
int xls_dma_begin_transfer(xls_dma_tsfr_t* tsfr);
int xls_dma_complete_transfer(xls_dma_tsfr_t* tsfr, uint64_t timeout);
void xls_dma_cancel_transfer(xls_dma_tsfr_t* tsfr);

/* Call this inside of an ISR to handle an interrupt from DMA */
void xls_dma_update_isr(xls_dma_t* dma, xls_dma_man_t* dma_man);

const char* xls_dma_err_name(int code);

#endif /* __XLS_DMA_H__ */
