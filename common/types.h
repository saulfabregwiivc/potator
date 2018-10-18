#ifndef __TYPES_H__
#define __TYPES_H__

#if defined NDS
#include <nds/ndstypes.h>
#endif

#ifdef NULL
#undef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#ifndef BOOL
#define BOOL int
#endif

#if defined(_ODSDL_)

#define uint8   unsigned char
#define uint16  unsigned short
#define uint32  unsigned long
//#define uint64  signed __int64
#define int8    signed char
#define int16   signed short
#define int32   signed long
//#define int64   unsigned __int64

#elif !defined(NDS)

typedef unsigned char           uint8;
typedef unsigned short int      uint16;
typedef unsigned int            uint32;
typedef unsigned long long int  uint64;

typedef signed char             int8;
typedef signed short int        int16;
typedef signed int              int32;
typedef signed long long int    int64;

typedef float                   float32;
typedef double                  float64;

typedef volatile uint8          vuint8;
typedef volatile uint16         vuint16;
typedef volatile uint32         vuint32;
typedef volatile uint64         vuint64;

typedef volatile int8           vint8;
typedef volatile int16          vint16;
typedef volatile int32          vint32;
typedef volatile int64          vint64;

typedef volatile float32        vfloat32;
typedef volatile float64        vfloat64;

typedef int32                   fixed;
typedef int64                   dfixed;

typedef volatile int32          vfixed;

#endif

#endif
