#pragma once

#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>

#define DEBUG true

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float float32;

typedef uint8 byte;

#define VK_CHECK(f, ...) \
    if (f != VK_SUCCESS) \
    { \
	fprintf(output_file.fp, "[%s:%d] error : %s - ", __FILE__, __LINE__, #f); \
     assert(false);	  \
     } \
    else\
    { \
	fprintf(output_file.fp, "[%s:%d] success : %s\n", __FILE__, __LINE__, #f);	\
    }

#define OUTPUT_DEBUG_LOG(str, ...) \
    fprintf(output_file.fp, "[%s:%d] log: ", __FILE__, __LINE__); \
    fprintf(output_file.fp, str, __VA_ARGS__);

#define global static
#define persist static
#define internal static
#define extern_impl

extern class GLFWwindow *window;

extern struct Debug_Output
{
    FILE *fp;
} output_file;

extern void
output_debug(const char *format
	     , ...);

struct Stack_Allocation_Header
{
    #if DEBUG
    const char *allocation_name;
    #endif
    
    uint32 size;
    void *prev;
};

extern struct Stack_Allocator
{
    void *start = nullptr;
    void *current = nullptr;
    
    uint32 allocation_count = 0;
    uint32 capacity;
} stack_allocator_global;

extern void *
allocate_stack(uint32 allocation_size
	       , uint8 alignment
	       , const char *name = ""
	       , Stack_Allocator *allocator = &stack_allocator_global);

// only applies to the allocation at the top of the stack
extern void
extend_stack_top(uint32 extension_size
	     , Stack_Allocator *allocator = &stack_allocator_global);

// contents in the allocation must be destroyed by the user
extern void
pop_stack(Stack_Allocator *allocator = &stack_allocator_global);
 
inline uint8
get_alignment_adjust(void *ptr
      , uint32 alignment)
{
    byte *byte_cast_ptr = (byte *)ptr;
    uint8 adjustment = alignment - reinterpret_cast<uint64>(ptr) & static_cast<uint64>(alignment - 1);
    if (adjustment == 0) return(0);
    
    return(adjustment);
}

template <typename T>
inline void
destroy(T *ptr
	, uint32 size = 1)
{
    for (uint32 i = 0
	     ; i < size
	     ; ++i)
    {
	ptr[i].~T();
    }
}

inline constexpr uint64
kilobytes(uint32 kb)
{
    return(kb * 1024);
}

inline constexpr uint64
megabytes(uint32 mb)
{
    return(kilobytes(mb * 1024));
}

// math TODO()
struct alignas(16) V2f_32
{
    static constexpr uint32 dimension = 2;
    
    union
    {
	struct { float32 x, y; };
	float32 v[2];
    };

    inline float32 &
    operator[](uint32 i)
    {
	return v[i];
    }
};

struct alignas(16) V3f_32
{
    static constexpr uint32 dimension = 3;

    union
    {
	struct { float32 x, y, z; };
	float32 v[3];
    };

    inline float32 &
    operator[](uint32 i)
    {
	return v[i];
    }
};

struct alignas(16) V4f_32
{
    static constexpr uint32 dimension = 4;

    union
    {
	struct { float32 x, y, z, w; };
	float32 v[4];
    };

    inline float32 &
    operator[](uint32 i)
    {
	return v[i];
    }
};

inline void
add(float32 *dest
    , float32 *__restrict a
    , float32 *__restrict b
    , uint32 dim)
{
    for (uint32 i = 0
	     ; i < dim
	     ; ++i)

    {
	
    }
}

inline void
add_simd(float32 *__restrict a
    , float32 *__restrict b
    , uint8 dim)
{
    
}
