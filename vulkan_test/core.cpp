#include <iostream>
#include "core.hpp"

#include <stdio.h>
#include <cassert>
#include <windows.h>
#include "graphics.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include <iostream>

#define DEBUG_FILE ".debug"

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
output_debug(const char *format
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
	       , const char *name
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
    
#if DEBUG
    header->allocation_name = name;
    OUTPUT_DEBUG_LOG("stack allocation for \"%s\"\n", name);
#endif

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
    
#if DEBUG
    if (allocator->allocation_count == 1)
    {
	OUTPUT_DEBUG_LOG("cleared stack : last allocation was \"%s\"\n", current_header->allocation_name);
    }
    else
    {
	OUTPUT_DEBUG_LOG("poping allocation \"%s\" -> new head is \"%s\"\n", current_header->allocation_name
		     , ((Stack_Allocation_Header *)(current_header->prev))->allocation_name);
    }
#endif

    if (allocator->allocation_count == 1) allocator->current = allocator->start;
    else allocator->current = current_header->prev;
    --(allocator->allocation_count);
}

int32
main(int32 argc
     , char * argv[])
{
    try
    {
	open_debug_file();

	OUTPUT_DEBUG_LOG("%s\n", "starting session");

	stack_allocator_global.start = stack_allocator_global.current =  malloc(megabytes(8));
	stack_allocator_global.capacity = STACK_ALLOCATOR_GLOBAL_SIZE;

	OUTPUT_DEBUG_LOG("stack allocator start address : %p\n", stack_allocator_global.current);
    
	if (!glfwInit())
	{
	    return(0);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	window = glfwCreateWindow(1280
				  , 720
				  , "Vulkan App"
				  , NULL
				  , NULL);

	init_vk();

	uint32 current_frame = 0;
	while(!glfwWindowShouldClose(window))
	{
	    glfwPollEvents();
	    draw_frame();
	}

	OUTPUT_DEBUG_LOG("stack allocator start address is : %p\n", stack_allocator_global.current);
	OUTPUT_DEBUG_LOG("stack allocator allocated %d bytes\n", (uint32)((uint8 *)stack_allocator_global.current - (uint8 *)stack_allocator_global.start));
    
	OUTPUT_DEBUG_LOG("finished session\n", 1);

	close_debug_file();

	destroy_vk();

	glfwDestroyWindow(window);
	glfwTerminate();
    }
    catch(...)
    {
	OUTPUT_DEBUG_LOG("CRASH!!!\n");
	close_debug_file();
    }
    
    return(0);
}

