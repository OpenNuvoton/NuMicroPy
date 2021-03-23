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

#ifndef __NVTMEDIA_MEDIA_IF_H__
#define __NVTMEDIA_MEDIA_IF_H__

typedef enum{
	eNM_MEDIA_UNKNOWN = -1,
	eNM_MEDIA_NONE,
	eNM_MEDIA_AVI,
	eNM_MEDIA_MP4,
	eNM_MEDIA_RTMP,
	eNM_MEDIA_RTPREAD,
	eNM_MEDIA_RTPWRITE,
	eNM_MEDIA_END,	
}E_NM_MEDIA_TYPE;

typedef struct{
	int32_t i32FD;
	uint32_t u32Duration;	// milli second
	uint32_t u32VideoChunks;	//Total video chunks
	uint32_t u32AudioChunks;	//Total video chunks
}S_NM_GENERIC_MEDIA_ATTR;

typedef struct{
	int32_t i32FD;
	uint32_t u32MediaSize;		//for AVI reader
	char *szIdxFilePath;		//for AVI writer

	//output for AVI reader //input for AVI writer
	uint32_t u32Duration;	// milli second

	//output for AVI reader
	uint32_t u32VideoChunks;	//Total video chunks
	uint32_t u32AudioChunks;	//Total video chunks
	
	//output for AVI reader H264 
	E_NM_H264_PROFILE eH264Profile;			///< H.264 profile
	uint32_t u32H264Level;				///< H.264 level	

	//output for AVI reader AAC 
	E_NM_AAC_PROFILE eAACProfile;
	
}S_NM_AVI_MEDIA_ATTR;

typedef struct{
	int32_t i32FD;
	uint32_t u32MediaSize;		//for MP4 reader

	//output for MP4 reader
	uint64_t u64Duration;	// milli second

	//output for MP4 reader
	uint32_t u32VideoChunks;	//Total video chunks
	uint32_t u32AudioChunks;	//Total video chunks
	
	//output for MP4 reader H264 
	E_NM_H264_PROFILE eH264Profile;			///< H.264 profile
	uint32_t u32H264Level;				///< H.264 level	

	//output for AVI reader AAC 
	E_NM_AAC_PROFILE eAACProfile;
	
}S_NM_MP4_MEDIA_ATTR;

typedef struct{
	char *szRTMPUrl;
}S_NM_RTMP_MEDIA_ATTR;

typedef struct{
	void		*pAudConfData;			/**< Optional audio config data
											- AAC: profile
										*/
	uint32_t	u32AudConfLen;			///< Audio condig length

	uint32_t	u32AudPayloadType;		///< Audio RFC 3551 payload type
	int32_t		i32AudRTPPort;			///< Audio RTP session port
	int32_t		i32AudRTCPPort;			///< Audio RTCP port

	void		*pVidConfData;			/**< Optional video config data
											- H.264: SPS and PPS
										*/

	uint32_t	u32VidConfLen;			///< Video config length
	uint32_t	u32VidPayloadType;		///< Video RFC 3551 payload type
	int32_t		i32VidRTPPort;			///< Video RTP session port
	int32_t		i32VidRTCPPort;			///< Video RTCP port

	// Optional to use user private transport layer for RTP packet receive and transmite
	void		*pvAudRTPTransport;		///< struct RtpTransport of libortp
	void		*pvAudRTCPTransport;	///< struct RtpTransport of libortp
	void		*pvVidRTPTransport;		///< struct RtpTransport of libortp
	void		*pvVidRTCPTransport;	///< struct RtpTransport of libortp
}S_NM_RTPREAD_MEDIA_ATTR;

typedef struct{
	char		*pszRemoteAddr;			///< Remote IP address in format "xxx.xxx.xxx.xxx".

	uint32_t	u32AudPayloadType;		///< Audio RFC 3551 payload type
	int32_t		i32AudRTPPort;			///< Audio RTP session port
	int32_t		i32AudRTCPPort;			///< Audio RTCP port
	uint32_t	u32AudSSRC;				///< Audio RTP SSRC (RFC 3550)

	uint32_t	u32VidPayloadType;		///< Video RFC 3551 payload type
	int32_t		i32VidRTPPort;			///< Video RTP session port
	int32_t		i32VidRTCPPort;			///< Video RTCP port
	uint32_t	u32VidSSRC;				///< Video RTP SSRC (RFC 3550)

	uint32_t	u32DataTransSize;		///< Max transmited packet size

	// Optional to use user private transport layer for RTP packet receive and transmite
	void		*pvAudRTPTransport;		///< struct RtpTransport of libortp
	void		*pvAudRTCPTransport;	///< struct RtpTransport of libortp
	void		*pvVidRTPTransport;		///< struct RtpTransport of libortp
	void		*pvVidRTCPTransport;	///< struct RtpTransport of libortp
}S_NM_RTPWRITE_MEDIA_ATTR;

// ==================================================
// Media interface
// ==================================================


/**
 *	- For playback, open media and read media information from it
 * 	- For recording, open media and write media information into it
 *
 * 	@param[in\out]		psVideoCtx	video context
 * 	@param[in\out]		psAudioCtx	audio context
 * 	@param[in]			i32FD	file descriptor
 * 	@param[out]	ppMediaRes		Pointer to media resource
 * 								- NULL if failed
 *
 * 	@return	Error code
 *
 *
 */

typedef	E_NM_ERRNO
(*PFN_NM_MEDIA_OPEN)(
	S_NM_VIDEO_CTX *psVideoCtx,
	S_NM_AUDIO_CTX *psAudioCtx,
	void *psMediaAttr,	
	void **ppMediaRes
);

/**	@brief	Close media
 *
 * 	@param[in]	ppMediaRes		Pointer to media resource
 * 	@param[out]	ppMediaRes		*ppMediaRes = NULL if successful to close the media
 *
 * 	@return	Error code
 *
 * 	@note	All tags will be written to media and freed before media closed.
 *	@note	This function will free media resource.
 *
 */
typedef	E_NM_ERRNO
(*PFN_NM_MEDIA_CLOSE)(
	void	**ppMediaRes
);

/**	@brief	Write video chunk with embedded tags
 *
 * 	@param	u32ChunkID		Chunk ID
 * 	@param	psCtx			Video context to write video chunk
 * 	@param	pMediaRes		Media resource
 *
 * 	@return	Error code
 *
 * 	@note	The embedded tag will be freed after chunk written.
 * 	@note	Application should do sync on demand by itself.
 */
typedef	E_NM_ERRNO
(*PFN_NM_MEDIA_WRITE_VIDEOCHUNK)(
	uint32_t			u32ChunkID,
	const S_NM_VIDEO_CTX	*psCtx,
	void				*pMediaRes
);

/**	@brief	Write audio chunk 
 *
 * 	@param	u32ChunkID		Chunk ID
 * 	@param	psCtx			Audio context to write audio chunk
 * 	@param	pMediaRes		Media resource
 *
 * 	@return	Error code
 *
 * 	@note	The embedded tag will be freed after chunk written.
 * 	@note	Application should do sync on demand by itself.
 */
typedef	E_NM_ERRNO
(*PFN_NM_MEDIA_WRITE_AUDIOCHUNK)(
	uint32_t			u32ChunkID,
	const S_NM_AUDIO_CTX	*psCtx,
	void				*pMediaRes
);

/**	@brief	Read video chunk with embedded tags
 *
 * 	@param	u32ChunkID		Chunk ID
 * 	@param	psCtx			Video context to read video chunk
 * 	@param	pMediaRes		Media resource
 *
 * 	@return	Error code
 *
 */
typedef	E_NM_ERRNO
(*PFN_NM_MEDIA_READ_VIDEOCHUNK)(
	uint32_t			u32ChunkID,
	S_NM_VIDEO_CTX	*psCtx,
	void				*pMediaRes
);

/**	@brief	Read audio chunk 
 *
 * 	@param	u32ChunkID		Chunk ID
 * 	@param	psCtx			Audio context to read audio chunk
 * 	@param	pMediaRes		Media resource
 *
 * 	@return	Error code
 *
 */
typedef	E_NM_ERRNO
(*PFN_NM_MEDIA_READ_AUDIOCHUNK)(
	uint32_t			u32ChunkID,
	S_NM_AUDIO_CTX	*psCtx,
	void				*pMediaRes
);

/**	@brief	Get audio read chunk information 
 *
 * 	@param	u32ChunkID		Chunk ID
 * 	@param	pu32ChunkSize	[out] chunk byte size
 * 	@param	pu64ChunkTime	[out] chunk time offset in millisecond
 * 	@param	pbSeekable		[out] Seekable chunk
 * 	@param	pMediaRes		Media resource
 *
 * 	@return	Error code
 *
 */
typedef	E_NM_ERRNO
(*PFN_NM_MEDIA_GET_AUDIOCHUNK_INFO)(
	uint32_t			u32ChunkID,
	uint32_t			*pu32ChunkSize,
	uint64_t			*pu64ChunkTime,
	bool				*pbSeekable,
	void				*pMediaRes
);

/**	@brief	Get video read chunk information 
 *
 * 	@param	u32ChunkID		Chunk ID
 * 	@param	pu32ChunkSize	[out] chunk byte size
 * 	@param	pu64ChunkTime	[out] chunk time offset in millisecond
 * 	@param	pbSeekable		[out] Seekable chunk
 * 	@param	pMediaRes		Media resource
 *
 * 	@return	Error code
 *
 */
typedef	E_NM_ERRNO
(*PFN_NM_MEDIA_GET_VIDEOCHUNK_INFO)(
	uint32_t			u32ChunkID,
	uint32_t			*pu32ChunkSize,
	uint64_t			*pu64ChunkTime,
	bool				*pbSeekable,
	void				*pMediaRes
);

// ==================================================
// Media write interface
// ==================================================

typedef struct S_NM_MEDIAWRITE_IF {
	E_NM_MEDIA_TYPE						eMediaType;

	PFN_NM_MEDIA_OPEN					pfnOpenMedia;
	PFN_NM_MEDIA_CLOSE					pfnCloseMedia;

	PFN_NM_MEDIA_WRITE_AUDIOCHUNK		pfnWriteAudioChunk;
	PFN_NM_MEDIA_WRITE_VIDEOCHUNK		pfnWriteVideoChunk;
} S_NM_MEDIAWRITE_IF;

typedef struct S_NM_MEDIAREAD_IF {
	E_NM_MEDIA_TYPE						eMediaType;

	PFN_NM_MEDIA_OPEN					pfnOpenMedia;
	PFN_NM_MEDIA_CLOSE					pfnCloseMedia;

	PFN_NM_MEDIA_READ_AUDIOCHUNK		pfnReadAudioChunk;
	PFN_NM_MEDIA_READ_VIDEOCHUNK		pfnReadVideoChunk;

	PFN_NM_MEDIA_GET_AUDIOCHUNK_INFO	pfnGetAudioChunkInfo;
	PFN_NM_MEDIA_GET_VIDEOCHUNK_INFO	pfnGetVideoChunkInfo;

} S_NM_MEDIAREAD_IF;

extern S_NM_MEDIAWRITE_IF g_sMP4WriterIF;
extern S_NM_MEDIAWRITE_IF g_sAVIWriterIF;

extern S_NM_MEDIAREAD_IF g_sAVIReaderIF;
extern S_NM_MEDIAREAD_IF g_sMP4ReaderIF;

#endif
