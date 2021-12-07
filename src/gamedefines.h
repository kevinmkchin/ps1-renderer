#pragma once

#include <cstdint>

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


#define TARGET_FPS 60
#define BACKBUFFER_WIDTH 640
#define BACKBUFFER_HEIGHT 480


/** HELPER FUNCTIONS **/
#if SLOW_BUILD
#define ASSERT(predicate) if(!(predicate)) {*(int*)0 = 0;}
#else
#define ASSERT(predicate)
#endif
