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

#ifndef __NVTMEDIA_H__
#define __NVTMEDIA_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif


// Boolean type
#if !defined(__cplusplus) && !defined(NM_NBOOL)
	#if defined(bool) && (false != 0 || true != !false)
		#warning bool is redefined: false(0), true(!0)
	#endif

	#undef	bool
	#undef	false
	#undef	true
	#define bool	uint8_t
	#define false	0
	#define true	(!false)

#endif // #if !defined(__cplusplus) && !defined(NM_NBOOL)

#define	eNM_INVALID_FILENO		(-1)					///< Invalid file descriptor
#define	eNM_INVALID_HANDLE		(-1)					///< Invalid play handle
#define	eNM_INVALID_SIZE        (UINT32_MAX)			///< Invalid size
#define	eNM_UNKNOWN_NUM         (0)						///< Unknown number
#define	eNM_UNKNOWN_RATE        (0)						///< Unknown rate
#define	eNM_UNKNOWN_SIZE        (0)						///< Unknown size
#define	eNM_UNKNOWN_TIME        (0)						///< Unknown time
#define	eNM_UNLIMIT_NUM         (UINT32_MAX)			///< Unlimited number
#define	eNM_UNLIMIT_RATE        (UINT32_MAX)			///< Unlimited rate
#define	eNM_UNLIMIT_SIZE        (UINT32_MAX)			///< Unlimited size
#define	eNM_UNLIMIT_TIME        (UINT32_MAX)			///< Unlimited time

typedef enum{
	eNM_ERRNO_NONE,
	eNM_ERRNO_NULL_PTR,
	eNM_ERRNO_MALLOC,
	eNM_ERRNO_CODEC_TYPE,
	eNM_ERRNO_CODEC_OPEN,
	eNM_ERRNO_IO,	
	eNM_ERRNO_SIZE,
	eNM_ERRNO_ENCODE,
	eNM_ERRNO_DECODE,
	eNM_ERRNO_NOT_READY,
	eNM_ERRNO_MEDIA_OPEN,
	eNM_ERRNO_URI,
	eNM_ERRNO_EOM,
	eNM_ERRNO_CHUNK,
	eNM_ERRNO_NULL_RES,	
	eNM_ERRNO_CTX,
	eNM_ERRNO_OS,
	eNM_ERRNO_BAD_PARAM,
	eNM_ERRNO_BUSY,	
}E_NM_ERRNO;

typedef enum{
	eNM_AAC_UNKNOWN = -1,
	eNM_AAC_MAIN = 1,
	eNM_AAC_LC = 2,
	eNM_AAC_SSR = 3,
	eNM_AAC_LTP = 4,
	eNM_AAC_HE_AAC = 5,
	eNM_AAC_ER_LC = 17,
	eNM_AAC_ER_LTP = 19,
	eNM_AAC_LD = 23,
	eNM_AAC_DRM_ER_LC = 27,	
}E_NM_AAC_PROFILE;

typedef enum{
	eNM_H264_UNKNOWN = -1,
	eNM_H264_BASELINE = 66,
	eNM_H264_MAIN = 77,
	eNM_H264_HIGH = 100,
	eNM_H264_HIGH10 = 110,
	eNM_H264_HIGH422 = 122,
	eNM_H264_HIGH444_PREDICTIVE = 244,
}E_NM_H264_PROFILE;

#include "NVTMedia_Log.h"
#include "NVTMedia_Context.h"
#include "NVTMedia_CodecIF.h"
#include "NVTMedia_MediaIF.h"
#include "NVTMedia_Util.h"

typedef enum{
	eNM_MEDIA_FORMAT_FILE,
	eNM_MEDIA_FORMAT_STREMING,
}E_NM_MEDIA_FORMAT;


// ==================================================
// Play data type declaration
// ==================================================

/**	@brief	Play handle
 *
 *	@note	Library could handle multiple handles at the same time.
 */
typedef uint32_t	HPLAY;

typedef enum{
	eNM_PLAY_FASTFORWARD_1X = 1,
	eNM_PLAY_FASTFORWARD_2X = 2,
	eNM_PLAY_FASTFORWARD_4X = 4,
}E_NM_PLAY_FASTFORWARD_SPEED;

typedef enum{
	eNM_PLAY_STATUS_ERROR,
	eNM_PLAY_STATUS_PAUSE,
	eNM_PLAY_STATUS_PLAYING,
	eNM_PLAY_STATUS_EOM,				//End of media
}E_NM_PLAY_STATUS;

typedef struct{
	S_NM_MEDIAREAD_IF *psMediaIF;
	void *pvMediaRes;
	S_NM_CODECDEC_VIDEO_IF *psVideoCodecIF;
	S_NM_CODECDEC_AUDIO_IF *psAudioCodecIF;
	PFN_NM_VIDEOCTX_FLUSHCB pfnVideoFlush;
	PFN_NM_AUDIOCTX_FLUSHCB pfnAudioFlush;
}S_NM_PLAY_IF;

typedef struct{
	S_NM_VIDEO_CTX sMediaVideoCtx;
	S_NM_AUDIO_CTX sMediaAudioCtx;
	S_NM_VIDEO_CTX sFlushVideoCtx;
	S_NM_AUDIO_CTX sFlushAudioCtx;
}S_NM_PLAY_CONTEXT;


typedef struct{
	E_NM_MEDIA_FORMAT eMediaFormat;
	int32_t 	i32MediaHandle;
	E_NM_CTX_VIDEO_TYPE eVideoType;
	E_NM_CTX_AUDIO_TYPE eAudioType;
	uint32_t   u32Duration;	//milli second
	uint32_t u32VideoChunks;
	uint32_t u32AudioChunks;	
}S_NM_PLAY_INFO;

// ==================================================
// Play API declaration
// ==================================================

/**	@brief	Open file/url for playback
 *
 *	@param	szPath[in]		file or url path for playback
 *	@param	psPlayIF[out]	give suggestion on media and codec interface 
 *	@param	psPlayCtx[out]	report media video/audio context
 *	@param	psPlayInfo[out][option]	report media information
 *	@param	pvNMOpenRes[out] return media opened resource, it must be released in NMPlay_Close() 
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMPlay_Open(
	char *szPath,
	S_NM_PLAY_IF *psPlayIF,
	S_NM_PLAY_CONTEXT *psPlayCtx,
	S_NM_PLAY_INFO *psPlayInfo,
	void **ppvNMOpenRes
);

/**	@brief	start/continue play playback
 *	@param	phPlay [in/out] Play engine handle
 *	@param	psPlayIF[in]	specify media and codec interface 
 *	@param	psPlayCtx[in]	specify media and flush video/audio context
 *	@param	bWait[in]	wait operation done
 *  @usage  For start play, *phPlay must equal eNM_INVALID_HANDLE, psPlayIF and psPlayCtx required
 *  @usage  For continue play, *phPlay != eNM_INVALID_HANDLE, psPlayIF and psPlayCtx must NULL
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMPlay_Play(
	HPLAY *phPlay,
	S_NM_PLAY_IF *psPlayIF,
	S_NM_PLAY_CONTEXT *psPlayCtx,
	bool bWait
);

/**	@brief	pause play
 *	@param	hPlay [in] Play engine handle
 *	@param	bWait[in]	wait operation done
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMPlay_Pause(
	HPLAY hPlay,
	bool bWait
);

/**	@brief	Fastforward play
 *	@param	hPlay [in] Play engine handle
 *  @param  eSpeed [in] fastforward speed
 *	@param	bWait[in]	wait operation done
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMPlay_Fastforward(
	HPLAY hPlay,
	E_NM_PLAY_FASTFORWARD_SPEED eSpeed,
	bool bWait
);

/**	@brief	Seek play
 *	@param	hPlay [in] Play engine handle
 *  @param  u32MilliSec [in] seek time
 *  @param  u32TotalVideoChunks [in][option] total video chunks (S_NM_PLAY_INFO.u32VideoChunks) 
 *  @param  u32TotalAudioChunks [in][option] total audio chunks (S_NM_PLAY_INFO.u32AudioChunks)
 *	@param	bWait[in]	wait operation done
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMPlay_Seek(
	HPLAY hPlay,
	uint32_t u32MilliSec,
	uint32_t u32TotalVideoChunks,
	uint32_t u32TotalAudioChunks,
	bool bWait
);

/**	@brief	Get player status
 *	@param	hPlay [in] Play engine handle
 *	@return	E_NM_PLAY_STATUS
 *
 */

E_NM_PLAY_STATUS
NMPlay_Status(
	HPLAY hPlay
);

/**	@brief	close playback
 *	@param	hPlay [in] Play engine handle
 *	@param	pvNMOpenRes[in][option] Must if use NMPlay_Open() to open media
 *  @usage  Execute MediaIF->pfnCloseMedia and close file/streaming handle if ppvNMOpenRes specified.
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMPlay_Close(
	HPLAY hPlay,
	void **ppvNMOpenRes
);

// ==================================================
// Record data type declaration
// ==================================================

/**	@brief	Record handle
 *
 *	@note	Library could handle multiple handles at the same time.
 */
typedef uint32_t	HRECORD;

typedef enum{
	eNM_RECORD_STATUS_ERROR,
	eNM_RECORD_STATUS_PAUSE,
	eNM_RECORD_STATUS_RECORDING,
	eNM_RECORD_STATUS_CHANGE_MEDIA,
	eNM_RECORD_STATUS_STOP,
}E_NM_RECORD_STATUS;

typedef struct{
	S_NM_MEDIAWRITE_IF *psMediaIF;
	void *pvMediaRes;
	S_NM_CODECENC_VIDEO_IF *psVideoCodecIF;
	S_NM_CODECENC_AUDIO_IF *psAudioCodecIF;
	PFN_NM_VIDEOCTX_FILLCB pfnVideoFill;
	PFN_NM_AUDIOCTX_FILLCB pfnAudioFill;
}S_NM_RECORD_IF;

typedef struct{
	S_NM_VIDEO_CTX sFillVideoCtx;
	S_NM_AUDIO_CTX sFillAudioCtx;
	S_NM_VIDEO_CTX sMediaVideoCtx;
	S_NM_AUDIO_CTX sMediaAudioCtx;
}S_NM_RECORD_CONTEXT;

typedef struct{
	E_NM_MEDIA_FORMAT eMediaFormat;
	E_NM_CTX_VIDEO_TYPE eVideoType;
	E_NM_CTX_AUDIO_TYPE eAudioType;
	uint32_t   u32Duration;	//milli second
	uint32_t u32VideoChunks;
	uint32_t u32AudioChunks;
	uint64_t u64TotalChunkSize;
}S_NM_RECORD_INFO;

/**	@brief	Non-blocking callback function to notify recording status
 *
 * 	@param[out]	eStatus: Recording status
 *
 * 	@return	none
 *
 */
typedef	void
(*PFN_NM_RECORD_STATUSCB)(
	E_NM_RECORD_STATUS eStatus,
	S_NM_RECORD_INFO *psInfo,
	void *pvPriv
);


// ==================================================
// Record API declaration
// ==================================================

/**	@brief	Open file/url for record
 *
 *	@param	szPath[in]	file or url path for record
 *	@param	eMediaType[in]	specify record media type
 *	@param	u32Duration[in]	Limit of A/V millisecond duration. eNM_UNLIMIT_TIME: unlimited
 *	@param	psRecordCtx[in]	specify media video/audio context type
 *	@param	psRecordIF[out]	give suggestion on media and codec interface 
 *	@param	pvNMOpenRes[out] return media opened resource, it must be released in NMRecord_Close() 
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMRecord_Open(
	char *szPath,
	E_NM_MEDIA_TYPE eMediaType,
	uint32_t u32Duration,
	S_NM_RECORD_CONTEXT *psRecordCtx,
	S_NM_RECORD_IF *psRecordIF,
	void **ppvNMOpenRes
);


/**	@brief	close record
 *	@param	hRecord [in] Rcord engine handle
 *	@param	pvNMOpenRes[in][option] Must if use NMRecord_Open() to open media
 *  @usage  Execute MediaIF->pfnCloseMedia and close file/streaming handle if ppvNMOpenRes specified.
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMRecord_Close(
	HRECORD hRecord,
	void **ppvNMOpenRes
);

/**	@brief	start record
 *	@param	u32Duration[in]	Limit of A/V millisecond duration. eNM_UNLIMIT_TIME: unlimited
 *	@param	phRecord [in/out] Play engine handle
 *	@param	psRecordIF[in]	specify media and codec interface 
 *	@param	psRecordCtx[in]	specify media and fill video/audio context
 *	@param	pfnStatusCB[in]	specify status notification callback
 *	@param	pvStatusCBPriv[in]	specify status notification callback private data
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMRecord_Record(
	HRECORD *phRecord,
	uint32_t u32Duration,
	S_NM_RECORD_IF *psRecordIF,
	S_NM_RECORD_CONTEXT *psRecordCtx,
	PFN_NM_RECORD_STATUSCB pfnStatusCB,
	void *pvStatusCBPriv
);

/**	@brief	Register next record media
 *	@param	hRecord [in] Record engine handle
 *	@param	psMediaIF[in]	specify next media interface 
 *	@param	pvMediaRes[in]	specify next media resource
 *	@param	pvStatusCBPriv[in]	specify status notification callback private data
 *	@return	E_NM_ERRNO
 *
 */

E_NM_ERRNO
NMRecord_RegNextMedia(
	HRECORD hRecord,
	S_NM_MEDIAWRITE_IF *psMediaIF,
	void *pvMediaRes,
	void *pvStatusCBPriv
);




#ifdef __cplusplus
}
#endif

#endif
