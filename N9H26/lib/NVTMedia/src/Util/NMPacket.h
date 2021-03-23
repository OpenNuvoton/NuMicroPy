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

#ifndef __NM_PACKET_H__
#define __NM_PACKET_H__

#include <inttypes.h>

#include "NVTMedia_SAL_OS.h"

typedef enum E_NMPACKET_MASK {
	eNMPACKET_NONE		= 0x00,			///< None
	eNMPACKET_IFRAME	= 0x01,			///< I-frame
	eNMPACKET_PFRAME	= 0x02,			///< P-frame
	eNMPACKET_PPS		= 0x10,			///< PPS
	eNMPACKET_SPS		= 0x20,			///< SPS
} E_NMPACKET_MASK;

typedef struct s_nmpacket {
	struct s_nmpacket 	*psNextPkt;	//Next packet
	uint64_t			u64PTS;	//present time
	uint64_t			u64DataTime;	//Data time
	uint32_t			u32DataSize;	//Total data buffer lenth
	uint8_t			*pu8DataPtr;	//Data buffer address
	uint32_t			u32UsedDataLen;	//how many bytes be used in data buffer
	uint32_t		ePktType;		//AV packet type
	uint64_t 			u64PktSeqNum;	//Packet sequence number
} S_NMPACKET;

typedef struct {
	S_NMPACKET *psListHead;		//Packet list head
	S_NMPACKET *psListTail;		//Packet list tail
	pthread_mutex_t tListMutex;	//packet list mutex
} S_NMPACKET_LIST;

typedef struct{
	uint32_t u32ChunkID;
	bool bKeyFrame;
	uint8_t *pu8ChunkBuf;
	uint32_t u32ChunkSize;
	uint32_t u32ChunkLimit;
	uint64_t u64ChunkPTS;
	uint64_t u64ChunkDataTime;
}S_NM_FRAME_DATA;

S_NMPACKET_LIST *
NMPacket_NewPacketList(void);

void
NMPacket_FreePacketList(
	S_NMPACKET_LIST **ppsPacketList
);

void
NMPacket_FreePacket(
	S_NMPACKET *psPkt
);

S_NMPACKET *
NMPacket_NewPacket(
	uint32_t	u32PktSize
);

void
NMPacket_PutPacketToList(
	S_NMPACKET_LIST *psPacketList,
	S_NMPACKET *psPkt
);

S_NMPACKET *
NMPacket_GetPacketFromList(
	S_NMPACKET_LIST *psPacketList,
	bool bLock
);

S_NMPACKET *
NMPacket_TouchPacketFromList(
	S_NMPACKET_LIST *psPacketList,
	bool bLock
);

#endif

