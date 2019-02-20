#include "rendering.hpp"

namespace Rendering
{

    // macro to automate the writing of "[NAME]_MAX_COUNT", "[Name]_Stack_Type", "[NAME]_STACK_MAX_REMOVED" for creating the template paramaters of the instances of the Object_Manager structs
#define MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(const_name, max_count, max_removed, not_const_name, stack_type) \
    global_var constexpr uint32 const_name##_MAX_COUNT = max_count;			\
    global_var constexpr uint32 const_name##_STACK_MAX_REMOVED = max_removed;		\
    using not_const_name##_Stack_Type = stack_type;					

    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(BUFFER, 64, 20, Buffer, uint8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(RENDER_PASS, 20, 10, Render_Pass, uint8)


    
    template <typename I_Type /* which type of int */
	      , uint32 N> struct Removed_Stack
    {
	static constexpr uint32 MAX = N;

	I_Type items[N] = {};
	uint32 head = 0;

	uint32
	push(I_Type index)
	{
	    uint32 handle = head;
#if DEBUG
	    if (head == MAX) { OUTPUT_DEBUG_LOG("%s\n", "cannot push to stack anymore"); }
	    else items[head++] = index;
#else
	    items[head++] = index;
#endif
	    return(handle);
	}

	I_Type
	pop(void)
	{
	    if (head != 0) return(items[head--]);
	    else return(I_Type(MAX));
	}
    };

    template <typename T
	      , uint32 N
	      , typename Stack_I_Type
	      , uint32 Stack_N> struct Object_Manager
    {
	T objects[N];
	uint32 object_count;
	
	Removed_Stack<Stack_I_Type
		      , Stack_N> removed_objects;

	uint32 /* handle */
	add(void)
	{
	    uint32 handle = object_count;

	    if (object_count == N)
	    {
		OUTPUT_DEBUG_LOG("%s\n", "manager object array is full!");
	    }
	    
	    return(handle);
	}
    };

    global_var Object_Manager<Vulkan_API::Buffer, BUFFER_MAX_COUNT, Buffer_Stack_Type, BUFFER_STACK_MAX_REMOVED> buffer_manager;

    Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> render_pass_index_map {"map.render_pass_index_map"};
    global_var Object_Manager<Vulkan_API::Render_Pass, RENDER_PASS_MAX_COUNT, Render_Pass_Stack_Type, RENDER_PASS_STACK_MAX_REMOVED> render_pass_manager;

    // buffer functions
    // TODO(luc) : make these functions actually do stuff in the future
    extern_impl Vulkan_Buffer_Handle
    add_buffer(const Constant_String &string)
    {
	return(0);
    }

    extern_impl Vulkan_Buffer_Handle
    get_buffer_handle(const Constant_String &string)
    {
	return(0);
    }
    
    extern_impl Vulkan_API::Buffer *
    get_buffer(Vulkan_Buffer_Handle handle)
    {
	return(0);
    }

    // render pass functions
    
    extern_impl Vulkan_Render_Pass_Handle
    add_render_pass(const Constant_String &string)
    {
	Vulkan_Render_Pass_Handle render_pass_handle = render_pass_manager.add();
	render_pass_index_map.insert(string.hash, render_pass_handle, string.str);
	return(render_pass_handle);
    }

    extern_impl Vulkan_Render_Pass_Handle
    get_render_pass_handle(const Constant_String &string)
    {
	return(0);
    }
    
    extern_impl Vulkan_API::Render_Pass *
    get_render_pass(Vulkan_Buffer_Handle handle)
    {
	return(0);
    }

}
