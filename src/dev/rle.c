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

#include "rle.h"

#ifdef RLE_DMA

xls_dma_t* rle0_dma = (xls_dma_t*)RLE0_BASE;

#ifdef RLE_DMA_IRQ
xls_dma_man_t rle0_dma_man = {
    .dman_complete = 0,
    .dman_tlast    = 0,
    .dman_chan_data =
        {[0] = {.dmanch_tsfr = NULL}, [1] = {.dmanch_tsfr = NULL}},
};
#endif /* RLE_DMA_IRQ */

#else /* RLE_DMA */

rle_io_t rle0_io = {
    .io_input_r =
        (xls_stream_rle_enc_in_data_t*)(RLE0_BASE + RLE_INPUT_R_OFFSET),
    .io_output_s =
        (xls_stream_rle_enc_out_data_t*)(RLE0_BASE + RLE_OUTPUT_S_OFFSET)};

#endif /* RLE_DMA */
