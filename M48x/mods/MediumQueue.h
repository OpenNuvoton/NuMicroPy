/***************************************************************************//**
 * @file     MediumQueue.h
 * @brief    Medium queue
 * @version  0.0.1
 *
 * SPDX-License-Identifier: Apache-2.0
 * @copyright (C) 2020 Nuvoton Technology Corp. All rights reserved.
 ******************************************************************************/

#ifndef __MEDIUM_QUEUE_H__
#define __MEDIUM_QUEUE_H__

#include <stdio.h>
#include <inttypes.h>

#ifndef bool
#define bool int32_t
#endif

#ifndef false
#define false 0
#endif

#ifndef true
#define true 1
#endif


#define DEF_MEDIUM_QUEUE_CNT		4			// Must be power of 2

typedef enum {
	eMEDUIM_QUEUE_NONE,
	eMEDUIM_QUEUE_NO_MEM,
	eMEDUIM_QUEUE_FULL,
	eMEDUIM_QUEUE_EMPTY,	
	eMEDUIM_QUEUE_SIZE,	
} E_MEDIUM_QUEUE_ERRNO;

typedef enum {
	eUTIL_H264_NAL_UNKNOWN	= 0x00000000,
	eUTIL_H264_NAL_SPS 		= 0x00000001,
	eUTIL_H264_NAL_PPS		= 0x00000002,
	eUTIL_H264_NAL_I		= 0x00000004,
	eUTIL_H264_NAL_P		= 0x00000008,
	eUTIL_H264_NAL_AUD		= 0x00000010,
	eUTIL_H264_NAL_SEI		= 0x00000020,
} E_UTIL_H264_NALTYPE_MASK;


typedef	bool
(*PFN_GET_CALLER_STATE)(void *pvPriv);					//If caller return false, medium return soon from wait loop

typedef void* PV_MEDIUM_QUEUE;

// New medium queue
PV_MEDIUM_QUEUE
MediumQueue_New(
	PFN_GET_CALLER_STATE pfnGetCaller,
	void *pvGetCallerPriv
);

// Free medium queue
void
MediumQueue_Free(
	PV_MEDIUM_QUEUE *ppsMediumQueue
);

// Touch medium data. Without update data read index.
E_MEDIUM_QUEUE_ERRNO
MediumQueue_TouchQueueData(
	PV_MEDIUM_QUEUE pvMediumQueue,
	uint8_t 	*pu8DataBuf,
	uint32_t 	u32DataBufLimit,
	uint32_t 	*pu32DataLen,
	uint64_t	*pu64TimeStamp,
	void		**ppvPriv
);

// Flush medium data. Update data read index only.
E_MEDIUM_QUEUE_ERRNO
MediumQueue_FlushQueueData(
	PV_MEDIUM_QUEUE pvMediumQueue
);

// Import medium data
E_MEDIUM_QUEUE_ERRNO
MediumQueue_ImportQueueData(
	PV_MEDIUM_QUEUE pvMediumQueue,
	uint8_t 	*pu8DataBuf,
	uint32_t 	u32DataBufLimit,
	uint32_t 	*pu32DataLen,
	uint64_t	*pu64TimeStamp,
	void		**ppvPriv
);

// Export medium data
E_MEDIUM_QUEUE_ERRNO
MediumQueue_ExportQueueData(
	PV_MEDIUM_QUEUE pvMediumQueue,
	uint8_t 	*pu8DataBuf,
	uint32_t 	u32DataLen,
	uint64_t	u64TimeStamp,
	bool		bVideoData,
	E_UTIL_H264_NALTYPE_MASK eNALType,
	void		*pvPriv
);

// Get medium info
E_MEDIUM_QUEUE_ERRNO
MediumQueue_GetQueueInfo(
	PV_MEDIUM_QUEUE pvMediumQueue,
	uint8_t 	**pu8DataBuf,
	uint32_t 	*pu32DataLen,
	uint64_t	*pu64TimeStamp,
	void		**ppvPriv
);

#endif //End of __MEDIUM_QUEUE_H__

