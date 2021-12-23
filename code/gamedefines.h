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

#define _RGB(r,g,b) ((float)r)/255.f, ((float)g)/255.f, ((float)b)/255.f

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
#define GAME_MAPS_DIRECTORY "data/maps/"

/** Custom Data Structures **/
template<typename T> struct PreAllocArray
{
    /** An array which pre-allocates memory before use.
     * No dynamic resizing allowed. This is essentially a
     * wrapper for a C-style array. */

    void Allocate(u32 numOfElements)
    {
        if(data)
        {
            Free();
        }
        // TODO(Kevin): use custom mem alloc
        data = (T*) calloc(numOfElements, sizeof(T));
        count = 0;
        capacity = numOfElements;

        for(u32 i = 0; i < numOfElements; ++i)
        {
            new (data + i) T; // placement new required for polymorphism
        }
    }

    void Free()
    {
        if(data)
        {
            // TODO(Kevin): use custom mem alloc
            free(data);
            count = 0;
            capacity = 0;
        }
    }

    void Clear()
    {
        count = 0;
    }

    T& At(const u32 index)
    {
        if(index < capacity)
        {
            return *(data + index);
        }
        throw std::out_of_range("PreAllocArray: Index out of range.");
    }

    T& operator[](const u32 index)
    {
        return At(index);
    }

    T& PushBack(T elem)
    {
        if(count < capacity)
        {
            *(data+count) = elem;
            return *(data + count++);
        }
        throw std::out_of_range("PreAllocArray: Cannot PushBack. Array is full.");
    }

    T PopBack()
    {
        return *(data + count--);
    }

    T PopAt(u32 index)
    {
        // Does not preserve order
        if(index < capacity)
        {
            T a = *(data + index);
            *(data + index) = *(data + --count);
            return a;
        }
        throw std::out_of_range("PreAllocArray: Index out of range.");
    }

    u32 Count() const
    {
        return count;
    }

    void IncrementCount()
    {
        ++count;
    }

    void DecrementCount()
    {
        --count;
    }

    u32 Capacity() const
    {
        return capacity;
    }

private:
    T* data = NULL;
    u32 count = 0;    // in number of elements
    u32 capacity = 0; // in number of elements
};
