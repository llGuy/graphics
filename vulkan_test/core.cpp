#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "vulkan.hpp"
#include "core.hpp"
#include "graphics.hpp"

#define DEBUG_FILE ".debug"

#define STACK_ALLOCATOR_GLOBAL_SIZE 0xffff
extern_impl Stack_Allocator stack_allocator_global;
extern_impl Debug_Output output_file;
extern_impl GLFWwindow *window;
extern_impl Free_List_Allocator free_list_allocator_global;

internal bool running;

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
	       , Alignment alignment
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

    return(start_address + sizeof(Stack_Allocation_Header));
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
    else if (allocator->allocation_count == 0)
    {
	OUTPUT_DEBUG_LOG("%s\n", "stack already cleared : pop stack call ignored");
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

internal void
init_free_list_allocator_head(Free_List_Allocator *allocator = &free_list_allocator_global)
{
    allocator->free_block_head = (Free_Block_Header *)allocator->start;
    allocator->free_block_head->free_block_size = allocator->available_bytes;
}

extern_impl void *
allocate_free_list(uint32 allocation_size
		   , Alignment alignment
		   , const char *name
		   , Free_List_Allocator *allocator)
{
    uint32 total_allocation_size = allocation_size + sizeof(Free_List_Allocation_Header);
    // find best fit free block
    // TODO(luc) : make free list allocator adjust the smallest free block according to the alignment as well
    Free_Block_Header *previous_free_block = nullptr;
    Free_Block_Header *smallest_free_block = allocator->free_block_head;
    for (Free_Block_Header *header = allocator->free_block_head
	     ; header
	     ; header = header->next_free_block)
    {
	if (header->free_block_size >= total_allocation_size)
	{
	    if (smallest_free_block->free_block_size >= header->free_block_size)
	    {
		smallest_free_block = header;
		break;
	    }
	}
	previous_free_block = header;
    }
    Free_Block_Header *next = smallest_free_block->next_free_block;
    if (previous_free_block)
    {
	uint32 previous_smallest_block_size = smallest_free_block->free_block_size;
	previous_free_block->next_free_block = (Free_Block_Header *)(((byte *)smallest_free_block) + total_allocation_size);
	previous_free_block->next_free_block->free_block_size = previous_smallest_block_size - total_allocation_size;
	previous_free_block->next_free_block->next_free_block = next;
    }
    else
    {
	Free_Block_Header *new_block = (Free_Block_Header *)(((byte *)smallest_free_block) + total_allocation_size);
	allocator->free_block_head = new_block;
	new_block->free_block_size = smallest_free_block->free_block_size - total_allocation_size;
	new_block->next_free_block = smallest_free_block->next_free_block;
    }
    
    Free_List_Allocation_Header *header = (Free_List_Allocation_Header *)smallest_free_block;
    header->size = allocation_size;
#if DEBUG
    header->name = name;
#endif
    return((byte *)header + sizeof(Free_List_Allocation_Header));
}

// TODO(luc) : optimize free list allocator so that all the blocks are stored in order
extern_impl void
deallocate_free_list(void *pointer
		     , Free_List_Allocator *allocator)
{
    Free_List_Allocation_Header *allocation_header = (Free_List_Allocation_Header *)((byte *)pointer - sizeof(Free_List_Allocation_Header));
    Free_Block_Header *new_free_block_header = (Free_Block_Header *)allocation_header;
    new_free_block_header->free_block_size = allocation_header->size + sizeof(Free_Block_Header);

    Free_Block_Header *previous = nullptr;
    Free_Block_Header *current_header = allocator->free_block_head;

    Free_Block_Header *viable_prev = nullptr;
    Free_Block_Header *viable_next = nullptr;
    
    // check if possible to merge free blocks
    bool merged = false;
    for (; current_header
	     ; current_header = current_header->next_free_block)
    {
	if (new_free_block_header > previous && new_free_block_header < current_header)
	{
	    viable_prev = previous;
	    viable_next = current_header;
	}
	
	// check if free blocks will overlap
	// does the header go over the newly freed header
	if (current_header->free_block_size + (byte *)current_header >= (byte *)new_free_block_header
	    && (byte *)current_header < (byte *)new_free_block_header)
	{
	    current_header->free_block_size = ((byte *)new_free_block_header + new_free_block_header->free_block_size) - (byte *)current_header;
	    new_free_block_header = current_header;
	    previous = current_header;
	    merged = true;
	    continue;
	}
	// does the newly freed header go over the header
	if ((byte *)current_header <= (byte *)new_free_block_header + new_free_block_header->free_block_size
	    && (byte *)current_header > (byte *)new_free_block_header)
	{
	    // if current header is not the head of the list
	    new_free_block_header->free_block_size = (byte *)((byte *)current_header + current_header->free_block_size) - (byte *)new_free_block_header; 
	    /*if (previous)
	    {
		previous->next_free_block = new_free_block_header;
		}*/
	    if (!previous)
	    {
		allocator->free_block_head = new_free_block_header;
	    }
	    new_free_block_header->next_free_block = current_header->next_free_block;
	    merged = true;
	    
	    previous = current_header;
	    continue;
	}

	if (merged) return;
	
	previous = current_header;
    }

    if (merged) return;

    // put the blocks in order if no blocks merged
    // if viable_prev == nullptr && viable_next != nullptr -> new block should be before the head
    // if viable_prev != nullptr && viable_next != nullptr -> new block should be between prev and next
    // if viable_prev == nullptr && viable_next == nullptr -> new block should be after the last header
    if (!viable_prev && viable_next)
    {
	Free_Block_Header *old_head = allocator->free_block_head;
	allocator->free_block_head = new_free_block_header;
	allocator->free_block_head->next_free_block = old_head;
    }
    else if (viable_prev && viable_next)
    {
	viable_prev->next_free_block = new_free_block_header;
	new_free_block_header->next_free_block = viable_next;
    }
    else if (!viable_prev && !viable_next)
    {
	// at the end of the loop, previous is that last current before current = nullptr
	previous->next_free_block = new_free_block_header;
    }
}

int32
main(int32 argc
     , char * argv[])
{
    try
    {
	open_debug_file();

	OUTPUT_DEBUG_LOG("%s\n", "starting session");
	
	stack_allocator_global.start = stack_allocator_global.current = malloc(megabytes(8));
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

	init_vk(window);
	
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
	OUTPUT_DEBUG_LOG("%s\n", "CRASH!!!");
	close_debug_file();
    }
    
    return(0);
}

