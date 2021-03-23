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

#include "NMPacket.h"

S_NMPACKET_LIST *
NMPacket_NewPacketList(void)
{
	S_NMPACKET_LIST *psList;

	psList = calloc(1, sizeof(S_NMPACKET_LIST));

	if (psList == NULL)
		return NULL;

	pthread_mutex_init(&psList->tListMutex, NULL);
	return psList;
}

void
NMPacket_FreePacket(
	S_NMPACKET *psPkt
)
{
	if (psPkt == NULL)
		return;

	free(psPkt);
}

void
NMPacket_FreePacketList(
	S_NMPACKET_LIST **ppsPacketList
)
{
	if ((ppsPacketList == NULL) || (*ppsPacketList == NULL))
		return;

	S_NMPACKET_LIST *psList = *ppsPacketList;
	S_NMPACKET *psFreePkt;

	pthread_mutex_lock(&psList->tListMutex);
	
	while (psList->psListHead) {
		psFreePkt = psList->psListHead;
		psList->psListHead = psFreePkt->psNextPkt;
		//Free packets
		NMPacket_FreePacket(psFreePkt);
	}

	psList->psListTail = NULL;
	pthread_mutex_unlock(&psList->tListMutex);
	pthread_mutex_destroy(&psList->tListMutex);
	free(psList);
	*ppsPacketList = NULL;
}

S_NMPACKET *
NMPacket_NewPacket(
	uint32_t	u32PktSize
)
{
	int32_t i32AllocSize;
	S_NMPACKET *psPkt;

	i32AllocSize = sizeof(S_NMPACKET) + u32PktSize;

	psPkt = malloc(i32AllocSize);

	if (psPkt == NULL){
		NMLOG_ERROR("PrefetchPacket_NewPacket failed size %d\n", i32AllocSize);
		return NULL;
	}

	psPkt->psNextPkt = NULL;

	psPkt->u64PTS = 0;
	psPkt->u64DataTime = 0;
	psPkt->u32DataSize = u32PktSize;
	psPkt->pu8DataPtr = (uint8_t *)((uint32_t)psPkt + sizeof(S_NMPACKET));
	psPkt->u32UsedDataLen = 0;
	psPkt->ePktType = eNMPACKET_NONE;
	psPkt->u64PktSeqNum = 0;

	return psPkt;
}

void
NMPacket_PutPacketToList(
	S_NMPACKET_LIST *psPacketList,
	S_NMPACKET *psPkt
)
{
	if ((!psPacketList) || (!psPkt))
		return;

	//Put packet to list tail
	pthread_mutex_lock(&psPacketList->tListMutex);

	psPkt->psNextPkt = NULL;

	if (psPacketList->psListHead == NULL) {
		psPacketList->psListHead = psPkt;
	}
	else {
		psPacketList->psListTail->psNextPkt = psPkt;
	}

	psPacketList->psListTail = psPkt;

	pthread_mutex_unlock(&psPacketList->tListMutex);
}

S_NMPACKET *
NMPacket_GetPacketFromList(
	S_NMPACKET_LIST *psPacketList,
	bool bLock
)
{
	S_NMPACKET *psAvailPkt = NULL;

	if (!psPacketList)
		return NULL;

	//Get packet from list head
	if (bLock == true)
		pthread_mutex_lock(&psPacketList->tListMutex);

	if (psPacketList->psListHead) {
		psAvailPkt = psPacketList->psListHead;
		psPacketList->psListHead = psAvailPkt->psNextPkt;

		if (psAvailPkt == psPacketList->psListTail) {
			psPacketList->psListTail = psAvailPkt->psNextPkt;
		}

		psAvailPkt->psNextPkt = NULL;
	}

	if (bLock == true)
		pthread_mutex_unlock(&psPacketList->tListMutex);

	return psAvailPkt;
}

S_NMPACKET *
NMPacket_TouchPacketFromList(
	S_NMPACKET_LIST *psPacketList,
	bool bLock
)
{
	S_NMPACKET *psAvailPkt = NULL;

	if (!psPacketList)
		return NULL;

	if (bLock == true)
		pthread_mutex_lock(&psPacketList->tListMutex);

	if (psPacketList->psListHead) {
		psAvailPkt = psPacketList->psListHead;
	}

	if (bLock == true)
		pthread_mutex_unlock(&psPacketList->tListMutex);

	return psAvailPkt;
}
