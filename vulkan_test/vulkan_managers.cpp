#include "vulkan_managers.hpp"

namespace Vulkan_API
{

    // macro to automate the writing of "[NAME]_MAX_COUNT", "[Name]_Stack_Type", "[NAME]_STACK_MAX_REMOVED" for creating the template paramaters of the instances of the Object_Manager structs
#define MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(const_name, max_count, max_removed, not_const_name, stack_type) \
    global_var constexpr uint32 const_name##_MAX_COUNT = max_count;	\
    global_var constexpr uint32 const_name##_STACK_MAX_REMOVED = max_removed; \
    using not_const_name##_Stack_Type = stack_type;
    
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(BUFFER, 64, 20, Buffer, uint8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(RENDER_PASS, 20, 10, Render_Pass, uint8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(DESCRIPTOR_SET_LAYOUT, 20, 10, Descriptor_Set_Layout, uint8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(MODEL, 30, 10, Model, uint8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(GRAPHICS_PIPELINE, 30, 10, Graphics_Pipeline, uint8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(COMMAND_POOL, 5, 5, Command_Pool, uint8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(IMAGE2D, 20, 6, Image2D, uint8)
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(FRAMEBUFFER, 20, 6, Framebuffer, uint8)

    
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
	using Object_Type = T;
	
	T objects[N];
	uint32 object_count;

	Removed_Stack<Stack_I_Type
		      , Stack_N> removed_objects;
 
	uint32 /* handle */
	add(void)
	{
	    uint32 handle = object_count++;

	    if (object_count == N)
	    {
		OUTPUT_DEBUG_LOG("%s\n", "manager object array is full!");
	    }
	    
	    return(handle);
	}
    };

    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> buffer_index_map {"map.buffer_index_map"};
    global_var Object_Manager<Vulkan_API::Buffer, BUFFER_MAX_COUNT, Buffer_Stack_Type, BUFFER_STACK_MAX_REMOVED> buffer_manager;

    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> render_pass_index_map {"map.render_pass_index_map"};
    global_var Object_Manager<Vulkan_API::Render_Pass, RENDER_PASS_MAX_COUNT, Render_Pass_Stack_Type, RENDER_PASS_STACK_MAX_REMOVED> render_pass_manager;

    global_var Object_Manager<VkDescriptorSetLayout, DESCRIPTOR_SET_LAYOUT_MAX_COUNT, Descriptor_Set_Layout_Stack_Type, DESCRIPTOR_SET_LAYOUT_STACK_MAX_REMOVED> descriptor_set_layout_manager;
    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> descriptor_set_layout_index_map {"map.descriptor_set_layout_index_map"};

    global_var Object_Manager<Model, MODEL_MAX_COUNT, Model_Stack_Type, MODEL_STACK_MAX_REMOVED> model_manager;
    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> model_index_map {"map.model_index_map"};

    global_var Object_Manager<Graphics_Pipeline, GRAPHICS_PIPELINE_MAX_COUNT, Graphics_Pipeline_Stack_Type, GRAPHICS_PIPELINE_STACK_MAX_REMOVED> graphics_pipeline_manager;
    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> graphics_pipeline_index_map {"map.graphics_pipeline_index_map"};

    global_var Object_Manager<VkCommandPool, COMMAND_POOL_MAX_COUNT, Command_Pool_Stack_Type, COMMAND_POOL_STACK_MAX_REMOVED> command_pool_manager;
    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> command_pool_index_map {"map.command_pool_index_map"};

    global_var Object_Manager<Image2D, IMAGE2D_MAX_COUNT, Image2D_Stack_Type, IMAGE2D_STACK_MAX_REMOVED> image2D_manager;
    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> image2D_index_map {"map.image2D_index_map"};

    global_var Object_Manager<Framebuffer, FRAMEBUFFER_MAX_COUNT, Framebuffer_Stack_Type, FRAMEBUFFER_STACK_MAX_REMOVED> framebuffer_manager;
    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> framebuffer_index_map {"map.framebuffer_index_map"};


    
    template <typename Manager_Type
	      , typename Hash_Table_Type> uint32
    add_template(Manager_Type *manager
		 , Hash_Table_Type *name_table
		 , const Constant_String &string)
    {
	uint32 handle = manager->add();
	name_table->insert(string.hash, handle, string.str);
	return(handle);
    }

    template <typename Manager_Type
	      , typename Hash_Table_Type> uint32
    get_handle_template(Manager_Type *manager
			, Hash_Table_Type *name_table
			, const Constant_String &string)
    {
	return(name_table->get(string.hash));
    }

    template <typename Manager_Type> typename Manager_Type::Object_Type *
    get_template(Manager_Type *manager
		 , uint32 handle)
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
    Vulkan_API::Buffer *
    get_buffer(Buffer_Handle handle)
    {
	return(get_template(&buffer_manager
			    , handle));
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
    Vulkan_API::Render_Pass *
    get_render_pass(Render_Pass_Handle handle)
    {
	return(get_template(&render_pass_manager
			    , handle));
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
    
}


