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

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "cpu/interrupts.h"
#include "cpu/riscv_csr.h"
#include "dev/rle.h"
#include "xls/xls_dma.h"
#include "xls/xls_stream.h"

typedef void (*on_encoded_t)(void*, rle_enc_out_data_t);

static const size_t RLE_TIMEOUT_CYCLES = 10000;

#ifdef RLE_DMA_IRQ
void isr(uint32_t irq) {
  printf("Interrupt handler, irq: %ld\n", irq);

  if (irq == RLE_DMA_IRQ_NUM) {
    xls_dma_update_isr(rle0_dma, &rle0_dma_man);
  }
}
#else
void isr(uint32_t irq) {}
#endif


#ifndef RLE_DMA

static void run_text_rle(const char* data, void* ctx, on_encoded_t callback) {
  while (*data != '\0') {
    int last;
    do {
      last                              = data[1] == '\0';
      rle0_io.io_input_r->s_data.e_sym  = (rle_sym_t)*data;
      rle0_io.io_input_r->s_data.e_last = last;
      xls_poll_and_transfer(&rle0_io.io_input_r->s_stream);
      ++data;
    } while (!last && xls_is_ready(&rle0_io.io_input_r->s_stream));
    do {
      xls_poll_and_transfer(&rle0_io.io_output_s->s_stream);
      callback(ctx, rle0_io.io_output_s->s_data);
    } while (xls_is_ready(&rle0_io.io_output_s->s_stream));
  }

  /* There might be some data left */
  for (size_t i = 0; i < RLE_TIMEOUT_CYCLES; ++i) {
    if (xls_is_ready((xls_stream_t*)rle0_io.io_output_s)) {
      xls_poll_and_transfer((xls_stream_t*)rle0_io.io_output_s);
      callback(ctx, rle0_io.io_output_s->s_data);
      i = 0;
    }
  }
}

#endif /* RLE_DMA */

#ifdef RLE_DMA

#define DMATSFR_BUF_LEN 256
static rle_enc_in_data_t dma_tsfr_in_buf[DMATSFR_BUF_LEN];
static rle_enc_out_data_t dma_tsfr_out_buf[DMATSFR_BUF_LEN];

#define MIN(a, b) (((a) <= (b)) ? (a) : (b))

static void print_tsfr_error(int code) {
  if (code == XLS_DMA_OK) {
    printf("DMA procedure succeded\n");
    return;
  }
  printf("DMA procedure failed with code %s", xls_dma_err_name(code));
}

typedef void (*on_encoded_dma_t)(void*, rle_sym_t* data);

static void prepare_dma_input_buf(char* data, size_t count) {
  for (size_t i = 0; i < count; i++) {
    dma_tsfr_in_buf[i].e_sym = data[i];
#ifndef RLE_DMA_AXI
    dma_tsfr_in_buf[i].e_last = (i == count - 1);
#endif
  }
}

#ifdef RLE_DMA
void complete_transfer(xls_dma_tsfr_t* tsfr) {
  uint32_t count = tsfr->tsfr_transferred_bytes;
  const char* tsfr_name = (const char*)tsfr->tsfr_ctx;
  printf("DMA transfer \"%s\" complete. Transferred %ld bytes\n",
         tsfr_name, count);
}
#endif /* RLE_DMA */

static int send_rle_input_dma(xls_dma_tsfr_t* tsfr) {
  int err;
  xls_dma_poll_ready(tsfr);
  if ((err = xls_dma_begin_transfer(tsfr))) {
    print_tsfr_error(err);
    return err;
  }
  if ((err = xls_dma_complete_transfer(tsfr, 0))) {
    print_tsfr_error(err);
    return err;
  }

  return XLS_DMA_OK;
}

static int receive_rle_output_dma(xls_dma_tsfr_t* tsfr) {
  int err;
  xls_dma_poll_ready(tsfr);
  if ((err = xls_dma_begin_transfer(tsfr))) {
    print_tsfr_error(err);
    return err;
  }
  if ((err = xls_dma_complete_transfer(tsfr, RLE_TIMEOUT_CYCLES))) {
#ifdef RLE_DMA_AXI
    print_tsfr_error(err);
    return err;
#else  /* RLE_DMA_AXI */
    if (err != XLS_DMA_TIMEOUT) {
      print_tsfr_error(err);
      return err;
    }
    uint32_t count = tsfr->tsfr_transferred_bytes;
    const char* tsfr_name = (const char*)tsfr->tsfr_ctx;
    printf("DMA tranfer \"%s\" timed out. Transferred %ld bytes\n",
           tsfr_name, count);
    xls_dma_cancel_transfer(tsfr);
#endif /* RLE_DMA_AXI */
  }

  return XLS_DMA_OK;
}

static void run_text_rle_dma(char* data, void* ctx, on_encoded_t callback) {
  size_t remaining = strlen(data);
  while (remaining) {
    uint64_t tsfr_len = MIN(remaining, DMATSFR_BUF_LEN);

    prepare_dma_input_buf(data, tsfr_len);

    // clang-format off
    xls_dma_tsfr_t input_transfer = {
        .tsfr_dma          = rle0_dma,
        .tsfr_chan         = RLE_RD_CHAN,
        .tsfr_data         = dma_tsfr_in_buf,
        .tsfr_len          = tsfr_len * sizeof(rle_enc_in_data_t),
        .tsfr_ignore       = 0,
        .tsfr_dir          = XLS_TSFR_TO_PERIPHERAL,
        .tsfr_ctx          = "SIM->XLS",
        .tsfr_callback_isr = &complete_transfer,
#ifdef RLE_DMA_IRQ
        .tsfr_dma_man      = &rle0_dma_man,
        .tsfr_polling      = 0,
#else  /* RLE_DMA_IRQ */
        .tsfr_polling      = 1,
#endif /* RLE_DMA_IRQ */
    };
    xls_dma_tsfr_t output_transfer = {
        .tsfr_dma          = rle0_dma,
        .tsfr_chan         = RLE_WR_CHAN,
        .tsfr_data         = dma_tsfr_out_buf,
        .tsfr_len          = tsfr_len * sizeof(rle_enc_out_data_t),
        .tsfr_ignore       = 0,
        .tsfr_dir          = XLS_TSFR_FROM_PERIPHERAL,
        .tsfr_ctx          = "XLS->SIM",
        .tsfr_callback_isr = &complete_transfer,
#ifdef RLE_DMA_IRQ
        .tsfr_dma_man      = &rle0_dma_man,
        .tsfr_polling      = 0,
#else  /* RLE_DMA_IRQ */
        .tsfr_polling      = 1,
#endif /* RLE_DMA_IRQ */
    };
    // clang-format on

    if (send_rle_input_dma(&input_transfer)) {
      return;
    }
    if (receive_rle_output_dma(&output_transfer)) {
      return;
    }

#ifdef RLE_DMA_AXI
    uint32_t output_elements_cnt =
        output_transfer.tsfr_transferred_bytes / sizeof(rle_enc_out_data_t);
    for (int i = 0; i < output_elements_cnt; ++i) {
      callback(ctx, dma_tsfr_out_buf[i]);
    }
#else  /* RLE_DMA_AXI */
    for (int i = 0; i < tsfr_len; i++) {
      callback(ctx, dma_tsfr_out_buf[i]);
      if (dma_tsfr_out_buf[i].e_last) break;
    }
#endif /* RLE_DMA_AXI */

    remaining -= tsfr_len;
  }
}
#endif

static void print_encoded_sym(void* ctx, rle_enc_out_data_t sym) {
#ifdef RLE_DMA_AXI
  printf("[%c, %u]\n", (char)sym.e_sym, sym.e_count);
#else  /* RLE_DMA_AXI */
  printf("[%c, %u]%s\n", (char)sym.e_sym, sym.e_count,
         sym.e_last ? " (last)" : "");
#endif /* RLE_DMA_AXI */
}

void check_init(void) {
  if (_init_mcause != 0) {
    printf(
        "[FATAL] An unhandled expection has been encountered at 0x%08X, "
        "cause: 0x%08X\n[INFO]  Machine has been reset.",
        _init_mbadaddr, _init_mcause);
  } else {
    printf("%s: Seems that we started cleanly.\n", __func__);
  }
  _init_mcause = 0;
  /* Identify initial system state */
  printf(
      "RISC-V CSRs: MVENDORID=%08x, MARCHID=%08x, MIMPID=%08x, "
      "MHARTID=%08x, "
      "MSTATUS=%08x, MISA=%08x, MIE=%08x\n",
      (unsigned)rv32_csr_read(CSR_MVENDORID),
      (unsigned)rv32_csr_read(CSR_MARCHID), (unsigned)rv32_csr_read(CSR_MIMPID),
      (unsigned)rv32_csr_read(CSR_MHARTID),
      (unsigned)rv32_csr_read(CSR_MSTATUS), (unsigned)rv32_csr_read(CSR_MISA),
      (unsigned)rv32_csr_read(CSR_MIE));
}

#define INPUT_BUF_STRLEN 256
#define QUOTE(A) #A
#define CAT3(A, B, C) #A QUOTE(B) #C
#define FMT_INPUT_BUF CAT3(%, INPUT_BUF_STRLEN, s)

int main(void) {
  check_init();

  char rle_input[INPUT_BUF_STRLEN + 1];

#ifdef RLE_DMA
#ifdef RLE_DMA_IRQ
  interrupt_init_external();
  interrupt_enable_external(RLE_DMA_IRQ_NUM);

  uint32_t priority_cnt;
  if ((priority_cnt = interrupt_priority_count()))
    interrupt_set_priority(RLE_DMA_IRQ_NUM, priority_cnt - 1);

  rv32_csr_write(CSR_MIE, (uint32_t)1 << 11);   /* mie.MEIE=1 */
  rv32_csr_write(CSR_MSTATUS, (uint32_t)1 << 3); /* mstatus.MIE=1 */
#endif

  if (!xls_dma_ok(rle0_dma)) {
    printf("DMA NOT OK\n");
    return 0;
  }

#ifdef PRINT_DMA_ADDRS
  printf("dma_tsfr_in_buf addr: %p\n", dma_tsfr_in_buf);
  printf("dma_tsfr_out_buf addr: %p\n", dma_tsfr_out_buf);
#endif
#endif

  printf("[INFO] Input symbol size: %d bytes\n", sizeof(rle_enc_in_data_t));
  printf("[INFO] Output symbol size: %d bytes\n", sizeof(rle_enc_out_data_t));

  while (1) {
    printf("Enter RLE input:\n");
    scanf(FMT_INPUT_BUF, rle_input);

    printf("RLE input: %s\n", rle_input);
    printf("Running RLE...\n");
#ifdef RLE_DMA
    run_text_rle_dma(rle_input, NULL, print_encoded_sym);
#else
    run_text_rle(rle_input, NULL, print_encoded_sym);
#endif
    printf("\n");
  }

  return 0;
}
