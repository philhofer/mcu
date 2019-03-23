#ifndef __BITS_H_
#define __BITS_H_
#include <assert.h>

#define NULL ((void *)0)
#define true  1
#define false 0

typedef unsigned long  ulong;
typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef _Bool bool;

static_assert(sizeof(ulong) == 4);
static_assert(sizeof(uint) == 4);
static_assert(sizeof(ushort) == 2);
static_assert(sizeof(uchar) == 1);
static_assert(sizeof(bool) == 1);

typedef unsigned long long u64;
typedef signed   long long i64;
typedef ulong  u32;
typedef signed long i32;
typedef ushort u16;
typedef signed short i16;
typedef uchar  u8;
typedef signed char i8;

static_assert(sizeof(u64) == sizeof(i64) && sizeof(u64) == 8);
static_assert(sizeof(u32) == sizeof(i32) && sizeof(u32) == 4);
static_assert(sizeof(u16) == sizeof(i16) && sizeof(u16) == 2);
static_assert(sizeof(u8) == sizeof(i8) && sizeof(u8) == 1);

#endif
