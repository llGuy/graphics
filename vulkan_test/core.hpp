#pragma once

#include <stdarg.h>
#include <stdint.h>

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
output_debug(char *format
	     , ...);

struct Stack_Allocation_Header
{
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
