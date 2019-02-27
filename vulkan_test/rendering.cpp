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

	Memory_Buffer_View<VkAttachmentDescription> descriptions_array	= {2, descriptions};
	Memory_Buffer_View<VkSubpassDescription> subpasses_array		= {1, &subpass};
	Memory_Buffer_View<VkSubpassDependency> dependencies_array		= {1, &dependency};
	
	Vulkan_API::init_render_pass(&descriptions_array
				     , &subpasses_array
				     , &dependencies_array
				     , gpu
				     , test_render_pass_object);
    }

    internal void
    init_model_info(void)
    {
	Vulkan_API::Model_Handle handle = Vulkan_API::add_model("vulkan_model.test_model"_hash);
	Vulkan_API::Model *model = Vulkan_API::get_model(handle);

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
	// initialize graphics pipeline object in the manager
	Vulkan_API::Graphics_Pipeline_Handle pipeline_handle = Vulkan_API::add_graphics_pipeline("pipeline.main_pipeline"_hash);
	Vulkan_API::Graphics_Pipeline *graphics_pipeline = Vulkan_API::get_graphics_pipeline(pipeline_handle);
	graphics_pipeline->stages = Vulkan_API::Graphics_Pipeline::Shader_Stages_Bits::VERTEX_SHADER_BIT
	                            | Vulkan_API::Graphics_Pipeline::Shader_Stages_Bits::FRAGMENT_SHADER_BIT;
	graphics_pipeline->base_dir_and_name = "../vulkan/shaders/";
	graphics_pipeline->descriptor_set_layout = Vulkan_API::get_descriptor_set_layout_handle("descriptor_set_layout.test_descriptor_set_layout"_hash);
	
	// create shaders
	File_Contents vsh_bytecode = read_file("../vulkan/shaders/vert.spv");
	File_Contents fsh_bytecode = read_file("../vulkan/shaders/frag.spv");
	
	VkShaderModule vsh_module;
	Vulkan_API::init_shader(VK_SHADER_STAGE_VERTEX_BIT, vsh_bytecode.size, vsh_bytecode.content, gpu, &vsh_module);
	
	VkShaderModule fsh_module;
	Vulkan_API::init_shader(VK_SHADER_STAGE_FRAGMENT_BIT, fsh_bytecode.size, fsh_bytecode.content, gpu, &fsh_module);

	VkPipelineShaderStageCreateInfo module_infos[2] = {};
	Vulkan_API::init_shader_pipeline_info(&vsh_module, VK_SHADER_STAGE_VERTEX_BIT, &module_infos[0]);
	Vulkan_API::init_shader_pipeline_info(&fsh_module, VK_SHADER_STAGE_FRAGMENT_BIT, &module_infos[1]);

	// init vertex input stuff
	Vulkan_API::Model_Handle model_handle = Vulkan_API::get_model_handle("vulkan_model.test_model"_hash);
	Vulkan_API::Model *model = Vulkan_API::get_model(model_handle);
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	Vulkan_API::init_pipeline_vertex_input_info(model, &vertex_input_info);

	// init assembly info
	VkPipelineInputAssemblyStateCreateInfo assembly_info = {};
	Vulkan_API::init_pipeline_input_assembly_info(0, VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, VK_FALSE, &assembly_info);

	// init viewport info
	VkViewport viewport = {};
	Vulkan_API::init_viewport(swapchain->extent.width, swapchain->extent.height, 0.0f, 1.0f, &viewport);
	VkRect2D scissor = {};
	Vulkan_API::init_rect_2D(VkOffset2D{}, swapchain->extent, &scissor);

	VkPipelineViewportStateCreateInfo viewport_info = {};
	Memory_Buffer_View<VkViewport> viewports = {1, &viewport};
	Memory_Buffer_View<VkRect2D>   scissors = {1, &scissor};
	Vulkan_API::init_pipeline_viewport_info(&viewports, &scissors, &viewport_info);

	// init rasterization info
	VkPipelineRasterizationStateCreateInfo rasterization_info = {};
	Vulkan_API::init_pipeline_rasterization_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, 1.0f, 0, &rasterization_info);

	// init multisample info
	VkPipelineMultisampleStateCreateInfo multisample_info = {};
	Vulkan_API::init_pipeline_multisampling_info(VK_SAMPLE_COUNT_1_BIT, 0, &multisample_info);

	// init blending info
	VkPipelineColorBlendAttachmentState blend_attachment = {};
	Vulkan_API::init_blend_state_attachment(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
						, VK_FALSE
						, VK_BLEND_FACTOR_ONE
						, VK_BLEND_FACTOR_ZERO
						, VK_BLEND_OP_ADD
						, VK_BLEND_FACTOR_ONE
						, VK_BLEND_FACTOR_ZERO
						, VK_BLEND_OP_ADD
						, &blend_attachment);
	VkPipelineColorBlendStateCreateInfo blending_info = {};
	Memory_Buffer_View<VkPipelineColorBlendAttachmentState> blend_attachments = {1, &blend_attachment};
	Vulkan_API::init_pipeline_blending_info(VK_FALSE, VK_LOGIC_OP_COPY, &blend_attachments, &blending_info);

	// init dynamic states info
	VkDynamicState dynamic_states[]
	{
	    VK_DYNAMIC_STATE_VIEWPORT,
	    VK_DYNAMIC_STATE_LINE_WIDTH
	};
	Memory_Buffer_View<VkDynamicState> dynamic_states_ptr = {2, dynamic_states};
	VkPipelineDynamicStateCreateInfo dynamic_info = {};
	Vulkan_API::init_pipeline_dynamic_states_info(&dynamic_states_ptr, &dynamic_info);

	// init pipeline layout
	VkDescriptorSetLayout *descriptor_set_layout = Vulkan_API::get_descriptor_set_layout(graphics_pipeline->descriptor_set_layout);
	Memory_Buffer_View<VkDescriptorSetLayout> layouts = {1, descriptor_set_layout};
	Memory_Buffer_View<VkPushConstantRange> ranges = {0, nullptr};
	Vulkan_API::init_pipeline_layout(&layouts, &ranges, gpu, &graphics_pipeline->layout);

	// init depth stencil info
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
	Vulkan_API::init_pipeline_depth_stencil_info(VK_TRUE, VK_TRUE, 0.0f, 1.0f, VK_FALSE, &depth_stencil_info);

	// init pipeline object
	Vulkan_API::Render_Pass_Handle render_pass_handle = Vulkan_API::get_render_pass_handle("render_pass.test_render_pass"_hash);
	Vulkan_API::Render_Pass *render_pass = Vulkan_API::get_render_pass(render_pass_handle);
	Memory_Buffer_View<VkPipelineShaderStageCreateInfo> modules = {2, module_infos};
	Vulkan_API::init_graphics_pipeline(&modules
					   , &vertex_input_info
					   , &assembly_info
					   , &viewport_info
					   , &rasterization_info
					   , &multisample_info
					   , &blending_info
					   , nullptr
					   , &depth_stencil_info
					   , &graphics_pipeline->layout
					   , render_pass
					   , 0
					   , gpu
					   , &graphics_pipeline->pipeline);

	vkDestroyShaderModule(gpu->logical_device, vsh_module, nullptr);
	vkDestroyShaderModule(gpu->logical_device, fsh_module, nullptr);
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
    
    void
    init_rendering_state(Vulkan_API::State *vulkan_state
			 , Rendering_Objects_Handle_Cache *cache)
    {
	init_test_render_pass(&vulkan_state->swapchain
			      , &vulkan_state->gpu);
	
	init_descriptor_set_layout(&vulkan_state->gpu);

	init_model_info();

	init_graphics_pipeline(&vulkan_state->swapchain, &vulkan_state->gpu);
	
	cache->test_render_pass = Vulkan_API::get_render_pass_handle("render_pass.test_render_pass"_hash);
	cache->descriptor_set_layout = Vulkan_API::get_descriptor_set_layout_handle("descriptor_set_layout.test_descriptor_set_layout"_hash);
	cache->graphics_pipeline = Vulkan_API::get_graphics_pipeline_handle("pipeline.main_pipeline"_hash);
    }

}
