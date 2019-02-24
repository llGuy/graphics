#include <glm/glm.hpp>
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
    MAKE_OBJECT_MANAGER_TMP_PARAM_GROUP(VULKAN_MODEL, 30, 10, Vulkan_Model, uint8)

    
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

    global_var Object_Manager<VkDescriptorSetLayout, DESCRIPTOR_SET_LAYOUT_MAX_COUNT, Descriptor_Set_Layout_Stack_Type, DESCRIPTOR_SET_LAYOUT_STACK_MAX_REMOVED> descriptor_set_layout_manager;
    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> descriptor_set_layout_index_map {"map.descriptor_set_layout_index_map"};

    global_var Object_Manager<Vulkan_Model, VULKAN_MODEL_MAX_COUNT, Vulkan_Model_Stack_Type, VULKAN_MODEL_STACK_MAX_REMOVED> vulkan_model_manager;
    global_var Hash_Table_Inline<uint32 /*index of item in the manager struct*/, 20, 8, 3> vulkan_model_index_map {"map.vulkan_model_index_map"};
    
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

    // model
    extern_impl Vulkan_Model_Handle
    add_vulkan_model(const Constant_String &string)
    {
	Vulkan_Model_Handle model_handle = vulkan_model_manager.add();
	vulkan_model_index_map.insert(string.hash, model_handle, string.str);
	return(model_handle);
    }

    extern_impl Vulkan_Model_Handle
    get_vulkan_model_handle(const Constant_String &string)
    {
	return(vulkan_model_index_map.get(string.hash));
    }
    
    extern_impl Vulkan_Model *
    get_vulkan_model(Vulkan_Model_Handle handle)
    {
	return(&vulkan_model_manager.objects[handle]);
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
    init_model_info(void)
    {
	Vulkan_Model_Handle handle = add_vulkan_model("vulkan_model.test_model"_hash);
	Vulkan_Model *model = get_vulkan_model(handle);

	model->attribute_count = 3;
	model->attributes_buffer = (Vulkan_Model_Binding::Attribute *)allocate_free_list(sizeof(Vulkan_Model_Binding::Attribute) * model->attribute_count
										  , Alignment(1)
										  , "test_model_attribute_list_allocation");
	model->binding_count = 1;
	model->bindings = (Vulkan_Model_Binding *)allocate_free_list(sizeof(Vulkan_Model_Binding) * model->binding_count
								     , Alignment(1)
								     , "test_model_binding_list_allocation");

	struct Vertex { glm::vec3 pos; glm::vec3 color; glm::vec2 uvs; };
	
	// only one binding
	Vulkan_Model_Binding *binding = model->bindings;
	binding->begin_attributes_creation(model->attributes_buffer);

	binding->push_attribute(0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(Vertex::pos));
	binding->push_attribute(1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(Vertex::color));
	binding->push_attribute(2, VK_FORMAT_R32G32B32_SFLOAT, sizeof(Vertex::uvs));

	binding->end_attributes_creation();
    }
    
    internal void
    init_graphics_pipeline(Vulkan_API::Swapchain *swapchain
			   , Vulkan_API::GPU *gpu)
    {
	// create shaders
	File_Contents vsh_bytecode = read_file("../vulkan/shaders/vert.spv");
	File_Contents fsh_bytecode = read_file("../vulkan/shaders/frag.spv");

	Vulkan_API::Shader_Module vsh_module = { VK_SHADER_STAGE_VERTEX_BIT };
	Vulkan_API::Shader_Module_Create_Params vsh_params = { VK_SHADER_STAGE_VERTEX_BIT
							       , vsh_bytecode.size
							       , vsh_bytecode.content
							       , gpu};
	Vulkan_API::init_shader(&vsh_params, &vsh_module);
	
	Vulkan_API::Shader_Module fsh_module = { VK_SHADER_STAGE_FRAGMENT_BIT };
	Vulkan_API::Shader_Module_Create_Params fsh_params = { VK_SHADER_STAGE_VERTEX_BIT
							       , fsh_bytecode.size
							       , fsh_bytecode.content
							       , gpu};
	Vulkan_API::init_shader(&fsh_params, &fsh_module);

	VkPipelineShaderStageCreateInfo module_infos[] = {};
	Vulkan_API::init_shader_pipeline_info(&vsh_module, &module_infos[0]);
	Vulkan_API::init_shader_pipeline_info(&fsh_module, &module_infos[1]);

	// create vertex input info
	VkPipelineVertexInputStateCreateInfo vertex_input = {};
	Vulkan_Model_Handle vulkan_model_handle = get_vulkan_model_handle("vulkan_model.test_model"_hash);
	Vulkan_Model *model = get_vulkan_model(vulkan_model_handle);
	model->create_vertex_input_state_info(&vertex_input);

	// create input assembly
	VkPipelineInputAssemblyStateCreateInfo input_assembly	= {};
	input_assembly.sType					= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	input_assembly.topology					= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	input_assembly.primitiveRestartEnable			= VK_FALSE;

	// viewport
	VkViewport viewport	= {};
	viewport.x		= 0.0f;
	viewport.y		= 0.0f;
	viewport.width		= (float32)swapchain->extent.width;
	viewport.height		= (float32)swapchain->extent.height;
	viewport.minDepth	= 0.0f;
	viewport.maxDepth	= 1.0f;

	VkRect2D scissor	= {};
	scissor.offset		= {0, 0};
	scissor.extent		= swapchain->extent;

	VkPipelineViewportStateCreateInfo viewport_info	= {};
	viewport_info.sType				= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewport_info.viewportCount			= 1;
	viewport_info.pViewports			= &viewport;
	viewport_info.scissorCount			= 1;
	viewport_info.pScissors				= &scissor;

	// rasterizer
	VkPipelineRasterizationStateCreateInfo rasterizer	= {};
	rasterizer.sType					= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;    
	rasterizer.depthClampEnable				= VK_FALSE;
	rasterizer.rasterizerDiscardEnable			= VK_FALSE;
	rasterizer.polygonMode					= VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth					= 1.0f;
	rasterizer.cullMode					= VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace					= VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable				= VK_FALSE;
	rasterizer.depthBiasConstantFactor			= 0.0f;
	rasterizer.depthBiasClamp				= 0.0f;
	rasterizer.depthBiasSlopeFactor				= 0.0f;

	// multisampling
	VkPipelineMultisampleStateCreateInfo multisampling	= {};
	multisampling.sType					= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable			= VK_FALSE;
	multisampling.rasterizationSamples			= VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading				= 1.0f;
	multisampling.pSampleMask				= nullptr;
	multisampling.alphaToCoverageEnable			= VK_FALSE;
	multisampling.alphaToOneEnable				= VK_FALSE;

	// blending
	VkPipelineColorBlendAttachmentState color_blend_attachment = {};
	color_blend_attachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	color_blend_attachment.blendEnable = VK_FALSE; 
	color_blend_attachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // VK_BLEND_FACTOR_SRC_ALPHA
	color_blend_attachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA
	color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
	color_blend_attachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	color_blend_attachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo color_blending	= {};
	color_blending.sType					= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	color_blending.logicOpEnable				= VK_FALSE;
	color_blending.logicOp					= VK_LOGIC_OP_COPY;
	color_blending.attachmentCount				= 1;
	color_blending.pAttachments				= &color_blend_attachment;
	color_blending.blendConstants[0]			= 0.0f;
	color_blending.blendConstants[1]			= 0.0f;
	color_blending.blendConstants[2]			= 0.0f;
	color_blending.blendConstants[3]			= 0.0f;

	// dynamic states
	VkDynamicState dynamic_states[]
	{
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_LINE_WIDTH
	};
	VkPipelineDynamicStateCreateInfo dynamic_state = {};
	dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamic_state.dynamicStateCount = 2;
	dynamic_state.pDynamicStates = dynamic_states;

	// layout
	// get test pipeline descriptor set layout
	Vulkan_Descriptor_Set_Handle descriptor_handle = get_descriptor_set_layout_handle("descriptor_set_layout.test_descriptor_set_layout"_hash);
	VkDescriptorSetLayout *descriptor_set_layout = get_descriptor_set_layout(descriptor_handle);
	
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	// pipeline needs pipeline layout
	VK_CHECK(vkCreatePipelineLayout(gpu->logical_device, &pipeline_layout_info, nullptr, &pipeline_layout));
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
