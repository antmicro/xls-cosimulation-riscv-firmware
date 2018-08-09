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

#ifndef __RLE_H__
#define __RLE_H__

#include <stdint.h>

#include "xls/xls_dma.h"
#include "xls/xls_stream.h"

// clang-format off
#define RLE0_BASE            0x70000000
#define RLE_INPUT_R_OFFSET       0x0000
#define RLE_OUTPUT_S_OFFSET      0x0400

#ifdef CPU_VEXRISCV
#define RLE_DMA_IRQMASK      0x00000010
#define RLE_DMA_IRQPEND      0x00000010
#endif
#ifdef CPU_U54_MC
#define RLE_DMA_IRQMASK      0x00000010
#define RLE_DMA_IRQPEND      0x00000010
#endif

#define RLE_DMA_IRQ_NUM 4


#define RLE_RD_CHAN                   0
#define RLE_WR_CHAN                   1

#define RLE_COUNT_WIDTH 2
// clamng-format on

typedef uint32_t rle_sym_t;

typedef struct __attribute__((packed, aligned(1))) rle_enc_in_data {
  rle_sym_t e_sym;
#ifndef RLE_DMA_AXI
  uint8_t e_last : 1;
#endif
} rle_enc_in_data_t;

typedef struct __attribute__((packed, aligned(1))) rle_enc_out_data {
  rle_sym_t e_sym;
  uint8_t e_count : RLE_COUNT_WIDTH;
#ifndef RLE_DMA_AXI
  /* Note - this works in GCC, but bit-field arrangement is not defined
   * as a part of C standard and may differ in other compilers */
  uint8_t e_last : 1;
#endif
} rle_enc_out_data_t;

XLS_TYPED_STREAM(rle_enc_in_data_t);
XLS_TYPED_STREAM(rle_enc_out_data_t);

typedef struct rle_io {
  xls_stream_rle_enc_in_data_t* const io_input_r;
  xls_stream_rle_enc_out_data_t* const io_output_s;
} rle_io_t;

#ifdef RLE_DMA
extern xls_dma_t* rle0_dma;
#ifdef RLE_DMA_IRQ
extern xls_dma_man_t rle0_dma_man;
#endif
#else
extern rle_io_t rle0_io;
#endif

#endif /* __RLE_H__ */
