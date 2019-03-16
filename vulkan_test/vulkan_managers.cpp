#include <stdlib.h>
#include <memory.h>
#include "core.hpp"
#include "vulkan_managers.hpp"

namespace Vulkan_API
{

    struct Object_Register_Info
    {
	u32 active_count = 0;
	void *p; // pointer to the actual object
	u32 size;
    };

    global_var Free_List_Allocator object_allocator;
    global_var Hash_Table_Inline<Object_Register_Info /* destroyed count */, 20, 4, 4> objects_list("map.object_removed_list");

    void
    init_manager(void)
    {
	object_allocator.start = malloc(megabytes(2));
	object_allocator.available_bytes = megabytes(2);
	memset(object_allocator.start, 0, object_allocator.available_bytes);
	
	object_allocator.free_block_head = (Free_Block_Header *)object_allocator.start;
	object_allocator.free_block_head->free_block_size = object_allocator.available_bytes;
    }
    
    Registered_Object_Base
    register_object(const Constant_String &id
		    , u32 bytes_size)
    {
	void *p = allocate_free_list(bytes_size
				     , 1
				     , ""
				     , &object_allocator);

	Object_Register_Info register_info {0, p, bytes_size};
	objects_list.insert(id.hash, register_info, "");

	return(Registered_Object_Base(p, id, bytes_size));
    }

    Registered_Object_Base
    get_object(const Constant_String &id)
    {
	Object_Register_Info *info = objects_list.get(id.hash);

	if (!info)
	{
	    OUTPUT_DEBUG_LOG("unable to find element %s\n", id.str);
	}
	
	return(Registered_Object_Base(info->p, id, info->size));
    }

    void
    remove_object(const Constant_String &id)
    {
	Object_Register_Info *info = objects_list.get(id.hash);
	if (info->active_count == 0)
	{
	    deallocate_free_list(info->p, &object_allocator);
	}
	else
	{
	    // error, cannot delete object yet
	}
    }

    void
    decrease_shared_count(const Constant_String &id)
    {
	Object_Register_Info *ri = objects_list.get(id.hash);
	--ri->active_count;
    }

    void
    increase_shared_count(const Constant_String &id)
    {
	Object_Register_Info *ri = objects_list.get(id.hash);
	++ri->active_count;
    }
    
}


