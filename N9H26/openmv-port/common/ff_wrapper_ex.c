/*
 * This file is part of the OpenMV project.
 * Copyright (c) 2013-2016 Kwabena W. Agyeman <kwagyeman@openmv.io>
 * This work is licensed under the MIT license, see the file LICENSE for details.
 *
 * File System Helper Functions
 *
 */
#include <string.h>

#include "extmod/vfs.h"
#include "extmod/vfs_fat.h"

static char *s_szFSError[FR_INVALID_PARAMETER + 1] =
{
	"FS: Success",
	"FS: Error occurred in the low level disk I/O layer"
	"FS: Assertion failed"
	"FS: The physical drive cannot work"
	"FS: Could not find the file"
    "FS: Could not find the path"
    "FS: The path name format is invalid"
    "FS: Access denied due to prohibited access or directory full"
    "FS: Access denied due to prohibited access"
    "FS: The file/directory object is invalid"
    "FS: The physical drive is write protected"
    "FS: The logical drive number is invalid"
    "FS: The volume has no work area"
    "FS: There is no valid FAT volume"
    "FS: The f_mkfs() aborted due to any problem"
    "FS: Could not get a grant to access the volume within defined period"
    "FS: The operation is rejected according to the file sharing policy"
    "FS: LFN working buffer could not be allocated"
    "FS: Number of open files > _FS_LOCK"
    "FS: Given parameter is invalid"
};

const char *ffs_strerror(FRESULT res)
{
	return s_szFSError[res];
}
