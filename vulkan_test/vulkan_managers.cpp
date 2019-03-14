#include <memory.h>
#include "vulkan_managers.hpp"

namespace Vulkan_API
{

    // macro to automate the writing of "[NAME]_MAX_COUNT", "[Name]_Stack_Type", "[NAME]_STACK_MAX_REMOVED" for creating the template paramaters of the instances of the Object_Manager structs
#define MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(const_name, max_count, max_removed, not_const_name, stack_type) \
    global_var constexpr u32 const_name##_MAX_COUNT = max_count;	\
    global_var constexpr u32 const_name##_STACK_MAX_REMOVED = max_removed; \
    using not_const_name##_Stack_Type = stack_type;
    
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(BUFFER, 64, 20, Buffer, u8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(RENDER_PASS, 20, 10, Render_Pass, u8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(DESCRIPTOR_SET_LAYOUT, 20, 10, Descriptor_Set_Layout, u8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(MODEL, 30, 10, Model, u8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(GRAPHICS_PIPELINE, 30, 10, Graphics_Pipeline, u8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(COMMAND_POOL, 5, 5, Command_Pool, u8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(IMAGE2D, 20, 6, Image2D, u8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(FRAMEBUFFER, 20, 6, Framebuffer, u8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(DESCRIPTOR_SET, 20, 6, Descriptor_Set, u8)

    
    template <typename I_Type /* which type of int */
	      , u32 N> struct Removed_Stack
    {
	static constexpr u32 MAX = N;

	I_Type items[N] = {};
	u32 head = 0;

	u32
	push(I_Type index)
	{
	    u32 handle = head;
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
	      , u32 N
	      , typename Stack_I_Type
	      , u32 Stack_N> struct Object_Manager
    {
	using Object_Type = T;
	
	T objects[N];
	u32 object_count;

	Removed_Stack<Stack_I_Type
		      , Stack_N> removed_objects;
 
	u32 /* handle */
	add(void)
	{
	    u32 handle = object_count++;

	    if (object_count == N)
	    {
		OUTPUT_DEBUG_LOG("%s\n", "manager object array is full!");
	    }
	    
	    return(handle);
	}
    };

    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> buffer_index_map {"map.buffer_index_map"};
    global_var Object_Manager<Vulkan_API::Buffer, BUFFER_MAX_COUNT, Buffer_Stack_Type, BUFFER_STACK_MAX_REMOVED> buffer_manager;

    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> render_pass_index_map {"map.render_pass_index_map"};
    global_var Object_Manager<Vulkan_API::Render_Pass, RENDER_PASS_MAX_COUNT, Render_Pass_Stack_Type, RENDER_PASS_STACK_MAX_REMOVED> render_pass_manager;

    global_var Object_Manager<VkDescriptorSetLayout, DESCRIPTOR_SET_LAYOUT_MAX_COUNT, Descriptor_Set_Layout_Stack_Type, DESCRIPTOR_SET_LAYOUT_STACK_MAX_REMOVED> descriptor_set_layout_manager;
    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> descriptor_set_layout_index_map {"map.descriptor_set_layout_index_map"};

    global_var Object_Manager<Model, MODEL_MAX_COUNT, Model_Stack_Type, MODEL_STACK_MAX_REMOVED> model_manager;
    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> model_index_map {"map.model_index_map"};

    global_var Object_Manager<Graphics_Pipeline, GRAPHICS_PIPELINE_MAX_COUNT, Graphics_Pipeline_Stack_Type, GRAPHICS_PIPELINE_STACK_MAX_REMOVED> graphics_pipeline_manager;
    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> graphics_pipeline_index_map {"map.graphics_pipeline_index_map"};

    global_var Object_Manager<VkCommandPool, COMMAND_POOL_MAX_COUNT, Command_Pool_Stack_Type, COMMAND_POOL_STACK_MAX_REMOVED> command_pool_manager;
    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> command_pool_index_map {"map.command_pool_index_map"};

    global_var Object_Manager<Image2D, IMAGE2D_MAX_COUNT, Image2D_Stack_Type, IMAGE2D_STACK_MAX_REMOVED> image2D_manager;
    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> image2D_index_map {"map.image2D_index_map"};

    global_var Object_Manager<Framebuffer, FRAMEBUFFER_MAX_COUNT, Framebuffer_Stack_Type, FRAMEBUFFER_STACK_MAX_REMOVED> framebuffer_manager;
    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> framebuffer_index_map {"map.framebuffer_index_map"};

    global_var Object_Manager<Descriptor_Set, DESCRIPTOR_SET_MAX_COUNT, Descriptor_Set_Stack_Type, DESCRIPTOR_SET_STACK_MAX_REMOVED> descriptor_set_manager;
    global_var Hash_Table_Inline<u32 /*index of item in the manager struct*/, 20, 8, 3> descriptor_set_index_map {"map.descriptor_set_index_map"};
    

    
    template <typename Manager_Type
	      , typename Hash_Table_Type> u32
    add_template(Manager_Type *manager
		 , Hash_Table_Type *name_table
		 , const Constant_String &string)
    {
	u32 handle = manager->add();
	name_table->insert(string.hash, handle, string.str);
	return(handle);
    }

    template <typename Manager_Type
	      , typename Hash_Table_Type> u32
    get_handle_template(Manager_Type *manager
			, Hash_Table_Type *name_table
			, const Constant_String &string)
    {
	return(*name_table->get(string.hash));
    }

    template <typename Manager_Type> typename Manager_Type::Object_Type *
    get_template(Manager_Type *manager
		 , u32 handle)
    {
	return(&manager->objects[handle]);
    }
    

    // buffer funcion implementations
    Buffer_Handle
    add_buffer(const Constant_String &string)
    {
	return(add_template(&buffer_manager
			    , &buffer_index_map
			    , string));
    }
    Buffer_Handle
    get_buffer_handle(const Constant_String &string)
    {
	return(get_handle_template(&buffer_manager
				   , &buffer_index_map
				   , string));
    }
    Buffer *
    get_buffer(Buffer_Handle handle)
    {
	return(get_template(&buffer_manager
			    , handle));
    }

    Buffer *
    get_buffer(const Constant_String &handle)
    {
	return(get_template(&buffer_manager
			    , get_handle_template(&buffer_manager
						  , &buffer_index_map
						  , handle)));
    }
    


    // render pass function implementations
    Render_Pass_Handle
    add_render_pass(const Constant_String &string)
    {
	return(add_template(&render_pass_manager
			    , &render_pass_index_map
			    , string));
    }
    
    Render_Pass_Handle
    get_render_pass_handle(const Constant_String &string)
    {
	return(get_handle_template(&render_pass_manager
				   , &render_pass_index_map
				   , string));
    }
    
    Render_Pass *
    get_render_pass(Render_Pass_Handle handle)
    {
	return(get_template(&render_pass_manager
			    , handle));
    }

    Render_Pass *
    get_render_pass(const Constant_String &handle)
    {
	return(get_template(&render_pass_manager
			    , get_handle_template(&render_pass_manager
						  , &render_pass_index_map
						  , handle)));
    }    
    


    // descriptor set layout function implementations
    Descriptor_Set_Layout_Handle
    add_descriptor_set_layout(const Constant_String &string)
    {
	return(add_template(&descriptor_set_layout_manager
			    , &descriptor_set_layout_index_map
			    , string));
    }
    Descriptor_Set_Layout_Handle
    get_descriptor_set_layout_handle(const Constant_String &string)
    {
	return(get_handle_template(&descriptor_set_layout_manager
				   , &descriptor_set_layout_index_map
				   , string));
    }
    VkDescriptorSetLayout *
    get_descriptor_set_layout(Descriptor_Set_Layout_Handle handle)
    {
	return(get_template(&descriptor_set_layout_manager
			    , handle));
    }

    VkDescriptorSetLayout *
    get_descriptor_set_layout(const Constant_String &handle)
    {
	return(get_template(&descriptor_set_layout_manager
			    , get_handle_template(&descriptor_set_layout_manager
						  , &descriptor_set_layout_index_map
						  , handle)));
    }    


    
    // model stuff
    Model_Handle
    add_model(const Constant_String &string)
    {
	return(add_template(&model_manager
			    , &model_index_map
			    , string));
    }
    Model_Handle
    get_model_handle(const Constant_String &string)
    {
	return(get_handle_template(&model_manager
				   , &model_index_map
				   , string));
    }
    Model *
    get_model(Model_Handle handle)
    {
	return(get_template(&model_manager
			    , handle));
    }

    Model *
    get_model(const Constant_String &handle)
    {
	return(get_template(&model_manager
			    , get_handle_template(&model_manager
						  , &model_index_map
						  , handle)));
    }    



    // graphics pipeline stuff
    Graphics_Pipeline_Handle
    add_graphics_pipeline(const Constant_String &string)
    {
	return(add_template(&graphics_pipeline_manager
			    , &graphics_pipeline_index_map
			    , string));
    }

    Graphics_Pipeline_Handle
    get_graphics_pipeline_handle(const Constant_String &string)
    {
	return(get_handle_template(&graphics_pipeline_manager
				   , &graphics_pipeline_index_map
				   , string));
    }	
    
    Graphics_Pipeline *
    get_graphics_pipeline(Graphics_Pipeline_Handle handle)
    {
	return(get_template(&graphics_pipeline_manager
			    , handle));
    }

    Graphics_Pipeline *
    get_graphics_pipeline(const Constant_String &handle)
    {
	return(get_template(&graphics_pipeline_manager
			    , get_handle_template(&graphics_pipeline_manager
						  , &graphics_pipeline_index_map
						  , handle)));
    }    
    


    // command pool stuff
    Command_Pool_Handle
    add_command_pool(const Constant_String &string)
    {
	return(add_template(&command_pool_manager
			    , &command_pool_index_map
			    , string));
    }

    Command_Pool_Handle
    get_command_pool_handle(const Constant_String &string)
    {
	return(get_handle_template(&command_pool_manager
				   , &command_pool_index_map
				   , string));
    }
    
    VkCommandPool *
    get_command_pool(Command_Pool_Handle handle)
    {
	return(get_template(&command_pool_manager
			    , handle));
    }

    VkCommandPool *
    get_command_pool(const Constant_String &handle)
    {
	return(get_template(&command_pool_manager
			    , get_handle_template(&command_pool_manager
						  , &command_pool_index_map
						  , handle)));
    }    



    // image2D stuff
    Image2D_Handle
    add_image2D(const Constant_String &string)
    {
	return(add_template(&image2D_manager
			    , &image2D_index_map
			    , string));
    }

    Image2D_Handle
    get_image2D_handle(const Constant_String &string)
    {
	return(get_handle_template(&image2D_manager
				   , &image2D_index_map
				   , string));
    }
    
    Image2D *
    get_image2D(Image2D_Handle handle)
    {
	return(get_template(&image2D_manager
			    , handle));
    }

    Image2D *
    get_image2D(const Constant_String &handle)
    {
	return(get_template(&image2D_manager
			    , get_handle_template(&image2D_manager
						  , &image2D_index_map
						  , handle)));
    }



    Framebuffer_Handle
    add_framebuffer(const Constant_String &string)
    {
	return(add_template(&framebuffer_manager
			    , &framebuffer_index_map
			    , string));
    }

    Framebuffer_Handle
    get_framebuffer_handle(const Constant_String &string)
    {
	return(get_handle_template(&framebuffer_manager
				   , &framebuffer_index_map
				   , string));
    }
    
    Framebuffer *
    get_framebuffer(Image2D_Handle handle)
    {
	return(get_template(&framebuffer_manager
			    , handle));
    }

    Framebuffer *
    get_framebuffer(const Constant_String &handle)
    {
	return(get_template(&framebuffer_manager
			    , get_handle_template(&framebuffer_manager
						  , &framebuffer_index_map
						  , handle)));
    }



    Descriptor_Set_Handle
    add_descriptor_set(const Constant_String &string)
    {
	return(add_template(&descriptor_set_manager
			    , &descriptor_set_index_map
			    , string));
    }

    Descriptor_Set_Handle
    get_descriptor_set_handle(const Constant_String &string)
    {
	return(get_handle_template(&descriptor_set_manager
				   , &descriptor_set_index_map
				   , string));
    }
    
    Descriptor_Set *
    get_descriptor_set(Descriptor_Set_Handle handle)
    {
	return(get_template(&descriptor_set_manager
			    , handle));
    }

    Descriptor_Set *
    get_descriptor_Set(const Constant_String &handle)
    {
	return(get_template(&descriptor_set_manager
			    , get_handle_template(&descriptor_set_manager
						  , &descriptor_set_index_map
						  , handle)));
    }




    

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


