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

#include "NVTMedia_SAL_OS.h"

int g_NM_i32LogLevel = DEF_NM_MSG_INFO;

void
NMUtil_SetLogLevel(
	int i32LogLevel
)
{
	g_NM_i32LogLevel = i32LogLevel;
}

void
NMUtil_ParseH264Frame(
	uint8_t *pu8BitStream,
	uint32_t u32BitStreamLen,
	S_NM_UTIL_H264_FRAME_INFO *psFrameInfo
)
{
	uint32_t u32NextSyncWordAdr = 0;
	uint32_t u32StartCodeLen;
	uint32_t u32StartCode4Byte = 0x01000000;
	uint32_t u32StartCode3Byte = 0x010000;

	memset(psFrameInfo, 0x00, sizeof(S_NM_UTIL_H264_FRAME_INFO));

	if (memcmp(pu8BitStream, &u32StartCode4Byte, 4) == 0) {
		u32StartCodeLen = 4;
	}
	else if (memcmp(pu8BitStream, &u32StartCode3Byte, 3) == 0) {
		u32StartCodeLen = 3;
	}
	else {
		psFrameInfo->eNALType = eNM_UTIL_H264_NAL_UNKNOWN;
		return;
	}

	if ((pu8BitStream[u32StartCodeLen] & 0x1F) == 0x05) {
		// It is I-frame
		psFrameInfo->u32IPStartCodeLen = u32StartCodeLen;
		psFrameInfo->u32IPLen = u32BitStreamLen;
		psFrameInfo->eNALType = eNM_UTIL_H264_NAL_I;
		return;
	}
	else if ((pu8BitStream[u32StartCodeLen] & 0x1F) == 0x01) {
		// It is P-frame
		psFrameInfo->u32IPStartCodeLen = u32StartCodeLen;
		psFrameInfo->u32IPLen = u32BitStreamLen;
		psFrameInfo->eNALType = eNM_UTIL_H264_NAL_P;
		return;
	}
	else {
		// Search I/P frame
		uint32_t i32Len = (u32BitStreamLen - u32StartCodeLen); //4:sync word len
		uint8_t u8FrameNAL;
		E_NM_UTIL_H264_NALTYPE_MASK eCurFrameNAL = eNM_UTIL_H264_NAL_UNKNOWN;

		while (i32Len > 0) {

			if (memcmp(pu8BitStream + u32NextSyncWordAdr, &u32StartCode4Byte, 4) == 0) {
				u32StartCodeLen = 4;
			}
			else if (memcmp(pu8BitStream + u32NextSyncWordAdr, &u32StartCode3Byte, 3) == 0) {
				u32StartCodeLen = 3;
			}
			else {
				u32StartCodeLen = 0;
			}

			if (u32StartCodeLen) {
				u8FrameNAL = pu8BitStream[u32NextSyncWordAdr + u32StartCodeLen] & 0x1F;

				if (eCurFrameNAL == eNM_UTIL_H264_NAL_SPS) {
					psFrameInfo->u32SPSLen = u32NextSyncWordAdr - psFrameInfo->u32SPSOffset;
				}
				else if (eCurFrameNAL == eNM_UTIL_H264_NAL_PPS) {
					psFrameInfo->u32PPSLen = u32NextSyncWordAdr - psFrameInfo->u32PPSOffset;
				}
				else if (eCurFrameNAL == eNM_UTIL_H264_NAL_AUD) {
					psFrameInfo->u32AUDLen = u32NextSyncWordAdr - psFrameInfo->u32AUDOffset;
				}
				else if (eCurFrameNAL == eNM_UTIL_H264_NAL_SEI) {
					psFrameInfo->u32SEILen = u32NextSyncWordAdr - psFrameInfo->u32SEIOffset;
				}

				if (u8FrameNAL == 0x07) {
					eCurFrameNAL = eNM_UTIL_H264_NAL_SPS;
					psFrameInfo->u32SPSStartCodeLen = u32StartCodeLen;
					psFrameInfo->u32SPSOffset = u32NextSyncWordAdr;
					psFrameInfo->eNALType |= eNM_UTIL_H264_NAL_SPS;
				}
				else if (u8FrameNAL == 0x08) {
					eCurFrameNAL = eNM_UTIL_H264_NAL_PPS;
					psFrameInfo->u32PPSStartCodeLen = u32StartCodeLen;
					psFrameInfo->u32PPSOffset = u32NextSyncWordAdr;
					psFrameInfo->eNALType |= eNM_UTIL_H264_NAL_PPS;
				}
				else if (u8FrameNAL == 0x09) {
					eCurFrameNAL = eNM_UTIL_H264_NAL_AUD;
					psFrameInfo->u32AUDStartCodeLen = u32StartCodeLen;
					psFrameInfo->u32AUDOffset = u32NextSyncWordAdr;
					psFrameInfo->eNALType |= eNM_UTIL_H264_NAL_AUD;
				}
				else if (u8FrameNAL == 0x06) {
					uint8_t u8PayloadSize;
					eCurFrameNAL = eNM_UTIL_H264_NAL_SEI;
					psFrameInfo->u32SEIStartCodeLen = u32StartCodeLen;
					psFrameInfo->u32SEIOffset = u32NextSyncWordAdr;
					psFrameInfo->eNALType |= eNM_UTIL_H264_NAL_SEI;
					u8PayloadSize = pu8BitStream[u32NextSyncWordAdr + u32StartCodeLen + 2];
					u32NextSyncWordAdr += (u8PayloadSize + u32StartCodeLen + 3);
					i32Len -= (u8PayloadSize + u32StartCodeLen + 3);
					continue;
				}
				else if (u8FrameNAL == 0x05) {
					eCurFrameNAL = eNM_UTIL_H264_NAL_I;
					psFrameInfo->u32IPStartCodeLen = u32StartCodeLen;
					psFrameInfo->u32IPOffset = u32NextSyncWordAdr;
					psFrameInfo->u32IPLen = u32BitStreamLen - u32NextSyncWordAdr;
					psFrameInfo->eNALType |= eNM_UTIL_H264_NAL_I;
					break;
				}
				else if (u8FrameNAL == 0x01) {
					eCurFrameNAL = eNM_UTIL_H264_NAL_P;
					psFrameInfo->u32IPStartCodeLen = u32StartCodeLen;
					psFrameInfo->u32IPOffset = u32NextSyncWordAdr;
					psFrameInfo->u32IPLen = u32BitStreamLen - u32NextSyncWordAdr;
					psFrameInfo->eNALType |= eNM_UTIL_H264_NAL_P;
					break;
				}

				u32NextSyncWordAdr += u32StartCodeLen;
				i32Len -= u32StartCodeLen;
			}
			else {
				u32NextSyncWordAdr ++;
				i32Len --;
			}
		}

		if (eCurFrameNAL == eNM_UTIL_H264_NAL_SPS) {
			psFrameInfo->u32SPSLen = u32BitStreamLen - psFrameInfo->u32SPSOffset;
		}
		else if (eCurFrameNAL == eNM_UTIL_H264_NAL_PPS) {
			psFrameInfo->u32PPSLen = u32BitStreamLen - psFrameInfo->u32PPSOffset;
		}
		else if (eCurFrameNAL == eNM_UTIL_H264_NAL_AUD) {
			psFrameInfo->u32AUDLen = u32BitStreamLen - psFrameInfo->u32AUDOffset;
		}
		else if (eCurFrameNAL == eNM_UTIL_H264_NAL_SEI) {
			psFrameInfo->u32SEILen = u32BitStreamLen - psFrameInfo->u32SEIOffset;
		}

		if (psFrameInfo->eNALType == eNM_UTIL_H264_NAL_UNKNOWN)
			NMLOG_ERROR("utils_ParseH264Frame: Unknown H264 frame \n");
	}

	return;
}

char* 
NMUtil_strdup(
	const char* szStr
)
{
	uint32_t u32Len;
	char* szCopy;

	u32Len = strlen(szStr) + 1;
	szCopy = (char*)malloc(u32Len);

	if(!szCopy)
		return NULL;

	memcpy(szCopy,szStr,u32Len);
	return szCopy;
}

uint16_t
NMUtil_GCD(
	uint16_t m1,
	uint16_t m2
)
{
	uint16_t m;

	if (m1 < m2) {
		m = m1;
		m1 = m2;
		m2 = m;
	}

	if (m1 % m2 == 0)
		return m2;
	else
		return (NMUtil_GCD(m2, m1 % m2));
}


#define  JPEG_M_SOF0   0xc0
#define  JPEG_M_SOF1   0xc1
#define  JPEG_M_SOF2   0xc2
#define  JPEG_M_SOF3   0xc3

#define  JPEG_M_SOF5   0xc5
#define  JPEG_M_SOF6   0xc6
#define  JPEG_M_SOF7   0xc7

#define  JPEG_M_JPG    0xc8
#define  JPEG_M_SOF9   0xc9
#define  JPEG_M_SOF10  0xca
#define  JPEG_M_SOF11  0xcb

#define  JPEG_M_SOF13  0xcd
#define  JPEG_M_SOF14  0xce
#define  JPEG_M_SOF15  0xcf

#define  JPEG_M_DHT    0xc4

#define  JPEG_M_DAC    0xcc

#define  JPEG_M_RST0   0xd0
#define  JPEG_M_RST1   0xd1
#define  JPEG_M_RST2   0xd2
#define  JPEG_M_RST3   0xd3
#define  JPEG_M_RST4   0xd4
#define  JPEG_M_RST5   0xd5
#define  JPEG_M_RST6   0xd6
#define  JPEG_M_RST7   0xd7

#define  JPEG_M_SOI    0xd8
#define  JPEG_M_EOI    0xd9
#define  JPEG_M_SOS    0xda
#define  JPEG_M_DQT    0xdb
#define  JPEG_M_DNL    0xdc
#define  JPEG_M_DRI    0xdd
#define  JPEG_M_DHP    0xde
#define  JPEG_M_EXP    0xdf

#define  JPEG_M_APP0   0xe0
#define  JPEG_M_APP1   0xe1
#define  JPEG_M_APP2   0xe2
#define  JPEG_M_APP3   0xe3
#define  JPEG_M_APP4   0xe4
#define  JPEG_M_APP5   0xe5
#define  JPEG_M_APP6   0xe6
#define  JPEG_M_APP7   0xe7
#define  JPEG_M_APP8   0xe8
#define  JPEG_M_APP9   0xe9
#define  JPEG_M_APP10  0xea
#define  JPEG_M_APP11  0xeb
#define  JPEG_M_APP12  0xec
#define  JPEG_M_APP13  0xed
#define  JPEG_M_APP14  0xee
#define  JPEG_M_APP15  0xef

#define  JPEG_M_JPG0   0xf0
#define  JPEG_M_JPG13  0xfd
#define  JPEG_M_COM    0xfe

#define  JPEG_M_TEM    0x01


E_NM_ERRNO
NM_ParseJPEG(
	S_NM_VIDEO_CTX	*psVideoCtx,
	uint8_t *pu8Buf,
	uint32_t u32BufLen
)
{
	//HByte,LByte:For JPEG Marker decode
	//MLength:Length of Marker (all data in the marker)
	unsigned char HByte, LByte, SOS_found = 0, SOF_found = 0;
	unsigned short int MLength;
	int index;
	index = 0;

	while (index < u32BufLen) {
		HByte = pu8Buf[index++];

		if (index == 1 && (HByte != 0xFF || pu8Buf[index] != JPEG_M_SOI))
			return eNM_ERRNO_SIZE;

		if (HByte == 0xFF) {
			LByte = pu8Buf[index++];

			switch (LByte) {
			case JPEG_M_SOI:		// SOI Marker (Start Of Image)
				break;
			case JPEG_M_SOF2:		// SOF Marker (Progressive mode Start Of Frame)
			case JPEG_M_SOF3:
			case JPEG_M_SOF5:
			case JPEG_M_SOF6:
			case JPEG_M_SOF7:
			case JPEG_M_JPG:
			case JPEG_M_SOF9:
			case JPEG_M_SOF10:
			case JPEG_M_SOF11:
			case JPEG_M_SOF13:
			case JPEG_M_SOF14:
			case JPEG_M_SOF15:	// SOF Marker(Not Baseline Mode)
				return eNM_ERRNO_SIZE;
			case JPEG_M_DQT:	// DQT Marker (Define Quantization Table)

				if (index + 2 > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}

				HByte = pu8Buf[index++];
				LByte = pu8Buf[index++];
				MLength = (HByte << 8) + LByte - 2;

				if (MLength != 65) {	// Has more than 1 Quantization Table
					if (MLength % 65) {
						return eNM_ERRNO_SIZE;
					}

					index += MLength;			// Skip Data
				}
				else	// Maybe has 1 Quantization Table
					index += 64;

				if (index > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}

				break;
			case JPEG_M_SOF0:      // SOF Marker (Baseline mode Start Of Frame)
			case JPEG_M_SOF1:
				if (index + 7 > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}

				HByte = pu8Buf[index++];
				LByte = pu8Buf[index++];
				MLength = (HByte << 8) + LByte;
				index++;	// Sample precision
				HByte = pu8Buf[index++];
				LByte = pu8Buf[index++];
				psVideoCtx->u32Height = (HByte << 8) + LByte;
				HByte = pu8Buf[index++];
				LByte = pu8Buf[index++];
				psVideoCtx->u32Width = (HByte << 8) + LByte;
				SOF_found = 1;
				if (SOS_found)
					return eNM_ERRNO_NONE;
				break;
			case JPEG_M_SOS:      // Scan Header(Start Of Scan)

				if (index + 2 > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}

				HByte = pu8Buf[index++];
				LByte = pu8Buf[index++];
				MLength = (HByte << 8) + LByte;
				index += MLength - 2;		// Skip Scan Header Data
				SOS_found = 1;
				if (index > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}

				break;
			case JPEG_M_DHT:      // DHT Marker (Define Huffman Table)

				if (index + 2 > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}

				HByte = pu8Buf[index++];
				LByte = pu8Buf[index++];
				MLength = (HByte << 8) + LByte;
				index += MLength - 2;

				if (index > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}

				break;
			case JPEG_M_APP0:
			case JPEG_M_APP1:
			case JPEG_M_APP2:
			case JPEG_M_APP3:
			case JPEG_M_APP4:
			case JPEG_M_APP5:
			case JPEG_M_APP6:
			case JPEG_M_APP7:
			case JPEG_M_APP8:
			case JPEG_M_APP9:
			case JPEG_M_APP10:
			case JPEG_M_APP11:
			case JPEG_M_APP12:
			case JPEG_M_APP13:
			case JPEG_M_APP14:
			case JPEG_M_APP15:
			case JPEG_M_COM:		//  Application Marker && Comment

				if (index + 2 > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}

				HByte = pu8Buf[index++];
				LByte = pu8Buf[index++];
				MLength = (HByte << 8) + LByte;
				index += MLength - 2;		// Skip Application or Comment Data

				if (index > u32BufLen) {
					return eNM_ERRNO_SIZE;
				}
				break;
			}
		}
	}
	if(SOS_found && SOF_found)
		return eNM_ERRNO_NONE;
	return eNM_ERRNO_CODEC_TYPE;		//Wrong file format
}

uint64_t
NMUtil_GetTimeMilliSec(void)
{
	uint64_t u64MilliSec;
	struct timespec sTimeSpec;
	
	clock_gettime(0, &sTimeSpec);
	u64MilliSec = sTimeSpec.tv_sec * 1000 + (sTimeSpec.tv_nsec / 1000000);

	return u64MilliSec;  
}

 /* Unoptimised version of glibc memrchr().
  * This is considered fast enough, as only this compat
  * version of dirname() depends on it.
  */
 static const char *
 __memrchr(const char *str, int c, size_t n)
 {
     const char *end = str;
 
     end += n - 1; /* Go to the end of the string */
     while (end >= str)
     {
         if (c == *end)
         {
             return end;
         }
         else
         {
             end--;
         }
     }
     return NULL;
 }
 
 /* Modified version based on glibc-2.14.1 by Ulrich Drepper <drepper@akkadia.org>
  * This version is extended to handle both / and \ in path names.
  */
char *
NMUtil_dirname(char *path)
{
     static const char dot[] = ".";
     char *last_slash;
     char separator = '/';
 
     /* Find last '/'.  */
     last_slash = path != NULL ? strrchr(path, '/') : NULL;
     /* If NULL, check for \ instead ... might be Windows a path */
     if (!last_slash)
     {
         last_slash = path != NULL ? strrchr(path, '\\') : NULL;
         separator = last_slash ? '\\' : '/'; /* Change the separator if \ was found */
     }
 
     if (last_slash != NULL && last_slash != path && last_slash[1] == '\0')
     {
         /* Determine whether all remaining characters are slashes.  */
         char *runp;
 
         for (runp = last_slash; runp != path; --runp)
         {
             if (runp[-1] != separator)
             {
                 break;
             }
         }
 
         /* The '/' is the last character, we have to look further.  */
         if (runp != path)
         {
             last_slash = (char *) __memrchr(path, separator, runp - path);
         }
     }
 
     if (last_slash != NULL)
     {
         /* Determine whether all remaining characters are slashes.  */
         char *runp;
 
         for (runp = last_slash; runp != path; --runp)
         {
             if (runp[-1] != separator)
             {
                 break;
             }
         }
 
         /* Terminate the path.  */
         if (runp == path)
         {
             /* The last slash is the first character in the string.  We have to
              * return "/".  As a special case we have to return "//" if there
              * are exactly two slashes at the beginning of the string.  See
              * XBD 4.10 Path Name Resolution for more information.  */
             if (last_slash == path + 1)
             {
                 ++last_slash;
             }
             else
             {
                 last_slash = path + 1;
             }
         }
         else
         {
             last_slash = runp;
         }
 
         last_slash[0] = '\0';
     }
     else
     {
         /* This assignment is ill-designed but the XPG specs require to
          * return a string containing "." in any case no directory part is
          * found and so a static and constant string is required.  */
         path = (char *) dot;
     }
 
     return path;
}



char *
NMUtil_basename(
	char *name
)
{
	char *base = name;
	
	while(*name){
		if(*name++ == '\\')
		{
			base = name;
		}
	}
	return (char *) base;
}

