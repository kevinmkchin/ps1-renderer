#pragma once

#include <cstdint>

#define GLOBAL_VAR    static
#define INTERNAL      static
#define LOCAL_PERSIST static

typedef uint8_t       u8;
typedef uint16_t      u16;
typedef uint32_t      u32;
typedef int8_t        i8;
typedef int16_t       i16;
typedef int32_t       i32;
typedef uint_fast8_t  u8f;
typedef uint_fast16_t u16f;
typedef uint_fast32_t u32f;
typedef int_fast8_t   i8f;
typedef int_fast16_t  i16f;
typedef int_fast32_t  i32f;
typedef i32           bool32;

#define INDEX_NONE -1

/** HELPER FUNCTIONS **/
#if SLOW_BUILD
#define ASSERT(predicate) if(!(predicate)) {*(int*)0 = 0;}
#else
#define ASSERT(predicate)
#endif

#define RGB(r,g,b) ((float)r)/255.f, ((float)g)/255.f, ((float)b)/255.f

#define printf printf

/** REFERENCE **/
INTERNAL u32 refQuadIndices[6] = {
        0, 1, 3,
        0, 3, 2
};
INTERNAL float refQuadVertices[16] = {
        //  x   y    u    v
        -1.f, -1.f, 0.f, 0.f,
        1.f, -1.f, 1.f, 0.f,
        -1.f, 1.f, 0.f, 1.f,
        1.f, 1.f, 1.f, 1.f
};

/** GAME **/
#define TARGET_FPS 60
#define INTERNAL_RESOLUTION_WIDTH 320
#define INTERNAL_RESOLUTION_HEIGHT 240
