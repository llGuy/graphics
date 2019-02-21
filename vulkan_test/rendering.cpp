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
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(DESCRIPTOR_SET_LAYOUT, 20, 10, Descriptor_Set_Layout, uint8)


    
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

    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> buffer_index_map {"map.buffer_index_map"};
    global_var Object_Manager<Vulkan_API::Buffer, BUFFER_MAX_COUNT, Buffer_Stack_Type, BUFFER_STACK_MAX_REMOVED> buffer_manager;

    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> render_pass_index_map {"map.render_pass_index_map"};
    global_var Object_Manager<Vulkan_API::Render_Pass, RENDER_PASS_MAX_COUNT, Render_Pass_Stack_Type, RENDER_PASS_STACK_MAX_REMOVED> render_pass_manager;

    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> descriptor_set_layout_index_map {"map.descriptor_set_layout_index_map"};
    global_var Object_Manager<VkDescriptorSetLayout, DESCRIPTOR_SET_LAYOUT_MAX_COUNT, Descriptor_Set_Layout_Stack_Type, DESCRIPTOR_SET_LAYOUT_STACK_MAX_REMOVED> descriptor_set_layout_manager;
    
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
	return(render_pass_index_map.get(string.hash));
    }
    
    extern_impl Vulkan_API::Render_Pass *
    get_render_pass(Vulkan_Buffer_Handle handle)
    {
	return(&render_pass_manager.objects[handle]);
    }

    // descriptor set layout
    extern_impl Vulkan_Render_Pass_Handle
    add_descriptor_set_layout(const Constant_String &string)
    {
	Vulkan_Descriptor_Set_Layout_Handle descriptor_set_layout_handle = descriptor_set_layout_manager.add();
	descriptor_set_layout_index_map.insert(string.hash, descriptor_set_layout_handle, string.str);
	return(descriptor_set_layout_handle);
    }

    extern_impl Vulkan_Render_Pass_Handle
    get_descriptor_set_layout_handle(const Constant_String &string)
    {
	return(descriptor_set_layout_index_map.get(string.hash));
    }
    
    extern_impl VkDescriptorSetLayout *
    get_descriptor_set_layout(Vulkan_Buffer_Handle handle)
    {
	return(&descriptor_set_layout_manager.objects[handle]);
    }

    

    /* initialize the rendering objects */
    internal void
    init_test_render_pass(Vulkan_API::Swapchain *swapchain
			  , Vulkan_API::GPU *gpu)
    {
	/* init main renderpass */
	Vulkan_Render_Pass_Handle test_render_pass = add_render_pass("render_pass.test_render_pass"_hash);
	Vulkan_API::Render_Pass *test_render_pass_object = get_render_pass(test_render_pass);
	
	enum {COLOR_DESCRIPTION = 0, DEPTH_DESCRIPTION = 1};
	
	VkAttachmentDescription descriptions[2]		= {};
	descriptions[COLOR_DESCRIPTION].format		= swapchain->format;
	descriptions[COLOR_DESCRIPTION].samples		= VK_SAMPLE_COUNT_1_BIT;
	descriptions[COLOR_DESCRIPTION].loadOp		= VK_ATTACHMENT_LOAD_OP_CLEAR;
	descriptions[COLOR_DESCRIPTION].storeOp		= VK_ATTACHMENT_STORE_OP_STORE;
	descriptions[COLOR_DESCRIPTION].stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	descriptions[COLOR_DESCRIPTION].stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	descriptions[COLOR_DESCRIPTION].initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	descriptions[COLOR_DESCRIPTION].finalLayout	= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	descriptions[DEPTH_DESCRIPTION].format		= gpu->supported_depth_format;
	descriptions[DEPTH_DESCRIPTION].samples		= VK_SAMPLE_COUNT_1_BIT;
	descriptions[DEPTH_DESCRIPTION].loadOp		= VK_ATTACHMENT_LOAD_OP_CLEAR;
	descriptions[DEPTH_DESCRIPTION].storeOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	descriptions[DEPTH_DESCRIPTION].stencilLoadOp	= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	descriptions[DEPTH_DESCRIPTION].stencilStoreOp	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
	descriptions[DEPTH_DESCRIPTION].initialLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
	descriptions[DEPTH_DESCRIPTION].finalLayout	= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference references[2]		= {};
	references[COLOR_DESCRIPTION].attachment	= 0;
	references[COLOR_DESCRIPTION].layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	references[DEPTH_DESCRIPTION].attachment	= 1;
	references[DEPTH_DESCRIPTION].layout		= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass	= {};
	subpass.pipelineBindPoint	= VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount	= 1;
	subpass.pColorAttachments	= &references[COLOR_DESCRIPTION];
	subpass.pDepthStencilAttachment	= &references[DEPTH_DESCRIPTION];

	VkSubpassDependency dependency	= {};
	dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass		= 0;
	dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask	= 0;
	dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask	= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

	Vulkan_API::Render_Pass_Create_Params test_render_pass_params = {};
	test_render_pass_params.r_attachment_description_count = 2;
	test_render_pass_params.r_attachment_descriptions = descriptions;
	test_render_pass_params.r_subpass_count = 1;
	test_render_pass_params.r_subpasses = &subpass;
	test_render_pass_params.r_dependency_count = 1;
	test_render_pass_params.r_dependencies = &dependency;
	test_render_pass_params.r_gpu = gpu;
	
	Vulkan_API::init_render_pass(&test_render_pass_params, test_render_pass_object);
    }

    internal void
    init_descriptor_set_layout(Vulkan_API::GPU *gpu)
    {
	// create descriptor set layout in manager
	Vulkan_Descriptor_Set_Layout_Handle handle = add_descriptor_set_layout("descriptor_set_layout.test_descriptor_set_layout"_hash);
	
	VkDescriptorSetLayoutBinding ubo_binding	= {};
	ubo_binding.binding				= 0;
	ubo_binding.descriptorType			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	ubo_binding.descriptorCount			= 1;
	ubo_binding.stageFlags				= VK_SHADER_STAGE_VERTEX_BIT;
	ubo_binding.pImmutableSamplers			= nullptr;

	VkDescriptorSetLayoutBinding sampler_binding	= {};
	sampler_binding.binding				= 1;
	sampler_binding.descriptorCount			= 1;
	sampler_binding.descriptorType			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	sampler_binding.pImmutableSamplers		= nullptr;
	sampler_binding.stageFlags			= VK_SHADER_STAGE_FRAGMENT_BIT;

	VkDescriptorSetLayoutBinding bindings[] = {ubo_binding, sampler_binding};

	VkDescriptorSetLayoutCreateInfo layout_info	= {};
	layout_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layout_info.bindingCount			= 2;
	layout_info.pBindings				= bindings;

	VkDescriptorSetLayout *layout = get_descriptor_set_layout(handle);
	VK_CHECK(vkCreateDescriptorSetLayout(gpu->logical_device, &layout_info, nullptr, layout));
    }
    
    extern_impl void
    init_rendering_state(Vulkan_API::State *vulkan_state
			 , Rendering_Objects_Handle_Cache *cache)
    {
	init_test_render_pass(&vulkan_state->swapchain
			      , &vulkan_state->gpu);
	
	init_descriptor_set_layout(&vulkan_state->gpu);

	cache->test_render_pass = get_render_pass_handle("render_pass.test_render_pass"_hash);
	cache->descriptor_set_layout = get_descriptor_set_layout_handle("descriptor_set_layout.test_descriptor_set_layout"_hash);
    }

}
