#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define global static
#define internal static
#define local_persist static

#define Bytes(n) (n)
#define Kilobytes(n) (Bytes(n)*1024)
#define Megabytes(n) (Kilobytes(n)*1024)

#define ArrayCount(a) (sizeof(a) / sizeof((a)[0]))

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

typedef struct v2
{
    f32 x;
    f32 y;
}
v2;

#define v2(x, y) V2Init(x, y)
internal v2
V2Init(f32 x, f32 y)
{
    v2 v = { x , y };
    return v;
}

typedef union v3
{
    struct {
        f32 r;
        f32 g;
        f32 b;
    };
    
    struct {
        f32 x;
        f32 y;
        f32 z;
    };
}
v3;

#define v3(x, y, z) V3Init(x, y, z)
internal v3
V3Init(f32 x, f32 y, f32 z)
{
    v3 v = { x, y, z };
    return v;
}

typedef struct iv2
{
    i32 x;
    i32 y;
}
iv2;

#define iv2(x, y) IV2Init(x, y)
internal iv2
IV2Init(i32 x, i32 y)
{
    iv2 v = { x , y };
    return v;
}

typedef union iv3
{
    struct {
        i32 r;
        i32 g;
        i32 b;
    };
    
    struct {
        i32 x;
        i32 y;
        i32 z;
    };
}
iv3;

#define iv3(x, y, z) IV3Init(x, y, z)
internal iv3
IV3Init(i32 x, i32 y, i32 z)
{
    iv3 v = { x, y, z };
    return v;
}

internal i32
ClampI32(i32 v, i32 l, i32 h)
{
    i32 result = v;
    if(result < l)
    {
        result = l;
    }
    else if(result > h)
    {
        result = h;
    }
    return result;
}