/* @copyright (C) 2019 Nuvoton Technology Corp. All rights reserved.
* Redistribution and use in source and binary forms, with or without modification,
* are permitted provided that the following conditions are met:
*   1. Redistributions of source code must retain the above copyright notice,
*      this list of conditions and the following disclaimer.
*   2. Redistributions in binary form must reproduce the above copyright notice,
*      this list of conditions and the following disclaimer in the documentation
*      and/or other materials provided with the distribution.
*   3. Neither the name of Nuvoton Technology Corp. nor the names of its contributors
*      may be used to endorse or promote products derived from this software
*      without specific prior written permission.
* 
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*****************************************************************************/

#ifndef __NVTMEDIA_UTIL_H__
#define __NVTMEDIA_UTIL_H__

#include <inttypes.h>

typedef enum {
	eNM_UTIL_H264_NAL_UNKNOWN	= 0x00000000,
	eNM_UTIL_H264_NAL_SPS 		= 0x00000001,
	eNM_UTIL_H264_NAL_PPS		= 0x00000002,
	eNM_UTIL_H264_NAL_I		= 0x00000004,
	eNM_UTIL_H264_NAL_P		= 0x00000008,
	eNM_UTIL_H264_NAL_AUD		= 0x00000010,
	eNM_UTIL_H264_NAL_SEI		= 0x00000020,
} E_NM_UTIL_H264_NALTYPE_MASK;

typedef struct {
	E_NM_UTIL_H264_NALTYPE_MASK eNALType;
	uint32_t u32SPSOffset;
	uint32_t u32SPSStartCodeLen;
	uint32_t u32SPSLen;
	uint32_t u32PPSOffset;
	uint32_t u32PPSStartCodeLen;
	uint32_t u32PPSLen;
	uint32_t u32IPOffset;
	uint32_t u32IPStartCodeLen;
	uint32_t u32IPLen;
	uint32_t u32AUDOffset;
	uint32_t u32AUDStartCodeLen;
	uint32_t u32AUDLen;
	uint32_t u32SEIOffset;
	uint32_t u32SEIStartCodeLen;
	uint32_t u32SEILen;
} S_NM_UTIL_H264_FRAME_INFO;

// Parse H264 frame
void
NMUtil_ParseH264Frame(
	uint8_t *pu8BitStream,
	uint32_t u32BitStreamLen,
	S_NM_UTIL_H264_FRAME_INFO *psFrameInfo
);

uint16_t
NMUtil_GCD(
	uint16_t m1,
	uint16_t m2
);

E_NM_ERRNO
NMUtil_ParseJPEG(
	S_NM_VIDEO_CTX	*psVideoCtx,
	uint8_t *pu8Buf,
	uint32_t u32BufLen
);

char* 
NMUtil_strdup(
	const char* szStr
);

uint64_t
NMUtil_GetTimeMilliSec(void);

char *
NMUtil_dirname(char *path);

char *
NMUtil_basename(
	char *name
);

void NMUtil_SetLogLevel(
	int i32LogLevel
);

#endif
