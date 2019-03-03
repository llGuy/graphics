#pragma once

#include <cassert>
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

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

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
    fprintf(output_file.fp, str, __VA_ARGS__); \
    fflush(output_file.fp);

#define persist static
#define internal static
#define global_var static

inline constexpr uint32
left_shift(uint32 n)
{
    return 1 << n;
}

extern struct Debug_Output
{
    FILE *fp;
} output_file;

void
output_debug(const char *format
	     , ...);

using Alignment = uint8;

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

void *
allocate_stack(uint32 allocation_size
	       , Alignment alignment = 1
	       , const char *name = ""
	       , Stack_Allocator *allocator = &stack_allocator_global);

// only applies to the allocation at the top of the stack
void
extend_stack_top(uint32 extension_size
	     , Stack_Allocator *allocator = &stack_allocator_global);

// contents in the allocation must be destroyed by the user
void
pop_stack(Stack_Allocator *allocator = &stack_allocator_global);

struct Free_Block_Header
{
    Free_Block_Header *next_free_block = nullptr;
    uint32 free_block_size = 0;
};

struct Free_List_Allocation_Header
{
    uint32 size;
#if DEBUG
    const char *name;
#endif
};

extern struct Free_List_Allocator
{
    Free_Block_Header *free_block_head;
    
    void *start;
    uint32 available_bytes;

    uint32 allocation_count = 0;
} free_list_allocator_global;

void *
allocate_free_list(uint32 allocation_size
		   , Alignment alignment = 1
		   , const char *name = ""
		   , Free_List_Allocator *allocator = &free_list_allocator_global);

void
deallocate_free_list(void *pointer
		     , Free_List_Allocator *allocator = &free_list_allocator_global);

struct File_Contents
{
    uint32 size;
    byte *content;
};

internal File_Contents
read_file(const char *filename
	  , Stack_Allocator *allocator = &stack_allocator_global)
{
    FILE *file = fopen(filename, "rb");
    if (file == nullptr)
    {
	OUTPUT_DEBUG_LOG("error - couldnt load file \"%s\"\n", filename);
	assert(false);
    }
    fseek(file, 0, SEEK_END);
    uint32 size = ftell(file);
    rewind(file);

    byte *buffer = (byte *)allocate_stack(size
					  , 1
					  , filename
					  , allocator);
    fread(buffer, 1, size, file);
    
    fclose(file);

    File_Contents contents { size, buffer };
    
    return(contents);
}
 
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

#ifndef __GNUC__
#include <intrin.h>
#endif

struct Bitset_32
{
    uint32 bitset = 0;

    inline uint32
    pop_count(void)
    {
#ifndef __GNUC__
	return __popcnt(bitset);
#else
	return __builtin_popcount(bitset);  
#endif
    }

    inline void
    set1(uint32 bit)
    {
	bitset |= left_shift(bit);
    }

    inline void
    set0(uint32 bit)
    {
	bitset &= ~(left_shift(bit));
    }

    inline bool
    get(uint32 bit)
    {
	return bitset & left_shift(bit);
    }
};

struct Constant_String
{
    const char* str;
    uint32 size;
    uint32 hash;

    inline bool
    operator==(const Constant_String &other) {return(other.hash == this->hash);}
};

internal inline constexpr uint32
compile_hash(const char *string, uint32 size)
{
    return ((size ? compile_hash(string, size - 1) : 2166136261u) ^ string[size]) * 16777619u;
}

internal inline constexpr Constant_String
operator""_hash(const char *string, size_t size)
{
    return(Constant_String{string, (uint32)size, compile_hash(string, (uint32)size)});
}

// fast and relatively cheap hash table
template <typename T
	  , uint32 Bucket_Count
	  , uint32 Bucket_Size
	  , uint32 Bucket_Search_Count> struct Hash_Table_Inline
{
    enum { UNINITIALIZED_HASH = 0xFFFFFFFF };
    enum { ITEM_POUR_LIMIT    = Bucket_Search_Count };

    struct Item
    {
	uint32 hash = UNINITIALIZED_HASH;
	T value = T();
    };

    struct Bucket
    {
	uint32 bucket_usage_count = 0;
	Item items[Bucket_Size] = {};
    };

    const char *map_debug_name;
    Bucket buckets[Bucket_Count] = {};

    Hash_Table_Inline(const char *name) : map_debug_name(name) {}

    void
    insert(uint32 hash, T value, const char *debug_name = "")
    {
	uint32 start_index = hash % Bucket_Count;
	uint32 limit = start_index + ITEM_POUR_LIMIT;
	for (Bucket *bucket = &buckets[start_index]
		 ; bucket->bucket_usage_count != Bucket_Size && start_index < limit
		 ; ++bucket)
	{
	    for (uint32 bucket_item = 0
		     ; bucket_item < Bucket_Size
		     ; ++bucket_item)
	    {
		Item *item = &bucket->items[bucket_item];
		if (item->hash == UNINITIALIZED_HASH)
		{
		    /* found a slot for the object */
		    item->hash = hash;
		    item->value = value;
		    return;
		}
	    }
	    OUTPUT_DEBUG_LOG("%s -> %s%s\n", map_debug_name, "hash bucket filled : need bigger buckets! - item name : ", debug_name);
	}

	OUTPUT_DEBUG_LOG("%s -> %s%s\n", map_debug_name, "couldn't fit item into hash because of filled buckets - item name : ", debug_name);
	assert(false);
    }

    void
    remove(uint32 hash)
    {
	uint32 start_index = hash % Bucket_Count;
	uint32 limit = start_index + ITEM_POUR_LIMIT;
	for (Bucket *bucket = &buckets[start_index]
		 ; bucket->bucket_usage_count != Bucket_Size && start_index < limit
		 ; ++bucket)
	{
	    for (uint32 bucket_item = 0
		     ; bucket_item < Bucket_Size
		     ; ++bucket_item)
	    {
		Item *item = &bucket->items[bucket_item];
		if (item->hash == UNINITIALIZED_HASH)
		{
		    item->hash = UNINITIALIZED_HASH;
		    item->value = T();
		}
	    }
	}
    }

    T 
    get(uint32 hash)
    {
	uint32 start_index = hash % Bucket_Count;
	uint32 limit = start_index + ITEM_POUR_LIMIT;
	for (Bucket *bucket = &buckets[start_index]
		 ; bucket->bucket_usage_count != Bucket_Size && start_index < limit
		 ; ++bucket)
	{
	    for (uint32 bucket_item = 0
		     ; bucket_item < Bucket_Size
		     ; ++bucket_item)
	    {
		Item *item = &bucket->items[bucket_item];
		if (item->hash != UNINITIALIZED_HASH)
		{
		    return(item->value);
		}
	    }
	}
	OUTPUT_DEBUG_LOG("%s -> %s\n", map_debug_name, "failed to find value requested from hash");
	assert(false);
	return(T());
    }
};

template <typename T>
struct Memory_Buffer_View
{
    uint32 count;
    T *buffer;
};
