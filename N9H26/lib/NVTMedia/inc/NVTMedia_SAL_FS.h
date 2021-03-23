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

#ifndef __NUMEDIA_SAL_FS_H__
#define __NUMEDIA_SAL_FS_H__

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * \anchor          NUMEDIA_SAL_PORTS
 * \name            NuMedia file system ports
 * \{
 *
 * List of already available system ports.
 * Configure \ref NUMEDIA_CFG_SYS_PORT with one of these values to use preconfigured ports
 */

#define NUMEDIA_SAL_FS_PORT_NONE                       0   /*!< Without SAL */
#define NUMEDIA_SAL_FS_PORT_NVTFAT                     1   /*!< NVTFAT based port */
#define NUMEDIA_SAL_FS_PORT_OOFAT                      2   /*!< OOFAT based port */
#define NUMEDIA_SAL_FS_PORT_USER                       99  /*!< User custom implementation.
                                                    When port is selected to user mode, user must provide "NuMedia_SAL_User.h" file,
                                                    which is not provided with library.
                                                    */

#define NUMEDIA_CFG_SAL_FS_PORT    NUMEDIA_SAL_FS_PORT_OOFAT


#if NUMEDIA_CFG_SAL_FS_PORT == NUMEDIA_SAL_FS_PORT_NONE
/* For a totally minimal and standalone system, we provide null
   definitions of the sys_ functions. */

#elif NUMEDIA_CFG_SAL_FS_PORT == NUMEDIA_SAL_FS_PORT_NVTFAT
#if defined (__N9H26__)

#include "wblib.h"
#include "N9H26_NVTFAT.h"
#endif

struct stat{
	uint8_t st_mode;
	uint64_t st_size;
	uint64_t st_atime;
	uint64_t st_mtime;
	uint64_t st_ctime;
};

#define S_ISDIR(m) (m & FA_DIR)

struct dirent{
	char d_name[256];
};

typedef struct __dirstream
{
	FILE_FIND_T tCurFieEnt;
	struct dirent sDirEnt;
	int bLastDirEnt;
}DIR;


int open(const char *path, int oflags);
int close(int fildes);
int read(int fildes, void *buf, int nbyte);
int write(int fildes, const void *buf, int nbyte);
int64_t lseek(int fildes, int64_t offset, int whence);
int stat(const char *path, struct stat *buf);
int fstat(int fildes, struct stat *buf);
int unlink(const char *pathname);		//delete file
int truncate(const char *path, uint64_t length);
int ftruncate(int fildes, int64_t length);
int fileno(FILE *pFile);

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

#elif NUMEDIA_CFG_SAL_FS_PORT == NUMEDIA_SAL_FS_PORT_OOFAT

#include "wblib.h"
#include "lib/oofatfs/ff.h"

/*===================================================== file mode and flag ==*/
/* File open mode */
#define O_RDONLY                0x0001          /* Open for read only*/
#define O_WRONLY                0x0002      /* Open for write only*/
#define O_RDWR                  0x0003      /* Read/write access allowed.*/
#define O_APPEND        0x0004      /* Seek to eof on each write*/
#define O_CREATE        0x0008      /* Create the file if it does not exist.*/
#define O_TRUNC         0x0010      /* Truncate the file if it already exists*/
#define O_EXCL          0x0020      /* Fail if creating and already exists */
#define O_DIR                   0x0080      /* Open a directory file */
#define O_EXCLUSIVE             0x0200          /* Open a file with exclusive lock */
#define O_NODIRCHK              0x0400          /* no dir/file check */
#define O_FSEEK                 0x1000          /* Open with cluster chain cache to enhance file seeking performance */
#define O_IOC_VER2              0x2000          
#define O_INTERNAL              0x8000          

/* file attributes used for file stat.st_mode */
#define FA_RDONLY       0x01        /* Read only attribute */
#define FA_HIDDEN       0x02        /* Hidden file */
#define FA_SYSTEM       0x04        /* System file */
#define FA_LABEL        0x08        /* Volume label */
#define FA_DIR          0x10        /* Directory    */
#define FA_ARCHIVE      0x20        /* Archive */

#define SEEK_SET        0
#define SEEK_CUR        1
#define SEEK_END        2

struct stat{
	uint8_t st_mode;
	uint64_t st_size;
	uint64_t st_atime;
	uint64_t st_mtime;
	uint64_t st_ctime;
};

#define S_ISDIR(m) (m & AM_DIR)

struct dirent{
	char d_name[256];
};

typedef struct __dirstream
{
	FF_DIR sCurDir;
	struct dirent sDirEnt;
}DIR;

int open(const char *path, int oflags);
int close(int fildes);
int stat(const char *path, struct stat *buf);
int fstat(int fildes, struct stat *buf);
int read(int fildes, void *buf, int nbyte);
int write(int fildes, const void *buf, int nbyte);
int64_t lseek(int fildes, int64_t offset, int whence);
int unlink(const char *pathname);		//delete file
int ftruncate(int fildes, int64_t length);

DIR *opendir(const char *name);
struct dirent *readdir(DIR *dirp);
int closedir(DIR *dirp);

uint64_t disk_free_space(const char *pathname);
#endif


#ifdef __cplusplus
}
#endif

#endif
