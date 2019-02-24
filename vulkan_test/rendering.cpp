#include <glm/glm.hpp>
#include "rendering.hpp"
#include "vulkan_handles.hpp"
#include "vulkan_managers.hpp"

namespace Rendering
{

    /* initialize the rendering objects */
    internal void
    init_test_render_pass(Vulkan_API::Swapchain *swapchain
			  , Vulkan_API::GPU *gpu)
    {
	/* init main renderpass */
	Vulkan_API::Render_Pass_Handle test_render_pass = Vulkan_API::add_render_pass("render_pass.test_render_pass"_hash);
	Vulkan_API::Render_Pass *test_render_pass_object = Vulkan_API::get_render_pass(test_render_pass);
	
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
	Vulkan_API::Model_Handle handle = Vulkan_API::add_vulkan_model("vulkan_model.test_model"_hash);
	Vulkan_API::Model *model = Vulkan_API::get_vulkan_model(handle);

	model->attribute_count = 3;
	model->attributes_buffer = (VkVertexInputAttributeDescription *)allocate_free_list(sizeof(VkVertexInputAttributeDescription) * model->attribute_count
											   , Alignment(1)
											   , "test_model_attribute_list_allocation");
	model->binding_count = 1;
	model->bindings = (Vulkan_API::Model_Binding *)allocate_free_list(sizeof(Vulkan_API::Model_Binding) * model->binding_count
									  , Alignment(1)
									  , "test_model_binding_list_allocation");

	struct Vertex { glm::vec3 pos; glm::vec3 color; glm::vec2 uvs; };
	
	// only one binding
	Vulkan_API::Model_Binding *binding = model->bindings;
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

	VkPipelineShaderStageCreateInfo module_infos[2] = {};
	Vulkan_API::init_shader_pipeline_info(&vsh_module, &module_infos[0]);
	Vulkan_API::init_shader_pipeline_info(&fsh_module, &module_infos[1]);

	// create vertex input info
	VkPipelineVertexInputStateCreateInfo vertex_input = {};
	Vulkan_API::Model_Handle vulkan_model_handle = Vulkan_API::get_vulkan_model_handle("vulkan_model.test_model"_hash);
	Vulkan_API::Model *model = Vulkan_API::get_vulkan_model(vulkan_model_handle);
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
	Vulkan_API::Descriptor_Set_Handle descriptor_handle = Vulkan_API::get_descriptor_set_layout_handle("descriptor_set_layout.test_descriptor_set_layout"_hash);
	VkDescriptorSetLayout *descriptor_set_layout = Vulkan_API::get_descriptor_set_layout(descriptor_handle);
	
	VkPipelineLayoutCreateInfo pipeline_layout_info = {};
	pipeline_layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 0;
	pipeline_layout_info.pPushConstantRanges = nullptr;

	// pipeline needs pipeline layout
	VkPipelineLayout pipeline_layout;
	VK_CHECK(vkCreatePipelineLayout(gpu->logical_device, &pipeline_layout_info, nullptr, &pipeline_layout));
    }
    
    internal void
    init_descriptor_set_layout(Vulkan_API::GPU *gpu)
    {
	// create descriptor set layout in manager
	Vulkan_API::Descriptor_Set_Layout_Handle handle = Vulkan_API::add_descriptor_set_layout("descriptor_set_layout.test_descriptor_set_layout"_hash);
	
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

	VkDescriptorSetLayout *layout = Vulkan_API::get_descriptor_set_layout(handle);
	VK_CHECK(vkCreateDescriptorSetLayout(gpu->logical_device, &layout_info, nullptr, layout));
    }
    
    extern_impl void
    init_rendering_state(Vulkan_API::State *vulkan_state
			 , Rendering_Objects_Handle_Cache *cache)
    {
	init_test_render_pass(&vulkan_state->swapchain
			      , &vulkan_state->gpu);
	
	init_descriptor_set_layout(&vulkan_state->gpu);
	
	cache->test_render_pass = Vulkan_API::get_render_pass_handle("render_pass.test_render_pass"_hash);
	cache->descriptor_set_layout = Vulkan_API::get_descriptor_set_layout_handle("descriptor_set_layout.test_descriptor_set_layout"_hash);
    }

}
