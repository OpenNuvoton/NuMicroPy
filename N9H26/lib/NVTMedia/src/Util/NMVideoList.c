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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "NVTMedia.h"

#include "NMVideoList.h"

typedef struct {
	E_NM_CTX_VIDEO_TYPE eVideoType;
	uint64_t u64ListDuration;
	S_NMPACKET_LIST *psPacketList;
} S_VIDEOLIST_RES;

static void
ReleaseExpiredPacket(
	S_NMPACKET_LIST *psPacketList,
	uint64_t u64ExpiredTime
)
{
	S_NMPACKET *psTempPkt = NULL;
	S_NMPACKET *psReleasePkt = NULL;
	S_NMPACKET *psEndRelPkt = NULL;	//End released packet

	pthread_mutex_lock(&psPacketList->tListMutex);

	if (!psPacketList->psListHead)
		goto ReleaseExpiredPacket_Done;

	psTempPkt = psPacketList->psListHead;

//	if (psTempPkt->u64DataTime >= u64ExpiredTime)
//		goto ReleaseExpiredPacket_Done;

	//Traversal list and find end released packet
	while (psTempPkt) {
		if (psTempPkt->u64PTS >= u64ExpiredTime)
			break;

		if (psTempPkt->ePktType & eNMPACKET_IFRAME) {
			psEndRelPkt = psTempPkt;
		}

		psTempPkt = psTempPkt->psNextPkt;
	}

	// Released packet
	if (psEndRelPkt) {
		psTempPkt = psPacketList->psListHead;

		while ((psTempPkt) && (psTempPkt != psEndRelPkt)) {
			psTempPkt = psTempPkt->psNextPkt;
			psReleasePkt = NMPacket_GetPacketFromList(psPacketList, false);
			NMPacket_FreePacket(psReleasePkt);
		}
	}

ReleaseExpiredPacket_Done:
	pthread_mutex_unlock(&psPacketList->tListMutex);
}

void *
NMVideoList_Create(
	E_NM_CTX_VIDEO_TYPE eVideoType,
	uint64_t u64ListDuration			//milli second
)
{
	S_VIDEOLIST_RES *psListRes;

	psListRes = calloc(1, sizeof(S_VIDEOLIST_RES));

	if (!psListRes)
		return NULL;

	//New prefetch packet list
	psListRes->psPacketList = NMPacket_NewPacketList();

	if (psListRes->psPacketList == NULL) {
		free(psListRes);
		return NULL;
	}

	psListRes->eVideoType = eVideoType;
	psListRes->u64ListDuration = u64ListDuration;

	return psListRes;
}

void
NMVideoList_Destroy(
	void **ppvListRes
)
{
	if ((ppvListRes == NULL) || (*ppvListRes == NULL))
		return;

	S_VIDEOLIST_RES *psListRes = (S_VIDEOLIST_RES *)*ppvListRes;

	NMPacket_FreePacketList(&psListRes->psPacketList);
	free(psListRes);
	*ppvListRes = NULL;
}

S_NMPACKET *
NMVideoList_AcquirePacket(
	void *pvListRes
)
{
	S_VIDEOLIST_RES *psListRes = (S_VIDEOLIST_RES *)pvListRes;

	return NMPacket_GetPacketFromList(psListRes->psPacketList, true);
}

void
NMVideoList_ReleasePacket(
	S_NMPACKET *psPacket,
	void *pvListRes
)
{
	NMPacket_FreePacket(psPacket);
}

void
NMVideoList_LockList(
	void *pvListRes
)
{
	S_VIDEOLIST_RES *psListRes = (S_VIDEOLIST_RES *)pvListRes;

	if (psListRes == NULL)
		return;

	pthread_mutex_lock(&psListRes->psPacketList->tListMutex);
}

void
NMVideoList_UnlockList(
	void *pvListRes
)
{
	S_VIDEOLIST_RES *psListRes = (S_VIDEOLIST_RES *)pvListRes;

	if (psListRes == NULL)
		return;

	pthread_mutex_unlock(&psListRes->psPacketList->tListMutex);
}

S_NMPACKET *
NMVideoList_TouchPacket(
	void *pvListRes,
	bool bLock
)
{
	S_VIDEOLIST_RES *psListRes = (S_VIDEOLIST_RES *)pvListRes;

	if (psListRes == NULL)
		return NULL;

	return NMPacket_TouchPacketFromList(psListRes->psPacketList, bLock);
}

void NMVideoList_PutVideoData(
	S_NM_FRAME_DATA *psVideoFrame,
	void				*pvPriv
)
{
	S_VIDEOLIST_RES *psListRes = (S_VIDEOLIST_RES *)pvPriv;
	uint64_t u64VideoUpdateTime = 0;

	if ((psVideoFrame == NULL) || (psListRes == NULL))
		return;

	u64VideoUpdateTime = psVideoFrame->u64ChunkPTS;

	//traversal list and release expired packet.
	ReleaseExpiredPacket(psListRes->psPacketList, u64VideoUpdateTime - psListRes->u64ListDuration);

	S_NMPACKET *psNewPkt;
	uint32_t u32FrameSize;

	//New packet
	u32FrameSize = psVideoFrame->u32ChunkSize;
	psNewPkt = NMPacket_NewPacket(u32FrameSize + 100);

	if (!psNewPkt)
		return;

	memcpy(psNewPkt->pu8DataPtr, psVideoFrame->pu8ChunkBuf, u32FrameSize);

	psNewPkt->u64PTS = u64VideoUpdateTime;
	psNewPkt->u64DataTime = psVideoFrame->u64ChunkDataTime;
	psNewPkt->u32UsedDataLen = u32FrameSize;

	if (psListRes->eVideoType == eNM_CTX_VIDEO_H264) {
		//Parser H.264 frame information
		S_NM_UTIL_H264_FRAME_INFO sH264FrameInfo;
		NMUtil_ParseH264Frame(psNewPkt->pu8DataPtr, u32FrameSize, &sH264FrameInfo);

		//Interested in I-frame
		if (sH264FrameInfo.eNALType & eNM_UTIL_H264_NAL_I) {
			psNewPkt->ePktType = eNMPACKET_IFRAME;
		}
	}
	else {
		psNewPkt->ePktType = eNMPACKET_IFRAME;
	}

	//Put packet to list
	NMPacket_PutPacketToList(psListRes->psPacketList, psNewPkt);
}

S_NMPACKET *
NMVideoList_AcquireClosedPacket(
	void *pvListRes,
	uint64_t u64Time
)
{
	S_VIDEOLIST_RES *psListRes = (S_VIDEOLIST_RES *)pvListRes;
	S_NMPACKET_LIST *psPacketList = psListRes->psPacketList;
	
	S_NMPACKET *psTempPkt = NULL;
	S_NMPACKET *psReleasePkt = NULL;
	S_NMPACKET *psEndRelPkt = NULL;	//End released packet

	pthread_mutex_lock(&psPacketList->tListMutex);

	if (!psPacketList->psListHead)
		goto ReleaseExpiredPacket_Done;

	psTempPkt = psPacketList->psListHead;

//	if (psTempPkt->u64DataTime > u64Time)
//		goto ReleaseExpiredPacket_Done;

	//Traversal list and find end released packet
	while (psTempPkt) {
		if (psTempPkt->u64PTS > u64Time)
			break;

		if (psTempPkt->ePktType & eNMPACKET_IFRAME) {
			psEndRelPkt = psTempPkt;
		}

		psTempPkt = psTempPkt->psNextPkt;
	}

	// Released packet
	if (psEndRelPkt) {
		psTempPkt = psPacketList->psListHead;

		while ((psTempPkt) && (psTempPkt != psEndRelPkt)) {
			psTempPkt = psTempPkt->psNextPkt;
			psReleasePkt = NMPacket_GetPacketFromList(psPacketList, false);
			NMPacket_FreePacket(psReleasePkt);
		}
	}

ReleaseExpiredPacket_Done:
	pthread_mutex_unlock(&psPacketList->tListMutex);

	return NMPacket_GetPacketFromList(psListRes->psPacketList, true);
}


