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

#include "xls_dma.h"

#include <stdint.h>

#include "stdio.h"

static inline xls_dma_chan_t* get_tsfr_chan(xls_dma_tsfr_t* tsfr) {
  return &tsfr->tsfr_dma->dma_chans[tsfr->tsfr_chan];
}

static inline const xls_dma_chan_t* get_tsfr_chan_const(
    const xls_dma_tsfr_t* tsfr) {
  return &tsfr->tsfr_dma->dma_chans[tsfr->tsfr_chan];
}

const char* xls_dma_err_name(int code) {
  switch (code) {
    case XLS_DMA_UNIMPLEMENTED:
      return "UNIMPLEMENTED";
    case XLS_DMA_START_WRONGDIR:
      return "START_WRONGDIR";
    case XLS_DMA_START_NOT_RDY:
      return "START_NOT_RDY";
    case XLS_DMA_TIMEOUT:
      return "TIMEOUT";
    case XLS_DMA_NOMAN:
      return "NOMAN";
    case XLS_DMA_OK:
      return "OK";
    default:
      return "[UNKNOWN]";
  }
}

void xls_dma_poll_ready(const xls_dma_tsfr_t* tsfr) {
  const xls_dma_chan_t* dma_chan = get_tsfr_chan_const(tsfr);
  while (!(dma_chan->dmach_ctrl & XLS_DMACH_CTRL_RDY))
    ;
}

int xls_dma_begin_transfer(xls_dma_tsfr_t* tsfr) {
  xls_dma_chan_t* dma_chan = get_tsfr_chan(tsfr);

  dma_chan->dmach_ctrl = 0;

  int valid_dir = dma_chan->dmach_ctrl & XLS_DMACH_CTRL_DIR
                      ? (tsfr->tsfr_dir == XLS_TSFR_FROM_PERIPHERAL)
                      : (tsfr->tsfr_dir == XLS_TSFR_TO_PERIPHERAL);
  if (!valid_dir) return XLS_DMA_START_WRONGDIR;

  if (!(dma_chan->dmach_ctrl & XLS_DMACH_CTRL_RDY)) {
    return XLS_DMA_START_NOT_RDY;
  }

  if (tsfr->tsfr_polling) {
    dma_chan->dmach_irqs |= -1;
    tsfr->tsfr_dma->dma_irq_mask &= ~(1 << tsfr->tsfr_chan);
  } else {
    if (!tsfr->tsfr_dma_man) {
      return XLS_DMA_NOMAN;
    }
    tsfr->tsfr_dma_man->dman_chan_data[tsfr->tsfr_chan].dmanch_tsfr = tsfr;
    tsfr->tsfr_dma_man->dman_complete &= ~(1 << tsfr->tsfr_chan);
    tsfr->tsfr_dma_man->dman_tlast &= ~(1 << tsfr->tsfr_chan);
    tsfr->tsfr_dma->dma_irq_mask |= 1 << tsfr->tsfr_chan;
    dma_chan->dmach_ctrl |= XLS_DMACH_CTRL_IRQMASK_TSFRDONE;
  }

  if (tsfr->tsfr_ignore) {
    dma_chan->dmach_ctrl &= ~XLS_DMACH_CTRL_MODE;
  } else {
    dma_chan->dmach_ctrl |= XLS_DMACH_CTRL_MODE;
    dma_chan->dmach_tsfr_base = (size_t)tsfr->tsfr_data;
  }
  dma_chan->dmach_tsfr_len = tsfr->tsfr_len;

  tsfr->tsfr_done = 0;
  dma_chan->dmach_ctrl |= XLS_DMACH_CTRL_TSFR;

  return XLS_DMA_OK;
}

int xls_dma_complete_transfer(xls_dma_tsfr_t* tsfr, uint64_t timeout) {
  xls_dma_chan_t* chan = get_tsfr_chan(tsfr);

  int done;

  if (tsfr->tsfr_polling) {
    if (timeout == 0) {
      while (!(chan->dmach_ctrl & XLS_DMACH_CTRL_TSFRDONE))
        ;
      tsfr->tsfr_done              = 1;
      tsfr->tsfr_transferred_bytes = chan->dmach_tsfr_donelen;
      if (tsfr->tsfr_callback_isr) {
        tsfr->tsfr_callback_isr(tsfr);
      }
      return XLS_DMA_OK;
    }

    while (timeout-- && !(done = chan->dmach_ctrl & XLS_DMACH_CTRL_TSFRDONE))
      ;
    tsfr->tsfr_transferred_bytes = chan->dmach_tsfr_donelen;
    tsfr->tsfr_done              = done ? 1 : 0;
    if (tsfr->tsfr_callback_isr) {
      tsfr->tsfr_callback_isr(tsfr);
    }
    return done ? XLS_DMA_OK : XLS_DMA_TIMEOUT;
  }

  if (!tsfr->tsfr_dma_man) {
    return XLS_DMA_NOMAN;
  }
  if (timeout == 0) {
    while (!(tsfr->tsfr_done))
      ;
    return XLS_DMA_OK;
  }
  while (timeout-- && !(done = tsfr->tsfr_done))
    ;
  if (!done) {
    return XLS_DMA_TIMEOUT;
  }
  return XLS_DMA_OK;
}

void xls_dma_cancel_transfer(xls_dma_tsfr_t* tsfr) {
  xls_dma_chan_t* chan = get_tsfr_chan(tsfr);
  chan->dmach_ctrl     = 0;
}

void xls_dma_update_isr(xls_dma_t* dma, xls_dma_man_t* dma_man) {
  /* Ideally this should be a reentrant procedure, but for the purpose
   * of the demo, whether it is or not is irrelevant */

  if (!(dma->dma_irqs & (XLS_DMAIRQ_TSFRDONE | XLS_DMAIRQ_TLAST))) {
    return;
  }

  for (int i = 0; i < dma->dma_ch_cnt; ++i) {
    xls_dma_chan_t* chan = &dma->dma_chans[i];
    uint64_t irqs        = chan->dmach_irqs;
    if (irqs & XLS_DMAIRQ_TSFRDONE) {
      dma_man->dman_complete |= (1 << i);
      dma_man->dman_chan_data[i].dmanch_tsfr->tsfr_done = 1;
      dma_man->dman_chan_data[i].dmanch_tsfr->tsfr_transferred_bytes =
          chan->dmach_tsfr_donelen;
    }
    if (irqs & XLS_DMAIRQ_TLAST) {
      dma_man->dman_tlast |= (1 << i);
    }
    if (irqs) {
      xls_dma_tsfr_t* tsfr = dma_man->dman_chan_data[i].dmanch_tsfr;
      chan->dmach_irqs     = 0xff;
      if (tsfr->tsfr_callback_isr) {
        tsfr->tsfr_callback_isr(tsfr);
      }
    }
  }
}
