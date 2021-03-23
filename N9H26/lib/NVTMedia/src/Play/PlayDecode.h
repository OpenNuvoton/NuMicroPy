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


#ifndef __PLAY_DECODE_H__
#define __PLAY_DECODE_H__

typedef enum{
	eDECODE_IOCTL_NONE,

	//Input command
	eDECODE_IOCTL_GET_STATE,
	
	
	//Output commadn
	eDECODE_IOCTL_SET_STATE = 0x8000,
	
}E_DECODE_IOCTL_CODE;


E_NM_ERRNO
PlayDecode_Init(
	S_PLAY_DECODE_RES *psDecodeRes
);

void
PlayDecode_Final(
	S_PLAY_DECODE_RES *psDecodeRes
);

E_NM_ERRNO
PlayDecode_ThreadCreate(
	S_PLAY_DECODE_RES *psDecodeRes
);

void
PlayDecode_ThreadDestroy(
	S_PLAY_DECODE_RES *psDecodeRes
);

E_NM_ERRNO
PlayDecode_Run(
	S_PLAY_DECODE_RES *psDecodeRes
);

E_NM_ERRNO
PlayDecode_Stop(
	S_PLAY_DECODE_RES *psDecodeRes
);

#endif

