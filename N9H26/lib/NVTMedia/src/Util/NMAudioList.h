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

#ifndef __NM_AUDIO_LIST_H__
#define __NM_AUDIO_LIST_H__

#include <inttypes.h>

#include "NMPacket.h"

void *
NMAudioList_Create(
	E_NM_CTX_AUDIO_TYPE eAudioType,
	uint64_t u64ListDuration					//milli sec
);

void
NMAudioList_Destroy(
	void **ppvListRes
);

S_NMPACKET *
NMAudioList_AcquirePacket(
	void *pvListRes
);

void
NMAudioList_ReleasePacket(
	S_NMPACKET *psPacket,
	void *pvListRes
);

void
NMAudioList_LockList(
	void *pvListRes
);

void
NMAudioList_UnlockList(
	void *pvListRes
);

S_NMPACKET *
NMAudioList_TouchPacket(
	void *pvListRes,
	bool bLock
);

void NMAudioList_PutAudioData(
	S_NM_FRAME_DATA *psAudioFrame,
	void			*pvPriv
);

S_NMPACKET *
NMAudioList_AcquireClosedPacket(
	void *pvListRes,
	uint64_t u64Time
);


#endif
