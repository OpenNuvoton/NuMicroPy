//      Copyright (c) 2016 Nuvoton Technology Corp. All rights reserved.
//
//      This program is free software; you can redistribute it and/or modify
//      it under the terms of the GNU General Public License as published by
//      the Free Software Foundation; either m_uiVersion 2 of the License, or
//      (at your option) any later m_uiVersion.
//
//      This program is distributed in the hope that it will be useful,
//      but WITHOUT ANY WARRANTY; without even the implied warranty of
//      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//      GNU General Public License for more details.
//
//      You should have received a copy of the GNU General Public License
//      along with this program; if not, write to the Free Software
//      Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
//      MA 02110-1301, USA.

#ifndef __REC_FILE_MGR_H__
#define __REC_FILE_MGR_H__

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <stdint.h>

#ifndef ERRCODE
    #define ERRCODE int32_t
#endif

#define FILEMGR_ERR                     0x80010000

#define ERR_FILEMGR_NONE                    0
#define ERR_FILEMGR_MALLOC              (FILEMGR_ERR | 0x01)
#define ERR_FILEMGR_WORK_FOLDER         (FILEMGR_ERR | 0x02)
#define ERR_FILEMGR_NULL_PTR            (FILEMGR_ERR | 0x04)
#define ERR_FILEMGR_NO_SPACE            (FILEMGR_ERR | 0x05)


// Media type
typedef enum
{
    eFILEMGR_TYPE_MP3,
    eFILEMGR_TYPE_AAC,
    eFILEMGR_TYPE_AUDIO_ONLY = eFILEMGR_TYPE_AAC,
    eFILEMGR_TYPE_JPEG,
    eFILEMGR_TYPE_VIDEO_ONLY = eFILEMGR_TYPE_JPEG,
    eFILEMGR_TYPE_AVI,
    eFILEMGR_TYPE_MP4,
    eFILEMGR_TYPE_END
} E_FILEMGR_TYPE;

int32_t
FileMgr_InitFileSystem(
    char **pszDiskVolume
);

//Create new file. Return <0: faile. 0:success . If success, it will return file name string in pchNewFileName.
//User must free pchNewFileName in somewhere.
int32_t
FileMgr_CreateNewFileName(
    E_FILEMGR_TYPE eFileType,
    char *pchSaveFolder,
    uint32_t u32FNPostOffset,
    char **pchNewFileName,
    int32_t i32InstIdx,
    uint32_t u32AllowFreeStorSize
);

//Close file
void
FileMgr_CloseFile(
    E_FILEMGR_TYPE eFileType,
    char *pchFileName,
    int32_t i32CloseFD,
    bool bUnlink
);

void
FileMgr_FreeAllFileList(void);

void
FileMgr_AppendFileInfo(
    E_FILEMGR_TYPE eFileType,
    char *pchFileName
);

void
FileMgr_UpdateFileInfo(
    E_FILEMGR_TYPE eFileType,
    char *pchFileName
);


#endif
