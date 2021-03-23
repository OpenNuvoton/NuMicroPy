
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

#ifndef __PLAY_ENGINE_H__
#define __PLAY_ENGINE_H__

#include "NVTMedia_SAL_OS.h"

#define _DEMUX_DEBUG_ 0
#define _DECODE_DEBUG_ 0

struct s_play_engine_res;

//Decode thread resource

typedef struct{
	pthread_t tVideoDecodeThread;

	pthread_mutex_t tVideoDecodeCtrlMutex;
	sem_t tVideoDecodeCtrlSem;
	
	S_NM_CODECDEC_VIDEO_IF *psVideoCodecIF;
	void *pvVideoCodecRes;
	PFN_NM_VIDEOCTX_FLUSHCB pfnVideoFlush;

	S_NM_VIDEO_CTX sMediaVideoCtx;
	S_NM_VIDEO_CTX sFlushVideoCtx;

	void *pvVideoDecPriv;
	struct s_play_decode_res *psPlayDecodeRes;
	
}S_PLAY_VIDEO_DECODE_RES;

typedef struct{
	pthread_t tAudioDecodeThread;

	pthread_mutex_t tAudioDecodeCtrlMutex;
	sem_t tAudioDecodeCtrlSem;

	S_NM_CODECDEC_AUDIO_IF *psAudioCodecIF;
	void *pvAudioCodecRes;
	PFN_NM_AUDIOCTX_FLUSHCB pfnAudioFlush;

	S_NM_AUDIO_CTX sMediaAudioCtx;
	S_NM_AUDIO_CTX sFlushAudioCtx;

	void *pvAudioDecPriv;
	struct s_play_decode_res *psPlayDecodeRes;
	
}S_PLAY_AUDIO_DECODE_RES;

typedef struct s_play_decode_res{		
	struct s_play_engine_res *psPlayEngRes;
	
	S_PLAY_VIDEO_DECODE_RES sVideoDecodeRes;
	S_PLAY_AUDIO_DECODE_RES sAudioDecodeRes;

}S_PLAY_DECODE_RES;

//DeMux thread resource
typedef struct{
	pthread_t tDeMuxThread;

	pthread_mutex_t tDeMuxCtrlMutex;
	int i32SemLock;
	sem_t tDeMuxCtrlSem;
	
	struct s_play_engine_res *psPlayEngRes;
	
	S_NM_MEDIAREAD_IF *psMediaIF;
	void *pvMediaRes;
	S_NM_VIDEO_CTX sMediaVideoCtx;
	S_NM_AUDIO_CTX sMediaAudioCtx;

	void *pvDeMuxPriv;

}S_PLAY_DEMUX_RES;

//Play engine whole resource
typedef struct s_play_engine_res{
	S_PLAY_DEMUX_RES sDeMuxRes;
	S_PLAY_DECODE_RES sDecodeRes;
	void *pvVideoList;
	void *pvAudioList;
}S_PLAY_ENGINE_RES;

E_NM_ERRNO
PlayEngine_Create(
	HPLAY *phPlay,
	S_NM_PLAY_IF *psPlayIF,
	S_NM_PLAY_CONTEXT *psPlayCtx
);

E_NM_ERRNO
PlayEngine_Destroy(
	HPLAY hPlay
);

E_NM_ERRNO
PlayEngine_Play(
	HPLAY hPlay,
	bool bWait
);

E_NM_ERRNO
PlayEngine_Pause(
	HPLAY hPlay,
	bool bWait
);

E_NM_ERRNO
PlayEngine_Fastforward(
	HPLAY hPlay,
	E_NM_PLAY_FASTFORWARD_SPEED eSpeed,
	bool bWait
);

E_NM_ERRNO
PlayEngine_Seek(
	HPLAY hPlay,
	uint32_t u32MilliSec,
	uint32_t u32TotalVideoChunks,
	uint32_t u32TotalAudioChunks,
	bool bWait
);

E_NM_PLAY_STATUS
PlayEngine_Status(
	HPLAY hPlay
);

#endif

