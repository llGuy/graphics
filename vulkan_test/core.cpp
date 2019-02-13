#include <iostream>
#include "core.hpp"

#include <stdio.h>
#include <cassert>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#define DEBUG_FILE "debug.txt"

#define STACK_ALLOCATOR_GLOBAL_SIZE 0xffff
extern_impl Stack_Allocator stack_allocator_global;
extern_impl Debug_Output output_file;
extern_impl GLFWwindow *window;

global bool running;

internal void
open_debug_file(void)
{
    output_file.fp = fopen(DEBUG_FILE, "w+");
    assert(output_file.fp >= NULL);
}

internal void
close_debug_file(void)
{
    fclose(output_file.fp);
}

extern void
output_debug(char *format
	     , ...)
{
    va_list arg_list;
    
    va_start(arg_list, format);

    fprintf(output_file.fp
	    , format
	    , arg_list);

    va_end(arg_list);

    fflush(output_file.fp);
}

extern_impl void *
allocate_stack(uint32 allocation_size
	       , uint8 alignment
	       , Stack_Allocator *allocator)
{
    byte *would_be_address;

    if (allocator->allocation_count == 0) would_be_address = (uint8 *)allocator->current + sizeof(Stack_Allocation_Header);
    else would_be_address = (uint8 *)allocator->current
					      + sizeof(Stack_Allocation_Header) * 2 
	                                      + ((Stack_Allocation_Header *)(allocator->current))->size;
    
    // calculate the aligned address that is needed
    uint8 alignment_adjustment = get_alignment_adjust(would_be_address
						      , alignment);

    // start address of (header + allocation)
    byte *start_address = (would_be_address + alignment_adjustment) - sizeof(Stack_Allocation_Header);
    assert((start_address + sizeof(Stack_Allocation_Header) + allocation_size) < (uint8 *)allocator->start + allocator->capacity);

    Stack_Allocation_Header *header = (Stack_Allocation_Header *)start_address;
    header->size = allocation_size;
    header->prev = allocator->allocation_count == 0 ? nullptr : (Stack_Allocation_Header *)allocator->current;

    allocator->current = (void *)header;
    ++(allocator->allocation_count);

    return start_address + sizeof(Stack_Allocation_Header);
}

extern_impl void
extend_stack_top(uint32 extension_size
	       , Stack_Allocator *allocator)
{
    Stack_Allocation_Header *current_header = (Stack_Allocation_Header *)allocator->current;
    current_header->size += extension_size;
}

extern_impl void
pop_stack(Stack_Allocator *allocator)
{
    Stack_Allocation_Header *current_header = (Stack_Allocation_Header *)allocator->current;
    allocator->current = current_header->prev;
}

int32
main(int32 argc
     , char * argv[])
{
    open_debug_file();
    output_debug("vector %f %f %f\n", 0.0f, 1.0f, 0.0f);
    
    stack_allocator_global.start = stack_allocator_global.current =  malloc(megabytes(4));
    stack_allocator_global.capacity = STACK_ALLOCATOR_GLOBAL_SIZE;
    
    if (!glfwInit())
    {
	return(0);
    }

    window = glfwCreateWindow(1280
			      , 720
			      , "Vulkan App"
			      , NULL
			      , NULL);

    while(!glfwWindowShouldClose(window))
    {
	glfwPollEvents();
    }

    output_debug("done\n");
    
    close_debug_file();
    
    return(0);
}

