/******************************************************************************/
/* RETARGET.C: 'Retarget' layer for target-dependent low level functions      */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005 Keil Software. All rights reserved.                     */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/

/*
 * Copyright (C) ARM Limited, 2006. All rights reserved.
 * 
 * This is a retargeted I/O example which implements the functions required
 * for communication through an UART. The implementation relies on two UART
 * functions which the user must provide (UART_write and UART_read) for 
 * sending and receiving single characters to and from the UART.
 *
 * See the "rt_sys.h" header file for complete function descriptions.
 */

#if defined (__GNUC__)
#undef _MODE_T_DECLARED
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined (__ARMCC_VERSION)
#include <rt_sys.h>
#endif

#if defined (__GNUC__)
#include <sys/stat.h>
#include <errno.h>
#endif

#include "wblib.h"
#include "N9H26_NVTFAT.h"

#if defined (__ARMCC_VERSION)
#pragma import(__use_no_semihosting_swi)
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if defined (__ARMCC_VERSION)

#define DEFAULT_HANDLE 0x100


/*
 * These names are special strings which will be recognized by 
 * _sys_open and will cause it to return the standard I/O handles, instead
 * of opening a real file.
 */
const char __stdin_name[] ="STDIN";
const char __stdout_name[]="STDOUT";
const char __stderr_name[]="STDERR";

/*
 * Open a file. May return -1 if the file failed to open. We do not require
 * this function to do anything. Simply return a dummy handle.
 */
FILEHANDLE _sys_open(const char * name, int openmode)
{
	int i32OpenFlag = 0;
	int i32FD;
	char szUnicodeFileName[MAX_FILE_NAME_LEN];
	
	if(strcmp(name, __stdin_name) == 0)
		return DEFAULT_HANDLE;
	
	if(strcmp(name, __stdout_name) == 0)
		return DEFAULT_HANDLE;

	if(strcmp(name, __stderr_name) == 0)
		return DEFAULT_HANDLE;
	
	if(openmode & OPEN_PLUS){
		i32OpenFlag = O_RDWR;
	}
	else if (openmode & OPEN_W){
		i32OpenFlag = O_WRONLY;
	}
	else if (openmode & OPEN_B){
		i32OpenFlag = O_WRONLY;
	}
	else if (openmode == OPEN_R){
		i32OpenFlag = O_RDONLY;
	}
	else if (openmode == OPEN_A){
		i32OpenFlag = O_APPEND;
	}
	
	if((i32OpenFlag == O_RDWR) || (i32OpenFlag == O_WRONLY)){
			if(openmode & OPEN_W){
				i32OpenFlag |= (O_CREATE | O_TRUNC);
			}
	}
	
	fsAsciiToUnicode((VOID *)name, szUnicodeFileName, TRUE); 
	i32FD = fsOpenFile(szUnicodeFileName, NULL, i32OpenFlag);
	if(i32FD < 0)
		return -1;
	
	return i32FD;
}

/*
 * Close a file. Should return 0 on success or a negative value on error.
 * Not required in this implementation. Always return success.
 */
int _sys_close(FILEHANDLE fh)
{
	if(fh != DEFAULT_HANDLE){
		fsCloseFile(fh);
	}

    return 0; //return success
}

/*
 * Write to a file. Returns 0 on success, negative on error, and the number
 * of characters _not_ written on partial success. This implementation sends
 * a buffer of size 'len' to the UART.
 */
int _sys_write(FILEHANDLE fh, const unsigned char * buf,
               unsigned len, int mode)
{
	INT i32WriteCnt = 0;
	INT i32Result;
	
	if(fh == DEFAULT_HANDLE){
		unsigned char *szStr = (unsigned char *)buf;
		char chLastByte = szStr[len];

		szStr[len] = 0;
		sysprintf("%s", szStr);
		szStr[len] = chLastByte;
		return 0;
	}

	i32Result = fsWriteFile(fh, (UINT8 *)buf, len, &i32WriteCnt);
	if(i32Result < 0)
		return i32Result;
	
	return (len - i32WriteCnt);
}

/*
 * Read from a file. Can return:
 *  - zero if the read was completely successful
 *  - the number of bytes _not_ read, if the read was partially successful
 *  - the number of bytes not read, plus the top bit set (0x80000000), if
 *    the read was partially successful due to end of file
 *  - -1 if some error other than EOF occurred
 * This function receives a character from the UART, processes the character
 * if required (backspace) and then echo the character to the Terminal 
 * Emulator, printing the correct sequence after successive keystrokes.  
 */
int _sys_read(FILEHANDLE fh, unsigned char * buf,
              unsigned len, int mode)
{
	INT i32ReadCnt = 0;
	INT i32Result;

	if(fh != DEFAULT_HANDLE){
		i32Result = fsReadFile(fh, (UINT8 *)buf, len, &i32ReadCnt);
		if(i32Result < 0)
			return -1;
		i32Result = len - i32ReadCnt;
		
		if(fsIsEOF(fh)){
			i32Result |= 0x80000000;
		}
		
		return i32Result;
	}
	
    return 0;  
}


/*
 * Return non-zero if the argument file is connected to a terminal.
 */
int _sys_istty(FILEHANDLE fh)
{
	if(fh == DEFAULT_HANDLE)
		return 1;

	return 0;
}

/*
 * Move the file position to a given offset from the file start.
 * Returns >=0 on success, <0 on failure. Seeking is not supported for the 
 * UART.
 */
int _sys_seek(FILEHANDLE fh, long pos)
{
	if(fh != DEFAULT_HANDLE){
		return fsFileSeek(fh, pos, SEEK_SET);
	}

	return -1; // error
}

/*
 * Flush any OS buffers associated with fh, ensuring that the file
 * is up to date on disk. Result is >=0 if OK, negative for an
 * error.
 */
int _sys_ensure(FILEHANDLE fh)
{
    return 0; // success
}

/*
 * Return the current length of a file, or <0 if an error occurred.
 * _sys_flen is allowed to reposition the file pointer (so Unix can
 * implement it with a single lseek, for example), since it is only
 * called when processing SEEK_END relative fseeks, and therefore a
 * call to _sys_flen is always followed by a call to _sys_seek.
 */
long _sys_flen(FILEHANDLE fh)
{
 	if(fh != DEFAULT_HANDLE){
		return fsGetFileSize(fh);
	}

	return -1;
}

/*
 * Return the name for temporary file number sig in the buffer
 * name. Returns 0 on failure. maxlen is the maximum name length
 * allowed.
 */
int _sys_tmpnam(char * name, int sig, unsigned maxlen)
{
    return 0; // fail, not supported
}
#endif

#if defined(__GNUC__)

#if 0
int _open (const char * path, int flags, ...)
{
  return (-1);
}

int _close (int fd)
{
  return (-1);
}

int _lseek (int fd, int ptr, int dir)
{
  return (0);
}
#endif

int __attribute__((weak)) _fstat (int fd, struct stat * st)
{
  memset (st, 0, sizeof (* st));
  st->st_mode = S_IFCHR;
  return (0);
}

#if 0
int _isatty (int fd)
{
  return (1);
}
#endif

//Cannot link _read._write for --specs=nosys.specs linker option. using  sysGetChar()/sysPutchar at hal/mphalport.c 
int _read (int fd, char * ptr, int len)
{
  if(ptr == NULL)
	return 0;

  ptr[0] = sysGetChar();
  return 1;
}

int _write (int fd, char * ptr, int len)
{
	unsigned char *szStr = (unsigned char *)ptr;
	char chLastByte = szStr[len];

	//chchen: TODO: Maby using sysPutChar();
	szStr[len] = 0;
	sysprintf("%s", szStr);
	szStr[len] = chLastByte;
	return len;
}

#if 0
caddr_t _sbrk (int incr)
{
  static char * heap;
         char * prev_heap;

  if (heap == NULL)
  {
    heap = (char *)&__HeapBase;
  }

  prev_heap = heap;

  if ((heap + incr) > (char *)&__HeapLimit)
  {
    errno = ENOMEM;
    return (caddr_t) -1;
  }

  heap += incr;

  return (caddr_t) prev_heap;
}
#endif

#endif


#ifdef __cplusplus
}
#endif
