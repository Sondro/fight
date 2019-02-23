#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define global static
#define internal static
#define local_persist static

#define Bytes(n) (n)
#define Kilobytes(n) (Bytes(n)*1024)
#define Megabytes(n) (Kilobytes(n)*1024)

typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef int8_t   b8;
typedef int16_t  b16;
typedef int32_t  b32;
typedef int64_t  b64;
typedef float    f32;
typedef double   f64;