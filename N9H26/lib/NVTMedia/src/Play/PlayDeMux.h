
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

#ifndef __PLAY_DEMUX_H__
#define __PLAY_DEMUX_H__

typedef enum{
	eDEMUX_IOCTL_NONE,

	//Output command
	eDEMUX_IOCTL_GET_STATE,
	
	
	//Input commadn
	eDEMUX_IOCTL_SET_STATE = 0x8000,
	eDEMUX_IOCTL_SET_FF_SPEED,
	eDEMUX_IOCTL_SET_SEEK,
	eDEMUX_IOCTL_SET_PLAY,
	eDEMUX_IOCTL_SET_PAUSE,
	
}E_DEMUX_IOCTL_CODE;

typedef struct{
	uint32_t u32SeekTime;
	uint32_t u32TotalVideoChunks;
	uint32_t u32TotalAudioChunks;
}S_DEMUX_IOCTL_SEEK;

E_NM_ERRNO
PlayDeMux_Init(
	S_PLAY_DEMUX_RES *psDeMuxRes
);

E_NM_ERRNO
PlayDeMux_ThreadCreate(
	S_PLAY_DEMUX_RES *psDeMuxRes
);

void
PlayDeMux_ThreadDestroy(
	S_PLAY_DEMUX_RES *psDeMuxRes
);

void
PlayDeMux_Final(
	S_PLAY_DEMUX_RES *psDeMuxRes
);

E_NM_ERRNO
PlayDeMux_Play(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	bool bWait
);

E_NM_ERRNO
PlayDeMux_Pause(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	bool bWait
);

E_NM_ERRNO
PlayDeMux_Fastforward(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	E_NM_PLAY_FASTFORWARD_SPEED eSpeed,
	bool bWait
);

E_NM_ERRNO
PlayDeMux_Seek(
	S_PLAY_DEMUX_RES *psDeMuxRes,
	uint32_t u32SeekTime,
	uint32_t u32TotalVideoChunks,
	uint32_t u32TotalAudioChunks,
	bool bWait
);

E_NM_PLAY_STATUS
PlayDeMux_Status(
	S_PLAY_DEMUX_RES *psDeMuxRes
);

#endif

