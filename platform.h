// platform.h
// FireFly server platform abstraction macros

#ifndef FIREFLY_PLATFORM_H
#define FIREFLY_PLATFORM_H

#ifdef WIN32
#define WIN32_LEAN_AND_MEAN  // strip down windows headers
#define _CRT_SECURE_NO_WARNINGS
#pragma warning( disable : 4786 )
#endif

// define this to include crypt() in unistd.h on RedHat 6.2
#define _XOPEN_SOURCE

#ifdef __CYGWIN__
// use cygwin's crypt implementation if available
#include <crypt.h>
#else
#if defined WIN32 || defined __APPLE__
// use a dummy implementation (no encryption)
// #define crypt(str,salt) (str)
// use Eric Young's crypt implementation from libdes-l
// see also related stuff at the top of main.cpp
#define HEADER_DES_LOCL_H // for correct crypt() prototype
#include "libdes-l/des.h"
#endif
#endif

#include <stdlib.h>
#include <stdio.h>		// printf
#include <memory.h>
#include <string.h>
#include <time.h>
#include <math.h>		// floor
#include <fcntl.h>      // fcntl
#include <sys/types.h>  // sockets, select
#ifdef WIN32
#include <signal.h>		// signal, SIGTERM, SIGPIPE
#else
//#include <bsd/signal.h> // documented but does not exist
#include <signal.h>
#endif

#ifdef WIN32
#define CDECL __cdecl
#else
#define CDECL
#endif

#ifdef WIN32
  #include <io.h>
  #include <sys/stat.h>	  // _S_IREAD, _S_IWRITE
  #define S_IRUSR _S_IREAD
  #define S_IWUSR _S_IWRITE
#else
  #include <unistd.h>	// read, write, close, fcntl, select
  #include <sys/stat.h>
#endif

#ifdef WIN32
  #include <winsock.h>
#else
  #include <sys/time.h> // timeval
  #include <sys/types.h>
  #include <sys/socket.h> // AF
  #include <netinet/in.h> // INADDR_ANY
  #define SOCKET int
  #define INVALID_SOCKET -1
#endif

// Silly Win32 does text conversion unless this is used
// So just define as 0 for all other platforms
#ifndef O_BINARY
#define O_BINARY 0
#endif

// Reserved port number for Windows file ports
#define FILE_PORT_FLAG -1

// Required for Bison under Win32
#ifdef WIN32
#include <malloc.h>
#endif

#define MALLOC malloc
#define REALLOC realloc
#define FREE free

#define MEMCPY memcpy
#define MEMMOVE memmove
#define MEMCMP memcmp
#define MEMCLEAR(X,Y) memset((X), 0, (Y))

#define STRLEN strlen
#define STRCMP strcmp
#define STRCPY strcpy

#ifdef WIN32
#define OPEN _open
#define READ _read
#define WRITE _write
#define CLOSE _close
#define UNLINK _unlink
#else
#define OPEN open
#define READ read
#define WRITE write
#define CLOSE close
#define UNLINK unlink
#endif

#endif // FIREFLY_PLATFORM_H
