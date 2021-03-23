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

#include "NVTMedia_SAL_FS.h"

#if NUMEDIA_CFG_SAL_FS_PORT == NUMEDIA_SAL_FS_PORT_NVTFAT


int open(
	const char *path, 
	int oflags
)
{
	char szUnicodeFileName[MAX_FILE_NAME_LEN];

	fsAsciiToUnicode((void *)path, szUnicodeFileName, TRUE); 
 
	return fsOpenFile(szUnicodeFileName, NULL, oflags);
}

int close(
	int fildes
)
{
	return fsCloseFile(fildes);
}

int read(
	int fildes, 
	void *buf, 
	int nbyte
)
{
	int32_t i32Ret;
	int32_t i32ReadCnt = 0;
	i32Ret = fsReadFile(fildes, buf, nbyte, (INT *)&i32ReadCnt);

	if(i32Ret < 0)
		return i32Ret;
	return i32ReadCnt;
}

int write(
	int fildes, 
	const void *buf, 
	int nbyte
)
{
	int32_t i32Ret;
	int32_t i32WriteCnt = 0;
	i32Ret = fsWriteFile(fildes, (UINT8 *)buf, nbyte, (INT *)&i32WriteCnt);

	if(i32Ret < 0)
		return i32Ret;
	
	return i32WriteCnt;
}


int64_t 
lseek(
	int fildes, 
	int64_t offset, 
	int whence
)
{
	return fsFileSeek(fildes, offset, whence);
}

int stat(
	const char *path, 
	struct stat *buf
)
{
	char szUnicodeFileName[MAX_FILE_NAME_LEN];
	FILE_STAT_T sFileState;

	fsAsciiToUnicode((void *)path, szUnicodeFileName, TRUE); 

	if(fsGetFileStatus(-1, szUnicodeFileName, NULL, &sFileState) < 0){
		return -1;
	}
	
	buf->st_mode = sFileState.ucAttrib;
	buf->st_size = sFileState.n64FileSize;
	
	//[FIXME]
	buf->st_atime = 0;
	buf->st_mtime = 0;
	buf->st_ctime = 0;	
	return 0;
}

int fstat(
	int fildes, 
	struct stat *buf
)
{
	FILE_STAT_T sFileState;


	if(fsGetFileStatus(fildes, NULL, NULL, &sFileState) < 0){
		return -1;
	}
	
	buf->st_mode = sFileState.ucAttrib;
	buf->st_size = sFileState.n64FileSize;
	
	//[FIXME]
	buf->st_atime = 0;
	buf->st_mtime = 0;
	buf->st_ctime = 0;	
	return 0;
}

int unlink(
	const char *pathname
)
{
	char szUnicodeFileName[MAX_FILE_NAME_LEN];

	fsAsciiToUnicode((void *)pathname, szUnicodeFileName, TRUE); 
	return fsDeleteFile(szUnicodeFileName, NULL);
}

int truncate(
	const char *path, 
	uint64_t length
)
{
	char szUnicodeFileName[MAX_FILE_NAME_LEN];

	fsAsciiToUnicode((void *)path, szUnicodeFileName, TRUE); 

	return fsSetFileSize(-1, szUnicodeFileName, NULL, length);
}

int fileno(FILE *pFile)
{
	int *piFileNo = (int *)((char *)pFile + 20);
	return *piFileNo;
}

int ftruncate(
	int fildes, 
	int64_t length
)
{
	return fsSetFileSize(fildes, NULL, NULL, length);
}

DIR *opendir(
	const char *name
)
{
	int32_t i32ReadDirRet;
	DIR *psDir = NULL;
	char szLongName[MAX_FILE_NAME_LEN/2];

	psDir = malloc(sizeof(DIR));
	if(psDir == NULL)
		return NULL;
	
	memset(psDir, 0 , sizeof(DIR));
	psDir->bLastDirEnt = 0;
	fsAsciiToUnicode((void *)name, szLongName, TRUE);

	i32ReadDirRet = fsFindFirst(szLongName, (CHAR *)name, &psDir->tCurFieEnt);
	if (i32ReadDirRet < 0){
		free(psDir);
		return NULL;
	}
	
	return psDir;	
}


struct dirent *readdir(DIR *dirp)
{
	if(dirp == NULL)
		return NULL;
	
	if(dirp->bLastDirEnt == 1){
		return NULL;
	}

	fsUnicodeToAscii(dirp->tCurFieEnt.suLongName, dirp->sDirEnt.d_name, 1);

	if(fsFindNext(&dirp->tCurFieEnt) != 0)
		dirp->bLastDirEnt = 1;

	return &dirp->sDirEnt;
}


int closedir(DIR *dirp){
	if(dirp == NULL)
		return -1;

	fsFindClose(&dirp->tCurFieEnt);	
	free(dirp);
	return 0;
}

#elif NUMEDIA_CFG_SAL_FS_PORT == NUMEDIA_SAL_FS_PORT_OOFAT

#include <stdio.h>
#include <stdlib.h>

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

static FATFS *lookup_path(const char **path) {
    mp_vfs_mount_t *fs = mp_vfs_lookup_path(*path, path);
    if (fs == MP_VFS_NONE || fs == MP_VFS_ROOT) {
        return NULL;
    }
    // here we assume that the mounted device is FATFS
    return &((fs_user_mount_t*)MP_OBJ_TO_PTR(fs->obj))->fatfs;
}


int open(
	const char *path, 
	int oflags
)
{
	FATFS *vfs = lookup_path(&path);
	BYTE mode = FA_OPEN_EXISTING;
    FRESULT res;
    FIL *psFile;
	
	printf("DDDDD open -- 0 \n");
	if(vfs == NULL)
		return -1;

	if(oflags & O_RDONLY)
		mode |= FA_READ;
	
	if(oflags & O_WRONLY)
		mode |= FA_WRITE;

	if(oflags & O_CREATE)
		mode |= FA_CREATE_ALWAYS;

	if(oflags & O_APPEND)
		mode |= FA_OPEN_APPEND;

	printf("DDDDD open -- 1 \n");
	psFile = malloc(sizeof(FIL));

	if(psFile == NULL)
		return  -2;

	printf("DDDDD open %s, oflags %x, mode %x -- 2 \n", path, oflags, mode);

#if 1
	res = f_open(vfs, psFile, (void *)path, mode);
//    res = f_open(vfs, psFile, (void *)"DCIM/NVTDV_P0_000000000001.mp4", mode);
#else
	char achCWD[100];
 
    res = f_chdir(vfs, (void *)"/");
    if (res != FR_OK){
		printf("chdir faild %d\n", res);
		return -4;
	}

	res = f_getcwd(vfs, achCWD, 100);
	printf("current paht %s, %d \n", achCWD, res);
	
    res = f_chdir(vfs, (void *)"DCIM");
    if (res != FR_OK){
		printf("chdir faild %d\n", res);
		return -4;
	}
	
	f_getcwd(vfs, achCWD, 100);
	printf("current paht1 %s, %d \n", achCWD, res);


//    res = f_open(vfs, psFile, (void *)"NVTDV_P0_000000000001.mp4", mode);

#endif
    if (res == FR_OK)
		return (int)psFile;

	printf("DDDDD open -- 3 error %d \n", res);
	return -3;
}

int close(int fildes)
{
	FIL *psFile = (FIL *)fildes;
    FRESULT res;
    
    if(psFile == NULL)
		return -1;
		
	res = f_close (psFile);
	free(psFile);
	return res;
}

int stat(
	const char *path, 
	struct stat *buf
)
{
	FRESULT res;
	FATFS *vfs = lookup_path(&path);
	FILINFO sFileInfo;

	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
	if(vfs == NULL)
		return -1;

	res = f_stat(vfs, path, &sFileInfo);        /* Get file status */
	if(res != FR_OK)
		return res;
		
	buf->st_mode = sFileInfo.fattrib;
	buf->st_size = sFileInfo.fsize;
	
	//[FIXME]
	buf->st_atime = 0;
	buf->st_mtime = 0;
	buf->st_ctime = 0;	

	return 0;
}

int fstat(int fildes, struct stat *buf)
{
    FIL *psFile = (FIL *)fildes;
    
	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
    if(psFile == NULL)
		return -1;

	buf->st_mode = psFile->obj.attr;
	buf->st_size = f_size(psFile);
	
	//[FIXME]
	buf->st_atime = 0;
	buf->st_mtime = 0;
	buf->st_ctime = 0;	

	return 0;
}

int read(int fildes, void *buf, int nbyte)
{
	FRESULT i32Ret;
	FIL *psFile = (FIL *)fildes;
	UINT u32ReadCnt = 0;

	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
	if(psFile == NULL)
		return -1;

	i32Ret = f_read(psFile, buf, (UINT)nbyte, &u32ReadCnt);

	if(i32Ret != FR_OK)
		return -i32Ret;

	return u32ReadCnt;
}

int write(int fildes, const void *buf, int nbyte)
{
	FRESULT i32Ret;
	UINT i32WriteCnt = 0;
	FIL *psFile = (FIL *)fildes;

	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
	if(psFile == NULL)
		return -1;

	i32Ret = f_write(psFile, buf, nbyte, &i32WriteCnt);

	if(i32Ret != FR_OK)
		return -i32Ret;
	
	return i32WriteCnt;
}

int64_t lseek(int fildes, int64_t offset, int whence)
{
	FIL *psFile = (FIL *)fildes;

	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
	if(psFile == NULL)
		return -1;

	switch (whence) {
		case SEEK_SET: // SEEK_SET
			f_lseek(psFile, offset);
			break;

		case SEEK_CUR: // SEEK_CUR
			f_lseek(psFile, f_tell(psFile) + offset);
			break;

		case SEEK_END: // SEEK_END
			f_lseek(psFile, f_size(psFile) + offset);
			break;
	}

	return f_tell(psFile);
}

int unlink(const char *pathname)
{
	FRESULT res;
	FATFS *vfs = lookup_path(&pathname);

	if(vfs == NULL)
		return -1;

	res = f_unlink (vfs, pathname);

	if(res != FR_OK)
		return -res;
	return 0;
}

int ftruncate(int fildes, int64_t length)
{
	FRESULT res;
	FIL *psFile = (FIL *)fildes;

	if(psFile == NULL)
		return -1;

	res = f_lseek(psFile, length);

	if(res != FR_OK)
		return -res;

	res = f_truncate(psFile);

	if(res != FR_OK)
		return -res;
	return 0;
}

DIR *opendir(
	const char *name
)
{
	FRESULT res;
	FATFS *vfs = lookup_path(&name);

	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
	if(vfs == NULL)
		return NULL;

	DIR *psDir = NULL;
	char *dir_name = NULL;

	psDir = malloc(sizeof(DIR));
	if(psDir == NULL)
		return NULL;

	int path_len = strlen(name);

	dir_name = malloc(path_len + 1 );
	if(dir_name == NULL){
		free(psDir);
		return NULL;
	}
	
	strcpy(dir_name, name); 
	if(dir_name[path_len - 1] == '/'){
		dir_name[path_len - 1] = 0;
	}

	res = f_opendir(vfs, &psDir->sCurDir, dir_name);

	free(dir_name);

	if(res != FR_OK){
		free(psDir);
		return NULL;
	}

	return psDir;
}

struct dirent *readdir(DIR *dirp)
{
	FRESULT res;
	FILINFO sFileInfo;

	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
	if(dirp == NULL)
		return NULL;

	res = f_readdir(&dirp->sCurDir, &sFileInfo);  /* Find the first item */

	if(res != FR_OK)
		return NULL;

	if(sFileInfo.fname[0] == 0)
		return NULL;
		
	memcpy(dirp->sDirEnt.d_name, sFileInfo.fname, _MAX_LFN);

	return &dirp->sDirEnt;
}

int closedir(DIR *dirp)
{
	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
	if(dirp == NULL)
		return -1;

	f_closedir(&dirp->sCurDir);
	free(dirp);
	return 0;
}

uint64_t disk_free_space(const char *pathname)
{
	FATFS *vfs = lookup_path(&pathname);
	uint32_t u32FreeClst;

	printf("DDDDD function %s -- 0 \n", __FUNCTION__);
	if(vfs == NULL)
		return 0;

	//Get free storage size;
	f_getfree(vfs, &u32FreeClst);
	return ((uint64_t)u32FreeClst * vfs->csize * 512);
}



#endif
