#ifndef _GLOBDEFS_H
#define _GLOBDEFS_H

#include <signal.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <float.h>


#ifndef u8
#define u8 unsigned char
#endif

#ifndef s8
#define s8 char
#endif

#ifndef c8
#define c8 char
#endif

#ifndef u16
#define u16 unsigned short
#endif


#ifndef s16
#define s16 short
#endif

#ifndef i32
#define i32  int
#endif


#ifndef u32
#define u32 uint32_t
#endif


#ifndef s32
#define s32 int32_t
#endif


#ifndef u64
#define u64 uint64_t
#endif


#ifndef s64
#define s64 int64_t
#endif


/* Длинное время */
#ifndef	time64
#define time64	int64_t
#endif

/* long double не поддержываеца  */
#ifndef f32
#define f32 float
#endif


#ifndef bool
#define bool u8
#endif

#ifndef true
#define true 1
#endif

#ifndef false
#define false 0
#endif


#ifndef IDEF
#define IDEF static inline
#endif




#endif				/* globdefs.h */
