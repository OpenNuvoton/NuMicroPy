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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (__GNUC__)
    #undef _TIME_T_DECLARED
#endif

#include <time.h>
#if defined (__GNUC__) && !(__CC_ARM)
    #include <strings.h>
    #include <inttypes.h>
#endif

#include "NVTMedia.h"
#include "NVTMedia_SAL_FS.h"

#include "Record_FileMgr.h"

#define DEFAULT_DV_FN_PREFIX    "NVTDV"
#define DEFAULT_DSC_FN_PREFIX   "NVTDSC"
#define DEFAULT_AUD_FN_PREFIX   "NVTAUD"

#define DEFAULT_FN_INST_IDX     "_P0_"
#define DEFAULT_INST_IDX_POS    2

#define DEFAULT_RESERVE_SIZE    (200*1024*1024)

#define JPEG_EXTENSION_NAME     ".jpg"
#define AVI_EXTENSION_NAME      ".avi"
#define MP4_EXTENSION_NAME      ".mp4"
#define AAC_EXTENSION_NAME      ".aac"
#define MP3_EXTENSION_NAME      ".mp3"


typedef struct s_media_file_ent
{
    struct s_media_file_ent *psNextFile;
    uint64_t u64PostIdx;
    uint32_t u32FileSize;
    uint32_t u32InstIdx;
} S_MEDIA_FILE_ENT;

typedef struct
{
    S_MEDIA_FILE_ENT *psListHead;
    S_MEDIA_FILE_ENT *psListTail;
    uint32_t u32LatestFileSize;
    uint32_t u32FreeStorSize;
} S_MEDIA_FILE_LIST;

typedef struct
{
    S_MEDIA_FILE_LIST *psFileList;
    char *szPrefixName;
    char *szExtName;
    char *szInstIdxName;
    uint32_t u32InstIdxPos;
    char *pchWorkingFolder;
} S_MEDIA_FILE_TABLE;

//static char *s_pchWorkingFolder = NULL;

static S_MEDIA_FILE_LIST s_sAVIFileList = {NULL, NULL, 0, DEFAULT_RESERVE_SIZE};
static S_MEDIA_FILE_LIST s_sJPEGFileList = {NULL, NULL, 0, DEFAULT_RESERVE_SIZE};
static S_MEDIA_FILE_LIST s_sMP4FileList = {NULL, NULL, 0, DEFAULT_RESERVE_SIZE};
static S_MEDIA_FILE_LIST s_sAACFileList = {NULL, NULL, 0, DEFAULT_RESERVE_SIZE};
static S_MEDIA_FILE_LIST s_sMP3FileList = {NULL, NULL, 0, DEFAULT_RESERVE_SIZE};

S_MEDIA_FILE_TABLE s_asMediaFileTable[eFILEMGR_TYPE_END] =
{
    {&s_sMP3FileList, DEFAULT_AUD_FN_PREFIX, MP3_EXTENSION_NAME, DEFAULT_FN_INST_IDX, DEFAULT_INST_IDX_POS},
    {&s_sAACFileList, DEFAULT_AUD_FN_PREFIX, AAC_EXTENSION_NAME, DEFAULT_FN_INST_IDX, DEFAULT_INST_IDX_POS},
    {&s_sJPEGFileList, DEFAULT_DSC_FN_PREFIX, JPEG_EXTENSION_NAME, DEFAULT_FN_INST_IDX, DEFAULT_INST_IDX_POS},
    {&s_sAVIFileList, DEFAULT_DV_FN_PREFIX, AVI_EXTENSION_NAME, DEFAULT_FN_INST_IDX, DEFAULT_INST_IDX_POS},
    {&s_sMP4FileList, DEFAULT_DV_FN_PREFIX, MP4_EXTENSION_NAME, DEFAULT_FN_INST_IDX, DEFAULT_INST_IDX_POS}
};


#ifndef strdup

static char *strdup(
    const char *src
)
{
    int len;
    char *tmp = NULL;

    len = strlen(src);
    tmp = (char *)malloc(len + 1);

    if (!tmp)
    {
        NMLOG_ERROR("Malloc failed!\n");
        return NULL;
    }

    strcpy(tmp, src);
    return tmp;
}

#endif

#ifndef basename

static char *
basename(
    char *name
)
{
    char *base = name;

    while (*name)
    {
        if (*name++ == '\\')
        {
            base = name;
        }
    }
    return (char *) base;
}

#endif

#if defined (__GNUC__) && !(__CC_ARM)
static char *ulltoStr(uint64_t val)
{
    static char buf[34] = { [0 ... 33] = 0 };
    char *out = &buf[32];
    uint64_t hval = val;
    unsigned int hbase = 10;

    do
    {
        *out = "0123456789abcdef"[hval % hbase];
        --out;
        hval /= hbase;
    }
    while (hval);

//    *out-- = 'x', *out = '0';
//   *out = '0';
    out ++;
    return out;
}
#endif

static void GetFNPostIdx(
    uint64_t *pu64PostIdx,
    uint32_t u32FNPostOffset
)
{
    struct tm *psCurTm;
    uint32_t u32Year;
    uint64_t u64PostIdx = 0;

    uint32_t u32CurTime = (uint32_t)(NMUtil_GetTimeMilliSec() / 1000);

    u32CurTime += u32FNPostOffset;

#if defined (__GNUC__) && !(__CC_ARM)
    psCurTm = localtime((const time_t *)&u32CurTime);
#else
    psCurTm = localtime(&u32CurTime);
#endif
    u32Year = psCurTm->tm_year + 1900;          //since 1900

    if (u32Year < 2000)
        u64PostIdx += (u32Year - 1900) * 10000000000LL;
    else
        u64PostIdx += (u32Year - 2000) * 10000000000LL;

    u64PostIdx += (psCurTm->tm_mon + 1) * 100000000LL;
    u64PostIdx += (psCurTm->tm_mday) * 1000000LL;
    u64PostIdx += (psCurTm->tm_hour) * 10000LL;
    u64PostIdx += (psCurTm->tm_min) * 100LL;
    u64PostIdx += (psCurTm->tm_sec) * 1LL;

    *pu64PostIdx = u64PostIdx;
}

static void GetSeqFNPostIdx(
    S_MEDIA_FILE_LIST *psFileList,
    uint64_t *pu64PostIdx
)
{
    if (psFileList->psListTail)
    {
        *pu64PostIdx = psFileList->psListTail->u64PostIdx + 1;
    }
    else
    {
        *pu64PostIdx = 1;
    }
}


static void
SortAndLinkExistFile(
    char *pchFolder,
    char *pchFileName,
    char *pchFilePrefixStr,
    char *pchFileExtStr,
    char *pchInstIdxStr,
    uint32_t u32InstIdxPos,
    S_MEDIA_FILE_LIST *psFileList
)
{
    char *pchFileFullPath = NULL;
    uint64_t u64PostIdx;
    char *pchEndPos;
    char *pchPostPos;
	struct stat sFileState;
    S_MEDIA_FILE_ENT *psFileEnt;
    S_MEDIA_FILE_ENT *psCurFileEnt;
    S_MEDIA_FILE_ENT *psPrevFileEnt;
    uint32_t u32InstIdx;
    char *pchInstIdxPos;

    pchInstIdxPos = pchFileName + strlen(pchFilePrefixStr);
    pchPostPos = pchFileName + strlen(pchFilePrefixStr) + strlen(pchInstIdxStr);
    u64PostIdx = strtoull(pchPostPos, &pchEndPos, 10);

    if ((*pchEndPos != '.') || (pchEndPos - pchPostPos != 12)) // File name postfix total 12 digit
        return;

    u32InstIdx = pchInstIdxPos[u32InstIdxPos] - '0';

    if (u32InstIdx >= 10)
        return;

    if (strcasecmp(pchEndPos, pchFileExtStr) != 0) //check extersion file name
        return;

    psFileEnt = malloc(sizeof(S_MEDIA_FILE_ENT));

    if (psFileEnt == NULL)
        return;

    pchFileFullPath = malloc(strlen(pchFolder) + strlen(pchFileName) + 50);

    if (pchFileFullPath == NULL)
    {
        free(psFileEnt);
        return;
    }

    sprintf(pchFileFullPath, "%s\\%s", pchFolder, pchFileName);

	if(stat(pchFileFullPath, &sFileState) < 0){
        free(psFileEnt);
        free(pchFileFullPath);
        return;
	}
    psFileEnt->u32FileSize = sFileState.st_size;	
    free(pchFileFullPath);

    psFileEnt->psNextFile = NULL;
    psFileEnt->u64PostIdx = u64PostIdx;
    psFileEnt->u32InstIdx = u32InstIdx;

    NMLOG_INFO("SortAndLinkExistFile add existed file name %s and size %d \n", pchFileName, psFileEnt->u32FileSize);

    if (psFileList->psListHead == NULL)
    {
        psFileList->psListHead = psFileEnt;
        psFileList->psListTail = psFileEnt;
        return;
    }

    psPrevFileEnt = NULL;
    psCurFileEnt = psFileList->psListHead;

    while (psCurFileEnt)
    {
        if (psCurFileEnt->u64PostIdx > u64PostIdx)
        {
            break;
        }

        psPrevFileEnt = psCurFileEnt;
        psCurFileEnt = psCurFileEnt->psNextFile;
    }

    if (psCurFileEnt == NULL)
    {
        psFileList->psListTail = psFileEnt;
    }

    if (psCurFileEnt == psFileList->psListHead)
    {
        psFileEnt->psNextFile =  psFileList->psListHead;
        psFileList->psListHead = psFileEnt;
    }
    else
    {
        psFileEnt->psNextFile = psCurFileEnt;
        psPrevFileEnt->psNextFile = psFileEnt;
    }
}


static ERRCODE
SearchExistFile(
    E_FILEMGR_TYPE eFileType,
    char *pchFolder
)
{
    char *pchFilePrefixStr = NULL;
    char *pchFileExtStr = NULL;
    char *pchInstIdxStr = NULL;
    S_MEDIA_FILE_LIST *psFileList = NULL;
	DIR *psDir = NULL;
    uint32_t u32InstIdxPos;

    psFileList = s_asMediaFileTable[eFileType].psFileList;
    pchFilePrefixStr = s_asMediaFileTable[eFileType].szPrefixName;
    pchFileExtStr = s_asMediaFileTable[eFileType].szExtName;
    pchInstIdxStr = s_asMediaFileTable[eFileType].szInstIdxName;
    u32InstIdxPos = s_asMediaFileTable[eFileType].u32InstIdxPos;

	//Open direct
	psDir = opendir(pchFolder);
	if(psDir == NULL)
        return ERR_FILEMGR_WORK_FOLDER;

    struct dirent *psDirEnt = NULL;

	while(1){
		psDirEnt = readdir(psDir);

		if(psDirEnt == NULL)
			break;

        if (strncmp(psDirEnt->d_name, pchFilePrefixStr, strlen(pchFilePrefixStr)) == 0){
            SortAndLinkExistFile(pchFolder, psDirEnt->d_name, pchFilePrefixStr, pchFileExtStr, pchInstIdxStr, u32InstIdxPos,
                                 psFileList);
        }
	}

	closedir(psDir);

    return ERR_FILEMGR_NONE;
}


static ERRCODE
CheckAndFreeStoargeSpace(
    E_FILEMGR_TYPE eFileType,
    char *pchFolder,
    uint32_t u32StroSize
)
{
    S_MEDIA_FILE_LIST *psFileList = NULL;
    uint32_t u32NeededSize;
    uint64_t u64DiskAvailSpace;
    uint64_t u64AvailBytes;

    S_MEDIA_FILE_ENT *psCurFileEnt;
    S_MEDIA_FILE_ENT *psRmFileEnt;
    S_MEDIA_FILE_ENT *psTempFileEnt;

    char *pchFilePrefixStr = NULL;
    char *pchFileExtStr = NULL;
    char *pchFileFullPath = NULL;
    char *pchInstIdxStr = NULL;
    uint32_t u32InstIdxPos;

    if (pchFolder == NULL)
        return ERR_FILEMGR_NULL_PTR;

    psFileList = s_asMediaFileTable[eFileType].psFileList;

    if (psFileList->psListHead == NULL)
    {
        //Try to search existed file and build file list
        SearchExistFile(eFileType, pchFolder);
    }

    psFileList->u32FreeStorSize = u32StroSize;
    u32NeededSize = psFileList->u32FreeStorSize;

	u64DiskAvailSpace = disk_free_space(pchFolder);
	printf("Free stoage space %d(B) \n", (uint32_t) u64DiskAvailSpace);

    if (u64DiskAvailSpace > (uint64_t)u32NeededSize)
        return ERR_FILEMGR_NONE;

    pchFilePrefixStr = s_asMediaFileTable[eFileType].szPrefixName;
    pchFileExtStr = s_asMediaFileTable[eFileType].szExtName;
    pchInstIdxStr = strdup(s_asMediaFileTable[eFileType].szInstIdxName);

    if (pchInstIdxStr == NULL)
        return ERR_FILEMGR_MALLOC;

    u32InstIdxPos = s_asMediaFileTable[eFileType].u32InstIdxPos;

    pchFileFullPath = malloc(strlen(pchFolder) + strlen(pchFilePrefixStr) + strlen(pchInstIdxStr) + strlen(
                                 pchFileExtStr) + 50);

    if (pchFileFullPath == NULL)
    {
        free(pchInstIdxStr);
        return ERR_FILEMGR_MALLOC;
    }

    //Calculate can be released storage space.
    u64AvailBytes = u64DiskAvailSpace;
    psCurFileEnt = psFileList->psListHead;

    while (psCurFileEnt)
    {
        u64AvailBytes += (uint64_t)(psCurFileEnt->u32FileSize);

        if (u64AvailBytes >= (uint64_t)(u32NeededSize))
        {
            psCurFileEnt = psCurFileEnt->psNextFile;
            break;
        }

        psCurFileEnt = psCurFileEnt->psNextFile;
    }

    if (u64AvailBytes < (uint64_t)(u32NeededSize))
    {
        free(pchInstIdxStr);
        free(pchFileFullPath);
        return ERR_FILEMGR_NO_SPACE;
    }

    // remove old file
    psRmFileEnt = psFileList->psListHead;

    while ((psRmFileEnt) && (psRmFileEnt != psCurFileEnt))
    {
        pchInstIdxStr[u32InstIdxPos] = '0' + psRmFileEnt->u32InstIdx;
#if defined (__GNUC__) && !(__CC_ARM)
        sprintf(pchFileFullPath, "%s/%s%s%012s%s", pchFolder, pchFilePrefixStr, pchInstIdxStr, ulltoStr(psRmFileEnt->u64PostIdx), pchFileExtStr);
#else
        sprintf(pchFileFullPath, "%s\\%s%s%012"PRId64"%s",
                pchFolder, pchFilePrefixStr, pchInstIdxStr, psRmFileEnt->u64PostIdx, pchFileExtStr);
#endif

		unlink(pchFileFullPath);
        NMLOG_INFO("Recycle file : %s \n ", pchFileFullPath);

        //Update file list
        psTempFileEnt = psRmFileEnt;

        if (psRmFileEnt == psFileList->psListHead)
        {
            psFileList->psListHead = psRmFileEnt->psNextFile;
        }

        if (psRmFileEnt == psFileList->psListTail)
        {
            psFileList->psListTail = psRmFileEnt->psNextFile;
        }

        psRmFileEnt = psRmFileEnt->psNextFile;

        if (psTempFileEnt)
            free(psTempFileEnt);
    }

    free(pchInstIdxStr);
    free(pchFileFullPath);
    return ERR_FILEMGR_NONE;
}

static void
RemoveFileEnt(
    S_MEDIA_FILE_LIST *psFileList,
    S_MEDIA_FILE_ENT  *psRmFileEnt
)
{
    S_MEDIA_FILE_ENT  *psCurFileEnt = NULL;
    S_MEDIA_FILE_ENT  *psPrevFileEnt = NULL;

    psCurFileEnt = psFileList->psListHead;

    while (psCurFileEnt)
    {
        if (psCurFileEnt == psRmFileEnt)
        {
            break;
        }
        psPrevFileEnt = psCurFileEnt;
        psCurFileEnt = psCurFileEnt->psNextFile;
    }

    if (psCurFileEnt)
    {
        if (psCurFileEnt == psFileList->psListHead)
        {
            psFileList->psListHead = psCurFileEnt->psNextFile;
        }

        if (psCurFileEnt == psFileList->psListTail)
        {
            psFileList->psListTail = psPrevFileEnt;
        }

        if (psPrevFileEnt)
            psPrevFileEnt->psNextFile = psCurFileEnt->psNextFile;

        free(psCurFileEnt);
    }
}


static S_MEDIA_FILE_ENT  *
UpdateFileEnt(
    char *pchFileName,
    char *pchFilePrefixStr,
    char *pchInstIdxStr,
    uint32_t u32InstIdxPos,
    S_MEDIA_FILE_LIST *psFileList
)
{
    char *pchEndPos;
    char *pchPostPos;
    uint64_t u64PostIdx;
	struct stat sFileState;

    char *pchFileBaseName;
    uint32_t u32InstIdx;
    char *pchInstIdxPos;
    S_MEDIA_FILE_ENT *psTempEnt;

    if ((psFileList == NULL) || (pchFileName == NULL) || (pchFilePrefixStr == NULL))
        return NULL;

    if (stat(pchFileName, &sFileState) < 0)
        return NULL;	

    NMLOG_INFO("New recorded file name %s, size %d \n", pchFileName, (uint32_t) sFileState.st_size);

    pchFileBaseName = basename(pchFileName);

    if (pchFileBaseName == NULL)
        return NULL;

    pchInstIdxPos = pchFileBaseName + strlen(pchFilePrefixStr);
    pchPostPos = pchFileBaseName + strlen(pchFilePrefixStr) + strlen(pchInstIdxStr);
    u64PostIdx = strtoull(pchPostPos, &pchEndPos, 10);

    if ((*pchEndPos != '.') || (pchEndPos - pchPostPos != 12)) // File name postfix total 12 digit
        return NULL;

    u32InstIdx = pchInstIdxPos[u32InstIdxPos] - '0';

    if (u32InstIdx >= 10)
        return NULL;


    psTempEnt = psFileList->psListHead;

    while (psTempEnt)
    {
        if ((psTempEnt->u64PostIdx == u64PostIdx) && (psTempEnt->u32InstIdx == u32InstIdx))
        {
            psTempEnt->u32FileSize = sFileState.st_size;

            if (psTempEnt->u32FileSize)
                psFileList->u32LatestFileSize = psTempEnt->u32FileSize;

            return psTempEnt;
        }

        psTempEnt = psTempEnt->psNextFile;
    }

    return NULL;
}


static void
AppendFileList(
    char *pchFileName,
    char *pchFilePrefixStr,
    char *pchInstIdxStr,
    uint32_t u32InstIdxPos,
    S_MEDIA_FILE_LIST *psFileList
)
{
    char *pchEndPos;
    char *pchPostPos;
    uint64_t u64PostIdx;
	struct stat sFileState;

    S_MEDIA_FILE_ENT  *psFileEnt;
    char *pchFileBaseName;
    uint32_t u32InstIdx;
    char *pchInstIdxPos;


    if ((psFileList == NULL) || (pchFileName == NULL) || (pchFilePrefixStr == NULL))
        return;

    if (stat(pchFileName, &sFileState) < 0)
        return;	

    pchFileBaseName = basename(pchFileName);

    if (pchFileBaseName == NULL)
        return;

    pchInstIdxPos = pchFileBaseName + strlen(pchFilePrefixStr);
    pchPostPos = pchFileBaseName + strlen(pchFilePrefixStr) + strlen(pchInstIdxStr);
    u64PostIdx = strtoull(pchPostPos, &pchEndPos, 10);

    if ((*pchEndPos != '.') || (pchEndPos - pchPostPos != 12)) // File name postfix total 12 digit
        return;

    u32InstIdx = pchInstIdxPos[u32InstIdxPos] - '0';

    if (u32InstIdx >= 10)
        return;

    psFileEnt = malloc(sizeof(S_MEDIA_FILE_ENT));

    if (psFileEnt == NULL)
        return;

    psFileEnt->u64PostIdx = u64PostIdx;
    psFileEnt->u32InstIdx = u32InstIdx;
    psFileEnt->u32FileSize = sFileState.st_size;
    psFileEnt->psNextFile = NULL;

    if (psFileEnt->u32FileSize)
        psFileList->u32LatestFileSize = psFileEnt->u32FileSize;

//  NMLOG_INFO("AppendFileList append file name %s, size %d \n", pchFileBaseName, psFileEnt->u32FileSize);

    // append file entity into file list
    if (psFileList->psListHead == NULL)
    {
        psFileList->psListHead = psFileEnt;
    }
    else
    {
        if (psFileList->psListTail == NULL)
        {
            NMLOG_DEBUG("%s", "121\n");

            while (1);
        }

        psFileList->psListTail->psNextFile = psFileEnt;
    }

    psFileList->psListTail = psFileEnt;
}

static void
FreeFileList(
    S_MEDIA_FILE_LIST *psFileList
)
{
    S_MEDIA_FILE_ENT *psFileEnt;

    if (psFileList == NULL)
        return;

    while (psFileList->psListHead)
    {
        psFileEnt = psFileList->psListHead;
        psFileList->psListHead = psFileEnt->psNextFile;
        free(psFileEnt);
    }
}

////////////////////////////////////////////////////////////////////////
// API implement
////////////////////////////////////////////////////////////////////////



int32_t
FileMgr_CreateNewFileName(
    E_FILEMGR_TYPE eFileType,
    char *pchSaveFolder,
    uint32_t u32FNPostOffset,
    char **pchNewFileName,
    int32_t i32InstIdx,
    uint32_t u32AllowFreeStorSize
)
{
    char *pchFilePath = NULL;
    uint64_t u64FNPostIdx;
    char *pchFilePrefixStr = NULL;
    char *pchFileExtStr = NULL;
    char *pchInstIdxStr = NULL;
    uint32_t u32InstIdxPos;
    S_MEDIA_FILE_LIST *psFileList;

    if (pchSaveFolder == NULL)
        return -1;

    if (pchNewFileName == NULL)
        return -2;

    *pchNewFileName = NULL;

    if (s_asMediaFileTable[eFileType].pchWorkingFolder == NULL)
    {
        s_asMediaFileTable[eFileType].pchWorkingFolder = strdup(pchSaveFolder);
    }

    if (strcmp(s_asMediaFileTable[eFileType].pchWorkingFolder, pchSaveFolder) != 0)
    {
        //Release all existed file list
        FileMgr_FreeAllFileList();
        s_asMediaFileTable[eFileType].pchWorkingFolder = strdup(pchSaveFolder);
    }

    pchFilePath = malloc(strlen(pchSaveFolder) + strlen(DEFAULT_DSC_FN_PREFIX) + 50);

    if (pchFilePath == NULL)
        return -3;

    ERRCODE  eRet;

    eRet = CheckAndFreeStoargeSpace(eFileType, s_asMediaFileTable[eFileType].pchWorkingFolder, u32AllowFreeStorSize);

    if (eRet != ERR_FILEMGR_NONE)
    {
        NMLOG_ERROR("FileMgr_CreateNewFile failed %x \n", eRet);
        return -4;
    }


    pchFilePrefixStr = s_asMediaFileTable[eFileType].szPrefixName;
    pchFileExtStr = s_asMediaFileTable[eFileType].szExtName;
    u32InstIdxPos = s_asMediaFileTable[eFileType].u32InstIdxPos;
    psFileList = s_asMediaFileTable[eFileType].psFileList;


//  GetFNPostIdx(&u64FNPostIdx, u32FNPostOffset);
    GetSeqFNPostIdx(psFileList, &u64FNPostIdx);

    pchInstIdxStr = strdup(s_asMediaFileTable[eFileType].szInstIdxName);

    if (pchInstIdxStr == NULL)
    {
        NMLOG_ERROR("strdup instance index failed \n");
        return -5;
    }

    pchInstIdxStr[u32InstIdxPos] = (i32InstIdx % 10) + '0';

#if defined (__GNUC__) && !(__CC_ARM)
    sprintf(pchFilePath, "%s/%s%s%012s%s", pchSaveFolder, pchFilePrefixStr, pchInstIdxStr, ulltoStr(u64FNPostIdx), pchFileExtStr);
#else
    sprintf(pchFilePath, "%s\\%s%s%012"PRId64"%s", pchSaveFolder, pchFilePrefixStr, pchInstIdxStr, u64FNPostIdx, pchFileExtStr);
#endif

    free(pchInstIdxStr);
    *pchNewFileName = pchFilePath;
    return 0;
}

void
FileMgr_AppendFileInfo(
    E_FILEMGR_TYPE eFileType,
    char *pchFileName
)
{
    S_MEDIA_FILE_LIST *psFileList;
    char *pchFilePrefixStr = NULL;
    char *pchInstIdxStr = NULL;
    uint32_t u32InstIdxPos;

    psFileList = s_asMediaFileTable[eFileType].psFileList;
    pchFilePrefixStr = s_asMediaFileTable[eFileType].szPrefixName;
    pchInstIdxStr = s_asMediaFileTable[eFileType].szInstIdxName;
    u32InstIdxPos = s_asMediaFileTable[eFileType].u32InstIdxPos;

    AppendFileList(pchFileName, pchFilePrefixStr, pchInstIdxStr, u32InstIdxPos, psFileList);
}


void
FileMgr_UpdateFileInfo(
    E_FILEMGR_TYPE eFileType,
    char *pchFileName
)
{
    S_MEDIA_FILE_LIST *psFileList;
    char *pchFilePrefixStr = NULL;
    char *pchInstIdxStr = NULL;
    uint32_t u32InstIdxPos;

    psFileList = s_asMediaFileTable[eFileType].psFileList;
    pchFilePrefixStr = s_asMediaFileTable[eFileType].szPrefixName;
    pchInstIdxStr = s_asMediaFileTable[eFileType].szInstIdxName;
    u32InstIdxPos = s_asMediaFileTable[eFileType].u32InstIdxPos;

    UpdateFileEnt(pchFileName, pchFilePrefixStr, pchInstIdxStr, u32InstIdxPos, psFileList);
}

void
FileMgr_CloseFile(
    E_FILEMGR_TYPE eFileType,
    char *pchFileName,
    int32_t i32CloseFD,
    bool bUnlink
)
{
    S_MEDIA_FILE_LIST *psFileList;
    char *pchFilePrefixStr = NULL;
    char *pchInstIdxStr = NULL;
    uint32_t u32InstIdxPos;
    S_MEDIA_FILE_ENT *psFileEnt;

    psFileList = s_asMediaFileTable[eFileType].psFileList;
    pchFilePrefixStr = s_asMediaFileTable[eFileType].szPrefixName;
    pchInstIdxStr = s_asMediaFileTable[eFileType].szInstIdxName;
    u32InstIdxPos = s_asMediaFileTable[eFileType].u32InstIdxPos;

    psFileEnt = UpdateFileEnt(pchFileName, pchFilePrefixStr, pchInstIdxStr, u32InstIdxPos, psFileList);

	close(i32CloseFD);

    if (bUnlink)
    {
		unlink(pchFileName);

        if (psFileEnt)
        {
            RemoveFileEnt(psFileList, psFileEnt);
        }
    }
}

void
FileMgr_FreeAllFileList(void)
{

    int32_t i;
    S_MEDIA_FILE_LIST *psFileList;

    for (i = 0; i < eFILEMGR_TYPE_END; i++)
    {
        psFileList = s_asMediaFileTable[i].psFileList;
        FreeFileList(psFileList);
        psFileList->u32LatestFileSize = DEFAULT_RESERVE_SIZE;

        if (s_asMediaFileTable[i].pchWorkingFolder)
        {
            free(s_asMediaFileTable[i].pchWorkingFolder);
            s_asMediaFileTable[i].pchWorkingFolder = NULL;
        }

    }
}

