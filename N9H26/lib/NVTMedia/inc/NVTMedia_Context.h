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

#ifndef __NVTMEDIA_CONTEXT_H__
#define __NVTMEDIA_CONTEXT_H__

typedef enum{
	eNM_CTX_VIDEO_UNKNOWN = -1,
	eNM_CTX_VIDEO_NONE,
	eNM_CTX_VIDEO_MJPG,
	eNM_CTX_VIDEO_H264,
	eNM_CTX_VIDEO_YUV422,		//Raw data format
	eNM_CTX_VIDEO_YUV422P,		
	eNM_CTX_VIDEO_YUV420P_MB,		
	eNM_CTX_VIDEO_END,		
}E_NM_CTX_VIDEO_TYPE;

typedef enum{
	eNM_CTX_AUDIO_UNKNOWN = -1,
	eNM_CTX_AUDIO_NONE,
	eNM_CTX_AUDIO_ALAW,
	eNM_CTX_AUDIO_ULAW,
	eNM_CTX_AUDIO_AAC,	
	eNM_CTX_AUDIO_MP3,	
	eNM_CTX_AUDIO_PCM_L16,		//Raw data format
	eNM_CTX_AUDIO_ADPCM,
	eNM_CTX_AUDIO_G726,	
	eNM_CTX_AUDIO_END,	
}E_NM_CTX_AUDIO_TYPE;


typedef struct{
	E_NM_AAC_PROFILE eProfile;

}S_NM_AAC_CTX_PARAM;


//Used for H264 encoder. Optional
typedef struct{
	E_NM_H264_PROFILE eProfile;
	
	uint32_t u32FixQuality; //Fixed quality(dynamic bitrate) (1(Better) ~ 52, Dynamic: 0 (According to bitrate))
	uint32_t u32FixBitrate;	//Fixed bitrate kbps (dynamic quality) (default value is 1024 (1Mbps))
	uint32_t u32FixGOP;			//GOP (0: According to dest context frame rate, 1:All I frame, > 1: Fixed GOP)
	
	uint32_t u32MaxQuality;	//Max quality for fixed bitrate  (1(Better) ~ 51). 0:default 50
	uint32_t u32MinQuality;	//Min quality for fixed bitrate  (1(Better) ~ 51). 0:default 25
	
}S_NM_H264_CTX_PARAM;


typedef struct{
	E_NM_CTX_VIDEO_TYPE eVideoType;		//video type
	uint32_t u32Width;					//video width
	uint32_t u32Height;					//video height
	uint32_t u32FrameRate;
	uint32_t u32BitRate;				//video bit rate
	
	uint8_t *pu8DataBuf;				//data buffer pointer
	uint32_t u32DataSize;				//data size
	uint32_t u32DataLimit;				//data buffer limit
	uint64_t u64DataTime;				//data time
	bool bKeyFrame;						//key frame
	
	void *pvParamSet;					//video parameter set
}S_NM_VIDEO_CTX;


typedef struct{
	E_NM_CTX_AUDIO_TYPE eAudioType;		//audio type
	uint32_t u32SampleRate;				//audio sample rate
	uint32_t u32Channel;				//audio channel
	uint32_t u32SamplePerBlock;			//audio samples per block
	uint32_t u32BitRate;				//audio bit rate
	
	uint8_t *pu8DataBuf;				//data buffer pointer
	uint32_t u32DataSize;				//data size
	uint32_t u32DataLimit;				//data buffer limit
	uint64_t u64DataTime;				//data time
	bool bKeyFrame;						//key frame

	void *pvParamSet;					//audio parameter set	
}S_NM_AUDIO_CTX;

/**	@brief	Non-blocking callback function to flush audio/video context
 *
 * 	@param[out]	psAudioCtx		Audio context
 *
 * 	@return	none
 *
 */
typedef	void
(*PFN_NM_AUDIOCTX_FLUSHCB)(
	S_NM_AUDIO_CTX	*psAudioCtx
);

/**	@brief	Non-blocking callback function to flush audio/video context
 *
 * 	@param[out]	psVideoCtx		Video context
 *
 * 	@return	none
 *
 */
typedef	void
(*PFN_NM_VIDEOCTX_FLUSHCB)(
	S_NM_VIDEO_CTX	*psVideoCtx
);

/**	@brief	Non-blocking callback function to fill audio/video context
 *
 * 	@param[out]	psAudioCtx		Audio context
 *
 * 	@return	none
 *
 */
typedef	bool
(*PFN_NM_AUDIOCTX_FILLCB)(
	S_NM_AUDIO_CTX	*psAudioCtx
);

/**	@brief	Non-blocking callback function to fill audio/video context
 *
 * 	@param[out]	psVideoCtx		Video context
 *
 * 	@return	none
 *
 */
typedef	bool
(*PFN_NM_VIDEOCTX_FILLCB)(
	S_NM_VIDEO_CTX	*psVideoCtx
);



#endif
