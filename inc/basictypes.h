#ifndef __BASICTYPES_H__
#define __BASICTYPES_H__

#include <stdint.h>
#include <stddef.h>

typedef unsigned char byte;
typedef unsigned short word;
typedef unsigned int dword;
typedef unsigned long long ddword;
typedef unsigned long long dword64;
typedef long long long64;
typedef uintptr_t pword;	// pointer sized unsigned integer
#undef NULL
#define NULL 0

#define ELEMENTSIZE(a) (sizeof(a[0]))
#define ARRAYSIZE(a) (sizeof(a) / sizeof(a[0]))
#define OFFSETOF(t, m) ((int)offsetof(t, m))
    
#endif // __BASICTYPES_H__
