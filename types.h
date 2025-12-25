#pragma once

#include <stdint.h> // uint64_t

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;
//typedef unsigned long long uint64;
typedef uint64_t uint64;
typedef signed char int8;
typedef signed short int16;
typedef signed int int32;
//typedef signed long long int64;
typedef int64_t int64;

#ifndef _WIN32
typedef uint64_t __time64_t;
#endif

#define MAX_PATH 260
//#define MAX_PATH 4096

#ifdef _WIN32
    #define NOMINMAX
    #include <windows.h> // Where LARGE_INTEGER is defined
#else
  // Define LARGE_INTEGER for non-Windows platforms (like Linux)
  typedef union _LARGE_INTEGER {
      struct {
          uint32_t LowPart;
          int32_t HighPart;
      } u;
      int64_t QuadPart;
  } LARGE_INTEGER;
#endif

#ifdef __linux__
    typedef wchar_t* PWCHAR;
    typedef int errno_t;
    #define fopen_s(pFile,filename,mode) ((*(pFile))=fopen((filename),(mode)))==NULL
#endif

