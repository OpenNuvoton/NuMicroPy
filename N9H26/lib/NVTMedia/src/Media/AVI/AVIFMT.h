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

 #ifndef __AVI_FMT_H__
 #define __AVI_FMT_H__
 /* The following is a short description of the AVI file format.  Please
 * see the accompanying documentation for a full explanation.
 *
 * An AVI file is the following RIFF form:
 *
 *	RIFF('AVI'
 *	      LIST('hdrl'
 *		    avih(<MainAVIHeader>)
 *                  LIST ('strl'
 *                      strh(<Stream header>)
 *                      strf(<Stream format>)
 *                      ... additional header data
 *            LIST('movi'	
 *      	  { LIST('rec'
 *      		      SubChunk...
 *      		   )
 *      	      | SubChunk } ....	
 *            )
 *            [ <AVIIndex> ]
 *      )
 *
 *	The main file header specifies how many streams are present.  For
 *	each one, there must be a stream header chunk and a stream format
 *	chunk, enlosed in a 'strl' LIST chunk.  The 'strf' chunk contains
 *	type-specific format information; for a video stream, this should
 *	be a BITMAPINFO structure, including palette.  For an audio stream,
 *	this should be a WAVEFORMAT (or PCMWAVEFORMAT) structure.
 *
 *	The actual data is contained in subchunks within the 'movi' LIST
 *	chunk.  The first two characters of each data chunk are the
 *	stream number with which that data is associated.
 *
 *	Some defined chunk types:
 *           Video Streams:
 *                  ##db:	RGB DIB bits
 *                  ##dc:	RLE8 compressed DIB bits
 *                  ##pc:	Palette Change
 *
 *           Audio Streams:
 *                  ##wb:	waveform audio bytes
 *
 * The grouping into LIST 'rec' chunks implies only that the contents of
 *   the chunk should be read into memory at the same time.  This
 *   grouping is used for files specifically intended to be played from
 *   CD-ROM.
 *
 * The index chunk at the end of the file should contain one entry for
 *   each data chunk in the file.
 *
 * Limitations for the current software:
 *	Only one video stream and one audio stream are allowed.
 *	The streams must start at the beginning of the file.
 *
 *
 * To register codec types please obtain a copy of the Multimedia
 * Developer Registration Kit from:
 *
 *  Microsoft Corporation
 *  Multimedia Systems Group
 *  Product Marketing
 *  One Microsoft Way
 *  Redmond, WA 98052-6399
 *
 * Reference website: http://msdn.microsoft.com/en-us/library/windows/desktop/dd318189(v=vs.85).aspx
 */
 
#include <stdint.h>


/*
** Main AVI File Header
*/	
		
/* flags for use in <dwFlags> in AVIFileHdr */
#define AVIF_HASINDEX		0x00000010	// Index at end of file
#define AVIF_MUSTUSEINDEX	0x00000020
#define AVIF_ISINTERLEAVED	0x00000100
#define AVIF_TRUSTCKTYPE	0x00000800	// Use CKType to find key frames
#define AVIF_WASCAPTUREFILE	0x00010000
#define AVIF_COPYRIGHTED	0x00020000

/* The AVI File Header LIST chunk should be padded to this size */
#define AVI_HEADERSIZE  2048                    // size of AVI header list
#define AVISF_DISABLED			0x00000001
#define AVISF_VIDEO_PALCHANGES		0x00010000 

typedef unsigned int   FOURCC;
typedef long    LONG; 

#ifndef mmioFOURCC
#define mmioFOURCC( ch0, ch1, ch2, ch3 )				\
		( (uint32_t)(uint8_t)(ch0) | ( (uint32_t)(uint8_t)(ch1) << 8 ) |	\
		( (uint32_t)(uint8_t)(ch2) << 16 ) | ( (uint32_t)(uint8_t)(ch3) << 24 ) )
#endif

/* Macro to make a TWOCC out of two characters */
#ifndef aviTWOCC
#define aviTWOCC(ch0, ch1) ((UINT16)(BYTE)(ch0) | ((UINT16)(BYTE)(ch1) << 8))
#endif

//typedef UINT16 TWOCC;

/* form types, list types, and chunk types */
#define formtypeAVI             mmioFOURCC('A', 'V', 'I', ' ')
#define listtypeAVIHEADER       mmioFOURCC('h', 'd', 'r', 'l')
#define ckidAVIMAINHDR          mmioFOURCC('a', 'v', 'i', 'h')
#define listtypeSTREAMHEADER    mmioFOURCC('s', 't', 'r', 'l')
#define ckidSTREAMHEADER        mmioFOURCC('s', 't', 'r', 'h')
#define ckidSTREAMFORMAT        mmioFOURCC('s', 't', 'r', 'f')
#define ckidSTREAMHANDLERDATA   mmioFOURCC('s', 't', 'r', 'd')
#define ckidSTREAMNAME			mmioFOURCC('s', 't', 'r', 'n')

#define listtypeAVIMOVIE        mmioFOURCC('m', 'o', 'v', 'i')
#define listtypeAVIRECORD       mmioFOURCC('r', 'e', 'c', ' ')

#define ckidAVINEWINDEX         mmioFOURCC('i', 'd', 'x', '1')

/*
** Stream types for the <fccType> field of the stream header.
*/
#define streamtypeVIDEO         mmioFOURCC('v', 'i', 'd', 's')
#define streamtypeAUDIO         mmioFOURCC('a', 'u', 'd', 's')
#define streamtypeMIDI			mmioFOURCC('m', 'i', 'd', 's')
#define streamtypeTEXT          mmioFOURCC('t', 'x', 't', 's')

/* Basic chunk types */
#define cktypeDIBbits           aviTWOCC('d', 'b')
#define cktypeDIBcompressed     aviTWOCC('d', 'c')
#define cktypePALchange         aviTWOCC('p', 'c')
#define cktypeWAVEbytes         aviTWOCC('w', 'b')

/* Chunk id to use for extra chunks for padding. */
#define ckidAVIPADDING          mmioFOURCC('J', 'U', 'N', 'K')

/*
** Useful macros
**
** Warning: These are nasty macro, and MS C 6.0 compiles some of them
** incorrectly if optimizations are on.  Ack.
*/

/* Macro to get stream number out of a FOURCC ckid */
#define FromHex(n)	(((n) >= 'A') ? ((n) + 10 - 'A') : ((n) - '0'))
#define StreamFromFOURCC(fcc) ((UINT16) ((FromHex(LOBYTE(LOWORD(fcc))) << 4) + \
                                             (FromHex(HIBYTE(LOWORD(fcc))))))

/* Macro to get TWOCC chunk type out of a FOURCC ckid */
#define TWOCCFromFOURCC(fcc)    HIWORD(fcc)

/* Macro to make a ckid for a chunk out of a TWOCC and a stream number
** from 0-255.
*/
#define ToHex(n)	((BYTE) (((n) > 9) ? ((n) - 10 + 'A') : ((n) + '0')))
#define MAKEAVICKID(tcc, stream) \
        MAKELONG((ToHex((stream) & 0x0f) << 8) | \
			    (ToHex(((stream) & 0xf0) >> 4)), tcc)

// AVI main header
// size is 56 bytes
typedef struct
{
    uint32_t		u32MicroSecPerFrame;	// frame display rate (or 0L)
    uint32_t		u32MaxBytesPerSec;	// max. transfer rate
    uint32_t		u32PaddingGranularity;	// pad to multiples of this
                                                // size; normally 2K.
    uint32_t		u32Flags;		// the ever-present flags
    uint32_t		u32TotalFrames;		// # frames in file
    uint32_t		u32InitialFrames;
    uint32_t		u32Streams;
    uint32_t		u32SuggestedBufferSize;

    uint32_t		u32Width;
    uint32_t		u32Height;

    uint32_t		u32Reserved[4];
} S_AVIFMT_MAINAVIHEADER;
// AVI main header

// AVI Stream header
// 56 bytes
typedef struct 
{
    FOURCC		fccType;
    FOURCC		fccHandler;
    uint32_t	u32Flags;	/* Contains AVITF_* flags */
    uint16_t	u16Priority;
    uint16_t	u16Language;
    uint32_t	u32InitialFrames;
    uint32_t	u32Scale;	
    uint32_t	u32Rate;	/* dwRate / dwScale == samples/second */
    uint32_t	u32Start;
    uint32_t	u32Length; /* In units above... */
    uint32_t	u32SuggestedBufferSize;
    uint32_t	u32Quality;
    uint32_t	u32SampleSize;
    //RECT		rcFrame;
    uint16_t	u16Left_X;		//Left top
    uint16_t	u16Top_Y;
    uint16_t	u16Right_X;
    uint16_t	u16Bottom_Y;
} S_AVIFMT_AVISTREAMHEADER;
// AVI Stream header
 
/* Flags for index */
#define AVIIF_LIST          0x00000001L // chunk is a 'LIST'
#define AVIIF_KEYFRAME      0x00000010L // this frame is a key frame.

#define AVIIF_NOTIME	    0x00000100L // this frame doesn't take any time
#define AVIIF_COMPUSE       0x0FFF0000L // these bits are for compressor use

typedef struct
{
    uint32_t		ckid;
    uint32_t		dwFlags;
    uint32_t		dwChunkOffset;		// Position of chunk
    uint32_t		dwChunkLength;		// Length of chunk
} S_AVIFMT_AVIINDEXENTRY;

typedef struct{
	int32_t	i32Width;
	int32_t	i32Height;
	uint32_t	u32FrameRate;
	uint32_t	u32TotalFrames;
	uint32_t	u32MaxBytesPerSec;
	//
	uint32_t	u32SizeRIFF;
	uint32_t	u32SizeHDRL;
	uint32_t	u32SizeJUNK;
	uint32_t	u32SizeMOVI;
	uint32_t	u32Streams;
	//
	uint32_t	u32SuggestedBufferSize;
	// for idx1
	uint32_t   u32posIndexEntry;
	uint32_t   u32arrCounter;
//	uint32_t   u32flag_arrJPEGSizes;
	uint32_t   u32PosMOVI;
//	uint32_t   *pu32arrJPEGSizes;
	S_AVIFMT_AVIINDEXENTRY *psaAVIdxChunkInfo;
	uint32_t   u32SamplingRate;
	uint32_t	 u32Channel;
	uint32_t	u32VideoFcc;
	uint32_t	u32AudioFmtTag;
	uint32_t	u32AudioFrameSize; //Encoded audio frame size
	uint32_t	u32AudioSamplesPerFrame; //Encoded audio samples per frame
	uint32_t	u32AudioBitRate; //Encoded audio samples per frame

	//for playback
	uint32_t	u32AVICurrentPos;
	uint32_t	u32LISTChunkSize;
	uint16_t	u16BitsPerSample;    /* Number of bits per sample of mono data */

} S_AVIFMT_AVI_CONTENT;

typedef struct ctagBITMAPINFOHEADER {
    uint32_t  biSize;
    LONG   biWidth;
    LONG   biHeight;
    uint16_t   biPlanes;
    uint16_t   biBitCount;
    uint32_t  biCompression;
    uint32_t  biSizeImage;
    LONG   biXPelsPerMeter;
    LONG   biYPelsPerMeter;
    uint32_t  biClrUsed;
    uint32_t  biClrImportant;
    uint32_t  reserverd[7];
} S_AVIFMT_BITMAPINFOHEADER;

// wav format header
typedef struct
{
    uint16_t   u16FormatTag;        /* format type */
    uint16_t   u16Channels;         /* number of channels (i.e. mono, stereo...) */
    uint32_t   u32SamplesPerSec;    /* sample rate */
    uint32_t   u32AvgBytesPerSec;   /* for buffer estimation */
    uint16_t   u16BlockAlign;       /* block size of data */
    uint16_t   u16BitsPerSample;    /* Number of bits per sample of mono data */
    uint16_t   u16cbSize;            /* The count in bytes of the size of
                                    extra information (after cbSize) */
//    uint16_t	 Reserved[2];                                	                                    
} __attribute__ ((packed)) S_AVIFMT_WAVEFORMATEX;
// wav format header
 
typedef struct
{
	S_AVIFMT_WAVEFORMATEX sWFX;
	uint16_t u16WId;
	uint32_t u32Flags;
	uint16_t u16BlockSize;
	uint16_t u16FramesPerBlock;
	uint16_t u16CodecDelay;
} __attribute__ ((packed)) S_AVIFMT_MP3WAVEFORMAT;

typedef struct
{
	S_AVIFMT_WAVEFORMATEX sWFX;
	uint16_t u16SamplePerBlock;
} __attribute__ ((packed)) S_AVIFMT_IMAADPCMWAVEFORMAT;

typedef struct {
	S_AVIFMT_WAVEFORMATEX sWFX;
	uint8_t u8Profile;
	uint8_t u8SRI;
	uint8_t u8SyncExtType_H;
	uint8_t u8SyncExtType_L;
	uint8_t u8DSRI;
	uint8_t u8Reseved;
} __attribute__((packed)) S_AVIFMT_AACWAVEFORMAT;


#endif


