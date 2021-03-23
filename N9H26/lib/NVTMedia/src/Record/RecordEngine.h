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

#ifndef __RECORD_ENGINE_H__
#define __RECORD_ENGINE_H__

#include "NVTMedia_SAL_OS.h"

struct s_record_engine_res;

//Encode thread resource

typedef struct{
	pthread_t tVideoEncodeThread;

	pthread_mutex_t tVideoEncodeCtrlMutex;
	sem_t tVideoEncodeCtrlSem;
	
	S_NM_CODECENC_VIDEO_IF *psVideoCodecIF;
	void *pvVideoCodecRes;
	PFN_NM_VIDEOCTX_FILLCB pfnVideoFill;

	S_NM_VIDEO_CTX sMediaVideoCtx;
	S_NM_VIDEO_CTX sFillVideoCtx;

	void *pvVideoEncPriv;
	struct s_record_encode_res *psRecordEncodeRes;
	
}S_RECORD_VIDEO_ENCODE_RES;

typedef struct{
	pthread_t tAudioEncodeThread;

	pthread_mutex_t tAudioEncodeCtrlMutex;
	sem_t tAudioEncodeCtrlSem;

	S_NM_CODECENC_AUDIO_IF *psAudioCodecIF;
	void *pvAudioCodecRes;
	PFN_NM_AUDIOCTX_FILLCB pfnAudioFill;

	S_NM_AUDIO_CTX sMediaAudioCtx;
	S_NM_AUDIO_CTX sFillAudioCtx;

	void *pvAudioEncPriv;
	struct s_record_encode_res *psRecordEncodeRes;
	
}S_RECORD_AUDIO_ENCODE_RES;

typedef struct s_record_encode_res{		
	struct s_record_engine_res *psRecordEngRes;
	
	S_RECORD_VIDEO_ENCODE_RES sVideoEncodeRes;
	S_RECORD_AUDIO_ENCODE_RES sAudioEncodeRes;

}S_RECORD_ENCODE_RES;


//Mux thread resource
typedef struct{
	pthread_t tMuxThread;

	pthread_mutex_t tMuxCtrlMutex;
	sem_t tMuxCtrlSem;
	
	struct s_record_engine_res *psRecordEngRes;
	
	S_NM_MEDIAWRITE_IF *psMediaIF;
	void *pvMediaRes;

	S_NM_VIDEO_CTX sMediaVideoCtx;
	S_NM_AUDIO_CTX sMediaAudioCtx;

	void *pvMuxPriv;
	
	PFN_NM_RECORD_STATUSCB pfnStatusCB;
	void *pvStatusCBPriv;
	
	uint32_t u32RecordTimeLimit;	
	
}S_RECORD_MUX_RES;

//Record engine whole resource
typedef struct s_record_engine_res{
	S_RECORD_MUX_RES sMuxRes;
	S_RECORD_ENCODE_RES sEncodeRes;
	void *pvVideoList;
	void *pvAudioList;
}S_RECORD_ENGINE_RES;


E_NM_ERRNO
RecordEngine_Create(
	HRECORD *phRecord,
	uint32_t u32Duration,
	S_NM_RECORD_IF *psRecordIF,
	S_NM_RECORD_CONTEXT *psRecordCtx,
	PFN_NM_RECORD_STATUSCB pfnStatusCB,
	void *pvStatusCBPriv
);

E_NM_ERRNO
RecordEngine_Destroy(
	HRECORD hRecord
);

E_NM_ERRNO
RecordEncode_Stop(
	S_RECORD_ENCODE_RES *psEncodeRes
);

E_NM_ERRNO
RecordEngine_RegNextMedia(
	HRECORD hRecord,
	S_NM_MEDIAWRITE_IF *psMediaIF,
	void *pvMediaRes,
	void *pvStatusCBPriv
);


#endif

