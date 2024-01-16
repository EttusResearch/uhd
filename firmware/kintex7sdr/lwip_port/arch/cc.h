#ifndef INCLUDED_ARCH_CC_H
#define INCLUDED_ARCH_CC_H

#define BYTE_ORDER BIG_ENDIAN


#if 1
#include <stdint.h>

typedef uint8_t    u8_t;
typedef int8_t     s8_t;
typedef uint16_t   u16_t;
typedef int16_t    s16_t;
typedef uint32_t   u32_t;
typedef int32_t    s32_t;

#else

typedef unsigned   char    u8_t;
typedef signed     char    s8_t;
typedef unsigned   short   u16_t;
typedef signed     short   s16_t;
typedef unsigned   long    u32_t;
typedef signed     long    s32_t;
#endif

typedef u32_t mem_ptr_t;

#if 1   /* minimal printf */
#define U16_F "u"
#define S16_F "d"
#define X16_F "x"
#define U32_F "u"
#define S32_F "d"
#define X32_F "x"

#else

#define U16_F "hu"
#define S16_F "hd"
#define X16_F "hx"
#define U32_F "lu"
#define S32_F "ld"
#define X32_F "lx"
#endif

#if 1	// gcc: don't pack
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#else	// gcc: do pack
#define PACK_STRUCT_FIELD(x) x
#define PACK_STRUCT_STRUCT __attribute__((packed))
#define PACK_STRUCT_BEGIN
#define PACK_STRUCT_END
#endif

//#define LWIP_PLATFORM_ASSERT(msg) ((void)0)
void abort(void);
#define LWIP_PLATFORM_ASSERT(msg) abort()


#endif /* INCLUDED_ARCH_CC_H */

