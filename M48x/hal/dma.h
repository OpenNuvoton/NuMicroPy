/* mbed Microcontroller Library
 * Copyright (c) 2015-2016 Nuvoton
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
 */

#ifndef MBED_DMA_H
#define MBED_DMA_H


#ifdef __cplusplus
extern "C" {
#endif

#define DMA_CAP_NONE    (0 << 0)

#define DMA_EVENT_ABORT             (1 << 0)
#define DMA_EVENT_TRANSFER_DONE     (1 << 1)
#define DMA_EVENT_TIMEOUT           (1 << 2)
#define DMA_EVENT_ALL               (DMA_EVENT_ABORT | DMA_EVENT_TRANSFER_DONE | DMA_EVENT_TIMEOUT)
#define DMA_EVENT_MASK              DMA_EVENT_ALL

#define DMA_ERROR_OUT_OF_CHANNELS (-1)

typedef enum {
    DMA_USAGE_NEVER,
    DMA_USAGE_OPPORTUNISTIC,
    DMA_USAGE_ALWAYS,
    DMA_USAGE_TEMPORARY_ALLOCATED,
    DMA_USAGE_ALLOCATED
} DMAUsage;

int dma_channel_allocate(uint32_t capabilities);
int dma_channel_free(int channelid);
void dma_set_handler(int channelid, uint32_t handler, uint32_t id, uint32_t event);
void dma_unset_handler(int channelid);

PDMA_T *dma_modbase(void);
int dma_untransfer_bytecount(int channelid);

int dma_fill_description(
	int channelid, 
	uint32_t u32Peripheral, 
	uint32_t data_width, 
	uint32_t addr_src, 
	uint32_t addr_dst, 
	int32_t length, 
	uint32_t timeout 
);


void Handle_PDMA_Irq(void);

#ifdef __cplusplus
}
#endif

#endif
