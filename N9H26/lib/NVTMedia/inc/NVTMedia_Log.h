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

#ifndef __NVTMEDIA_LOG_H__
#define __NVTMEDIA_LOG_H__

extern void	sysprintf (char *pcStr,...);
extern int g_NM_i32LogLevel;

#define printf sysprintf

#define DEF_NM_MSG_NONE		0
#define DEF_NM_MSG_ERROR		1
#define DEF_NM_MSG_INFO		2
#define DEF_NM_MSG_WARNING		3
#define DEF_NM_MSG_DEBUG		4
#define DEF_NM_MSG_ALL			5

#define NMLOG_ENTER()			{if (g_NM_i32LogLevel >= DEF_NM_MSG_ALL ){printf("NM[%-20s] : Enter...\n", __FUNCTION__); }}
#define NMLOG_LEAVE()			{if (g_NM_i32LogLevel >= DEF_NM_MSG_ALL ){printf("NM[%-20s] : Leave...\n", __FUNCTION__); }}

#define NMLOG_WARNING(...)	{if (g_NM_i32LogLevel >= DEF_NM_MSG_WARNING ){printf("NM WARNING(%s(), %d): ", __FUNCTION__, __LINE__); sysprintf(__VA_ARGS__); }}

#define NMLOG_ERROR(...)		{if (g_NM_i32LogLevel >= DEF_NM_MSG_ERROR ){printf("NM ERROR(%s(), %d): ", __FUNCTION__, __LINE__); sysprintf(__VA_ARGS__); }}

#define NMLOG_DEBUG(...)		{if (g_NM_i32LogLevel >= DEF_NM_MSG_DEBUG ){ printf("NM DEBUG(%s(), %d): ", __FUNCTION__, __LINE__); sysprintf(__VA_ARGS__); }}

#define NMLOG_INFO(...)		{if (g_NM_i32LogLevel >= DEF_NM_MSG_INFO ){printf("NM INFO(%s(), %d): ", __FUNCTION__, __LINE__); sysprintf(__VA_ARGS__); }}

#endif
