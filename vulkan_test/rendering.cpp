#include <chrono>
#include "vulkan.hpp"
#include <glm/glm.hpp>
#include "rendering.hpp"
#include "file_system.hpp"
#include "vulkan_managers.hpp"
#include <glm/gtx/transform.hpp>

namespace Rendering
{

    /* initialize the rendering objects */
    internal void
    init_test_render_pass(Vulkan_API::Swapchain *swapchain
			  , Vulkan_API::GPU *gpu)
    {
	/* init main renderpass */
	Vulkan_API::Registered_Render_Pass test_render_pass = Vulkan_API::register_object("render_pass.test_render_pass"_hash
											  , sizeof(Vulkan_API::Render_Pass));
	
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
				     , test_render_pass.p);
    }

    internal void
    init_model_info(void)
    {
	Vulkan_API::Registered_Model model = Vulkan_API::register_object("vulkan_model.test_model"_hash
									 , sizeof(Vulkan_API::Model));

	model.p->attribute_count = 3;
	model.p->attributes_buffer = (VkVertexInputAttributeDescription *)allocate_free_list(sizeof(VkVertexInputAttributeDescription) * model.p->attribute_count
											     , Alignment(1)
											     , "test_model_attribute_list_allocation");
	model.p->binding_count = 1;
	model.p->bindings = (Vulkan_API::Model_Binding *)allocate_free_list(sizeof(Vulkan_API::Model_Binding) * model.p->binding_count
									    , Alignment(1)
									    , "test_model_binding_list_allocation");

	struct Vertex { glm::vec3 pos; glm::vec3 color; glm::vec2 uvs; };
	
	// only one binding
	Vulkan_API::Model_Binding *binding = model.p->bindings;
	binding->begin_attributes_creation(model.p->attributes_buffer);

	binding->push_attribute(0, VK_FORMAT_R32G32B32_SFLOAT, sizeof(Vertex::pos));
	binding->push_attribute(1, VK_FORMAT_R32G32B32_SFLOAT, sizeof(Vertex::color));
	binding->push_attribute(2, VK_FORMAT_R32G32_SFLOAT, sizeof(Vertex::uvs));

	binding->end_attributes_creation();
    }
    
    internal void
    init_graphics_pipeline(Vulkan_API::Swapchain *swapchain
			   , Vulkan_API::GPU *gpu)
    {
	// initialize graphics pipeline object in the manager
	Vulkan_API::Registered_Graphics_Pipeline graphics_pipeline = Vulkan_API::register_object("pipeline.main_pipeline"_hash
											       , sizeof(Vulkan_API::Graphics_Pipeline));
	graphics_pipeline.p->stages = Vulkan_API::Graphics_Pipeline::Shader_Stages_Bits::VERTEX_SHADER_BIT
	    | Vulkan_API::Graphics_Pipeline::Shader_Stages_Bits::FRAGMENT_SHADER_BIT;
	graphics_pipeline.p->base_dir_and_name = "../vulkan/shaders/";
	graphics_pipeline.p->descriptor_set_layout = Vulkan_API::get_object("descriptor_set_layout.test_descriptor_set_layout"_hash);
	
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
	Vulkan_API::Registered_Model model = Vulkan_API::get_object("vulkan_model.test_model"_hash);
	VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
	Vulkan_API::init_pipeline_vertex_input_info(model.p, &vertex_input_info);

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
	Vulkan_API::init_blend_state_attachment(VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
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
	Vulkan_API::Registered_Descriptor_Set_Layout &descriptor_set_layout = graphics_pipeline.p->descriptor_set_layout;
	Memory_Buffer_View<VkDescriptorSetLayout> layouts = {1, descriptor_set_layout.p};
	Memory_Buffer_View<VkPushConstantRange> ranges = {0, nullptr};
	Vulkan_API::init_pipeline_layout(&layouts, &ranges, gpu, &graphics_pipeline.p->layout);

	// init depth stencil info
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
	Vulkan_API::init_pipeline_depth_stencil_info(VK_TRUE, VK_TRUE, 0.0f, 1.0f, VK_FALSE, &depth_stencil_info);

	// init pipeline object
	Vulkan_API::Registered_Render_Pass render_pass = Vulkan_API::get_object("render_pass.test_render_pass"_hash);
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
					   , &graphics_pipeline.p->layout
					   , render_pass.p
					   , 0
					   , gpu
					   , &graphics_pipeline.p->pipeline);

	vkDestroyShaderModule(gpu->logical_device, vsh_module, nullptr);
	vkDestroyShaderModule(gpu->logical_device, fsh_module, nullptr);
    }
    
    internal void
    init_descriptor_set_layout(Vulkan_API::GPU *gpu)
    {
	// create descriptor set layout in manager
	Vulkan_API::Registered_Descriptor_Set_Layout layout = Vulkan_API::register_object("descriptor_set_layout.test_descriptor_set_layout"_hash
											  , sizeof(VkDescriptorSetLayout));
	
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

	VK_CHECK(vkCreateDescriptorSetLayout(gpu->logical_device, &layout_info, nullptr, layout.p));
    }

    internal void
    init_command_pool(Vulkan_API::GPU *gpu)
    {
	Vulkan_API::Registered_Command_Pool graphics_command_pool = Vulkan_API::register_object("command_pool.graphics_command_pool"_hash
												,sizeof(VkCommandPool));

	Vulkan_API::allocate_command_pool(gpu->queue_families.graphics_family
					  , gpu
					  , graphics_command_pool.p);
    }

    internal void
    init_depth_texture(Vulkan_API::Swapchain *swapchain
		       , Vulkan_API::GPU *gpu)
    {
	VkFormat depth_format = gpu->supported_depth_format;

	Vulkan_API::Registered_Image2D depth_image = Vulkan_API::register_object("image2D.depth_image"_hash
										 , sizeof(Vulkan_API::Image2D));

	Vulkan_API::init_image(swapchain->extent.width
			       , swapchain->extent.height
			       , depth_format
			       , VK_IMAGE_TILING_OPTIMAL
			       , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
			       , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			       , gpu
			       , depth_image.p);

	VkMemoryRequirements requirements = depth_image.p->get_memory_requirements(gpu);
	Vulkan_API::Memory::allocate_gpu_memory(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
						, requirements
						, gpu
						, &depth_image.p->device_memory);
	vkBindImageMemory(gpu->logical_device, depth_image.p->image, depth_image.p->device_memory, 0);

	Vulkan_API::init_image_view(&depth_image.p->image
				    , depth_format
				    , VK_IMAGE_ASPECT_DEPTH_BIT
				    , gpu
				    , &depth_image.p->image_view);

	Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	Vulkan_API::transition_image_layout(&depth_image.p->image
					    , depth_format
					    , VK_IMAGE_LAYOUT_UNDEFINED
					    , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
					    , command_pool.p
					    , gpu);
    }

    internal void
    init_swapchain_framebuffers(Vulkan_API::GPU *gpu
				, Vulkan_API::Swapchain *swapchain)
    {
	swapchain->framebuffers = Vulkan_API::register_object("framebuffer.swapchain_framebuffers"_hash
							      , sizeof(Vulkan_API::Framebuffer) * swapchain->images.size);
	
	
	char framebuffer_images_name[] = "framebuffer.swapchain_framebuffer0";
	enum { NUMBER_STRING_INDEX = 33 };
	enum { COLOR_ATTACHMENT = 0 };

	persist constexpr u32 COLOR_ATTACHMENTS_PER_FBO = 1;

	Vulkan_API::Registered_Render_Pass compatible_render_pass = Vulkan_API::get_object("render_pass.test_render_pass"_hash);
	Vulkan_API::Registered_Image2D depth_image = Vulkan_API::get_object("image2D.depth_image"_hash);
	
	for (u32 i = 0
		 ; i < swapchain->images.size
		 ; ++i)
	{
	    auto *framebuffer = &swapchain->framebuffers.p[i];
	    
	    framebuffer->color_attachments.count = COLOR_ATTACHMENTS_PER_FBO;
	    framebuffer->color_attachments.buffer = (Vulkan_API::Registered_Image2D *)allocate_free_list(sizeof(Vulkan_API::Registered_Image2D) * COLOR_ATTACHMENTS_PER_FBO);
	    framebuffer->color_attachments.buffer[COLOR_ATTACHMENT] = swapchain->images.extract(i);
	    framebuffer->depth_attachment = depth_image;
	    
	    Vulkan_API::init_framebuffer(compatible_render_pass.p
					 , swapchain->extent.width
					 , swapchain->extent.height
					 , gpu
					 , framebuffer);
	}
    }

    

    internal void
    init_object_texture(Vulkan_API::GPU *gpu)
    {
	Vulkan_API::Registered_Image2D image_ptr = Vulkan_API::register_object("image2D.texture"_hash
									       , sizeof(Vulkan_API::Image2D));
	    
	persist const char *jpg_file = "../vulkan/textures/texture.jpg";

	File file_description;
	file_description.file_path = jpg_file;
	file_description.format = File_Format::JPG;
	auto image_data = read_file_data(file_description, Read_Flags::RECORD);

	u32 w = image_data.extras[File_Data::Extra_Data::WIDTH];
	u32 h = image_data.extras[File_Data::Extra_Data::HEIGHT];

	VkDeviceSize image_size = w * h * 4;
    
	Vulkan_API::Buffer staging_buffer;
	staging_buffer.size = image_size;
	    
	Vulkan_API::init_buffer(image_size
				, VK_BUFFER_USAGE_TRANSFER_SRC_BIT
				, VK_SHARING_MODE_EXCLUSIVE
				, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				, gpu
				, &staging_buffer);
	    

	Vulkan_API::Mapped_GPU_Memory mapped_memory = staging_buffer.construct_map();

	mapped_memory.begin(gpu);
	memcpy(mapped_memory.data, image_data.data, static_cast<u32>(image_size));
	mapped_memory.end(gpu);

	destroy_file_data(&image_data);

	Vulkan_API::init_image(w, h
			       , VK_FORMAT_R8G8B8A8_UNORM
			       , VK_IMAGE_TILING_OPTIMAL
			       , VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
			       , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
			       , gpu
			       , image_ptr.p);

	Vulkan_API::Registered_Command_Pool command_pool_ptr = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	Vulkan_API::transition_image_layout(&image_ptr.p->image
					    , VK_FORMAT_R8G8B8A8_UNORM
					    , VK_IMAGE_LAYOUT_UNDEFINED
					    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					    , command_pool_ptr.p
					    , gpu);
	    
	Vulkan_API::copy_buffer_into_image(&staging_buffer
					   , image_ptr.p
					   , w
					   , h
					   , command_pool_ptr.p
					   , gpu);

	Vulkan_API::transition_image_layout(&image_ptr.p->image
					    , VK_FORMAT_R8G8B8A8_UNORM
					    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
					    , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
					    , command_pool_ptr.p
					    , gpu);

	vkDestroyBuffer(gpu->logical_device, staging_buffer.buffer, nullptr);
	vkFreeMemory(gpu->logical_device, staging_buffer.memory, nullptr);

	Vulkan_API::init_image_view(&image_ptr.p->image
				    , VK_FORMAT_R8G8B8A8_UNORM
				    , VK_IMAGE_ASPECT_COLOR_BIT
				    , gpu
				    , &image_ptr.p->image_view);

	Vulkan_API::init_image_sampler(VK_FILTER_LINEAR, VK_FILTER_LINEAR
				       , VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_REPEAT
				       , VK_TRUE, 16
				       , VK_BORDER_COLOR_INT_OPAQUE_BLACK
				       , VK_FALSE, VK_COMPARE_OP_ALWAYS
				       , VK_SAMPLER_MIPMAP_MODE_LINEAR, 0.0f, 0.0f, 0.0f
				       , gpu
				       , &image_ptr.p->image_sampler);
    }
    
    internal void
    init_vbo(Vulkan_API::GPU *gpu)
    {
	struct Vertex { glm::vec3 pos, color; glm::vec2 uvs; };
	
	persist Vertex vertices[]
	{
	    {{-1.0f, -0.5f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
	    {{-0.5f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

	    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
	};
	
	Vulkan_API::Registered_Model object_model = Vulkan_API::get_object("vulkan_model.test_model"_hash);
	auto *main_binding = &object_model.p->bindings[0];
	    
	Vulkan_API::Registered_Buffer object_model_vbo = Vulkan_API::register_object("buffer.test_model_vbo"_hash
										     , sizeof(Vulkan_API::Buffer));

	main_binding->buffer = object_model_vbo;
	
	Memory_Byte_Buffer byte_buffer{sizeof(vertices), vertices};
	Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	
	Vulkan_API::invoke_staging_buffer_for_device_local_buffer(byte_buffer
								  , command_pool.p
								  , object_model_vbo.p
								  , gpu);
    }

    persist u32 mesh_indices[]
    {
	0, 1, 2, 2, 3, 0,
	4, 5, 6, 6, 7, 4
    };    

    internal void
    init_ibo(Vulkan_API::GPU *gpu)
    {
	Vulkan_API::Registered_Model model = Vulkan_API::get_object("vulkan_model.test_model"_hash);
	Vulkan_API::Registered_Buffer ibo = Vulkan_API::register_object("buffer.test_model_ibo"_hash
									, sizeof(Vulkan_API::Buffer));

	model.p->index_data.index_buffer = ibo;

	Memory_Byte_Buffer byte_buffer{sizeof(mesh_indices), mesh_indices};
	    
	Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	    
	Vulkan_API::invoke_staging_buffer_for_device_local_buffer(byte_buffer
								  , command_pool.p
								  , ibo.p
								  , gpu);
    }

    internal void
    update_ubo(u32 current_image
	       , Vulkan_API::GPU *gpu
	       , Vulkan_API::Swapchain *swapchain
	       , Vulkan_API::Registered_Buffer &uniform_buffers)
    {
	struct Uniform_Buffer_Object
	{
	    alignas(16) glm::mat4 model_matrix;
	    alignas(16) glm::mat4 view_matrix;
	    alignas(16) glm::mat4 projection_matrix;
	};
	
	persist auto start_time = std::chrono::high_resolution_clock::now();

	auto current_time = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

	Uniform_Buffer_Object ubo = {};

	ubo.model_matrix = glm::rotate(time * glm::radians(90.0f)
				       , glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view_matrix = glm::lookAt(glm::vec3(2.0f)
				      , glm::vec3(0.0f)
				      , glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.projection_matrix = glm::perspective(glm::radians(60.0f)
						 , (float)swapchain->extent.width / (float)swapchain->extent.height
						 , 0.1f
						 , 10.0f);

	ubo.projection_matrix[1][1] *= -1;

	Vulkan_API::Buffer &current_ubo = uniform_buffers.p[current_image];

	auto map = current_ubo.construct_map();
	map.begin(gpu);
	map.fill(Memory_Byte_Buffer{sizeof(ubo), &ubo});
	map.end(gpu);
    }

    	
    internal void
    init_ubos(Vulkan_API::GPU *gpu
	      , Vulkan_API::Swapchain *swapchain
	      , Vulkan_API::Registered_Buffer &ubos)
    {
	struct Uniform_Buffer_Object
	{
	    alignas(16) glm::mat4 model_matrix;
	    alignas(16) glm::mat4 view_matrix;
	    alignas(16) glm::mat4 projection_matrix;
	};
	
	u32 uniform_buffer_count = swapchain->images.size;
	    
	char ubo_name[] = "buffer.ubo0";
	u32 char_count = sizeof(ubo_name) / sizeof(char);

	ubos = Vulkan_API::register_object("buffer.ubos"_hash
					   , sizeof(Vulkan_API::Buffer) * uniform_buffer_count);
	    
	VkDeviceSize buffer_size = sizeof(Uniform_Buffer_Object);

	for (u32 i = 0
		 ; i < uniform_buffer_count
		 ; ++i)
	{
	    Vulkan_API::init_buffer(buffer_size
				    , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
				    , VK_SHARING_MODE_EXCLUSIVE
				    , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
				    , gpu
				    , &ubos.p[i]);
	}
    }	

    namespace Old
    {
	
	internal struct Vulkan_State
	{
	    VkDescriptorPool descriptor_pool;
	    //VkDescriptorSet *descriptor_sets;
	    //u32 descriptor_set_count;

	    u32 command_buffer_count;
	    VkCommandBuffer *command_buffers;

	    u32 semaphore_count;
	    VkSemaphore *image_ready_semaphores;
	    VkSemaphore *render_finished_semaphores;

	    u32 fence_count;
	    VkFence *fences;
	} vk;
	
	internal Vulkan_API::State vulkan_state = {};
	internal Rendering::Rendering_Objects_Handle_Cache rendering_objects = {};



	// functions

	namespace Implementation
	{

	    internal u32
	    find_memory_type(u32 type_filter
			     , VkMemoryPropertyFlags properties)
	    {
		VkPhysicalDeviceMemoryProperties mem_properties;
		vkGetPhysicalDeviceMemoryProperties(vulkan_state.gpu.hardware
						    , &mem_properties);

		for (u32 i = 0
			 ; i < mem_properties.memoryTypeCount
			 ; ++i)
		{
		    if (type_filter & (1 << i)
			&& (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
		    {
			return(i);
		    }
		}

		OUTPUT_DEBUG_LOG("%s\n", "failed to find memory type");
		assert(false);

		return(0);
	    }

	    internal VkImageView
	    create_image_view(VkImage image
			      , VkFormat format
			      , VkImageAspectFlags aspect_flags)
	    {
		VkImageViewCreateInfo view_info		= {};
		view_info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		view_info.image				= image;
		view_info.viewType				= VK_IMAGE_VIEW_TYPE_2D;
		view_info.format				= format;
		view_info.subresourceRange.aspectMask	= aspect_flags;
		view_info.subresourceRange.baseMipLevel	= 0;
		view_info.subresourceRange.levelCount	= 1;
		view_info.subresourceRange.baseArrayLayer	= 0;
		view_info.subresourceRange.layerCount	= 1;

		VkImageView image_view;
		VK_CHECK(vkCreateImageView(vulkan_state.gpu.logical_device, &view_info, nullptr, &image_view));

		return(image_view);
	    }
	    

	    internal void
	    create_image(u32 width
			 , u32 height
			 , VkFormat format
			 , VkImageTiling tiling
			 , VkImageUsageFlags usage
			 , VkMemoryPropertyFlags properties
			 , VkImage *image
			 , VkDeviceMemory *image_memory)
	    {
		VkImageCreateInfo image_info = {};
		image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_info.imageType = VK_IMAGE_TYPE_2D;
		image_info.extent.width = width;
		image_info.extent.height = height;
		image_info.extent.depth = 1;
		image_info.mipLevels = 1;
		image_info.arrayLayers = 1;
		image_info.format = format;
		image_info.tiling = tiling;
		image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		image_info.usage = usage;
		image_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		VK_CHECK(vkCreateImage(vulkan_state.gpu.logical_device, &image_info, nullptr, image));

		VkMemoryRequirements mem_requirements = {};
		vkGetImageMemoryRequirements(vulkan_state.gpu.logical_device
					     , *image
					     , &mem_requirements);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits
							      , properties);

		VK_CHECK(vkAllocateMemory(vulkan_state.gpu.logical_device, &alloc_info, nullptr, image_memory));

		vkBindImageMemory(vulkan_state.gpu.logical_device, *image, *image_memory, 0);
	    }
	    
	    
	    internal void
	    create_buffer(VkBuffer *write_buffer
			  , VkDeviceMemory *memory
			  , VkDeviceSize size
			  , VkBufferUsageFlags usage
			  , VkMemoryPropertyFlags properties)
	    {
		VkBufferCreateInfo buffer_info	= {};
		buffer_info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffer_info.size			= size;
		buffer_info.usage			= usage;
		buffer_info.sharingMode		= VK_SHARING_MODE_EXCLUSIVE;
		buffer_info.flags			= 0;

		VK_CHECK(vkCreateBuffer(vulkan_state.gpu.logical_device, &buffer_info, nullptr, write_buffer));

		VkMemoryRequirements mem_requirements;
		vkGetBufferMemoryRequirements(vulkan_state.gpu.logical_device, *write_buffer, &mem_requirements);

		VkMemoryAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		alloc_info.allocationSize = mem_requirements.size;
		alloc_info.memoryTypeIndex = Implementation::find_memory_type(mem_requirements.memoryTypeBits, properties);

		VK_CHECK(vkAllocateMemory(vulkan_state.gpu.logical_device, &alloc_info, nullptr, memory));

		vkBindBufferMemory(vulkan_state.gpu.logical_device, *write_buffer, *memory, 0);
	    }


	    
	    internal VkCommandBuffer
	    begin_single_time_command(void)
	    {
		Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
    
		VkCommandBufferAllocateInfo alloc_info = {};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		alloc_info.commandPool = *command_pool.p;
		alloc_info.commandBufferCount = 1;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(vulkan_state.gpu.logical_device, &alloc_info, &command_buffer);

		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

		vkBeginCommandBuffer(command_buffer
				     , &begin_info);

		return(command_buffer);
	    }


	    
	    internal void
	    end_single_time_command(VkCommandBuffer command_buffer)
	    {
		Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
    
		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info = {};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(vulkan_state.gpu.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(vulkan_state.gpu.graphics_queue);

		vkFreeCommandBuffers(vulkan_state.gpu.logical_device, *command_pool.p, 1, &command_buffer);
	    }


	    
	    internal void
	    copy_buffer(VkBuffer *__restrict src_buffer
			, VkBuffer *__restrict dst_buffer
			, VkDeviceSize size)
	    {
		VkCommandBuffer command_buffer = begin_single_time_command();

		VkBufferCopy copy_region = {};
		copy_region.size = size;
		vkCmdCopyBuffer(command_buffer, *src_buffer, *dst_buffer, 1, &copy_region);

		end_single_time_command(command_buffer);
	    }

	    internal bool
	    has_stencil_component(VkFormat format)
	    {
		return(format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT);
	    }

	    internal void
	    transition_image_layout(VkImage image
				    , VkFormat format
				    , VkImageLayout old_layout
				    , VkImageLayout new_layout)
	    {
		VkCommandBuffer command_buffer = begin_single_time_command();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = old_layout;
		barrier.newLayout = new_layout;

		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   
		barrier.image				= image;
		barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel	= 0;
		barrier.subresourceRange.levelCount		= 1;
		barrier.subresourceRange.baseArrayLayer	= 0;
		barrier.subresourceRange.layerCount		= 1;

		VkPipelineStageFlags source_stage;
		VkPipelineStageFlags destination_stage;

		if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
		    barrier.srcAccessMask = 0;
		    barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
		    barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		    barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		    source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		    destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		}
		else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
		    barrier.srcAccessMask = 0;
		    barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
			| VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		    source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		    destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		}
		else
		{
		    OUTPUT_DEBUG_LOG("%s\n", "unsupported layout transition");
		    assert(false);
		}

		if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
		{
		    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		    if (has_stencil_component(format))
		    {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		    }
		}
		else
		{
		    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		}

		vkCmdPipelineBarrier(command_buffer
				     , source_stage
				     , destination_stage
				     , 0
				     , 0
				     , nullptr
				     , 0
				     , nullptr
				     , 1
				     , &barrier);

		end_single_time_command(command_buffer);
	    }

	    
	    internal void
	    copy_buffer_to_image(VkBuffer buffer
				 , VkImage image
				 , u32 w, u32 h)
	    {
		VkCommandBuffer command_buffer = begin_single_time_command();

		VkBufferImageCopy region	= {};
		region.bufferOffset		= 0;
		region.bufferRowLength	= 0;
		region.bufferImageHeight	= 0;

		region.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
		region.imageSubresource.mipLevel		= 0;
		region.imageSubresource.baseArrayLayer	= 0;
		region.imageSubresource.layerCount		= 1;

		region.imageOffset = {0, 0, 0};
		region.imageExtent = { w, h, 1 };

		vkCmdCopyBufferToImage(command_buffer
				       , buffer
				       , image
				       , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
				       , 1
				       , &region);

		end_single_time_command(command_buffer);
	    }
	    
	}


	
	struct Uniform_Buffer_Object
	{
	    alignas(16) glm::mat4 model_matrix;
	    alignas(16) glm::mat4 view_matrix;
	    alignas(16) glm::mat4 projection_matrix;
	};





	
	internal void
	init_descriptor_pool(void)
	{
	    VkDescriptorPoolSize pool_sizes[2] = {};

	    pool_sizes[0].type			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    pool_sizes[0].descriptorCount	= vulkan_state.swapchain.images.size;
    
	    pool_sizes[1].type			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    pool_sizes[1].descriptorCount	= vulkan_state.swapchain.images.size;

	    VkDescriptorPoolCreateInfo pool_info = {};
	    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	    pool_info.poolSizeCount = 2;
	    pool_info.pPoolSizes = pool_sizes;

	    pool_info.maxSets = vulkan_state.swapchain.images.size;

	    VK_CHECK(vkCreateDescriptorPool(vulkan_state.gpu.logical_device, &pool_info, nullptr, &vk.descriptor_pool));
	}

	internal void
	init_descriptor_sets(void)
	{
	    u32 set_count = vulkan_state.swapchain.images.size;

	    VkDescriptorSetLayout *descriptor_set_layouts = (VkDescriptorSetLayout *)allocate_stack(set_count * sizeof(VkDescriptorSetLayout)
												    , Alignment(1)
												    , "descriptor_set_layout_list_allocation");
	    Vulkan_API::Registered_Descriptor_Set_Layout main_descriptor_layout = Vulkan_API::get_object("descriptor_set_layout.test_descriptor_set_layout"_hash);
    
	    for (u32 i = 0
		     ; i < set_count
		     ; ++i) descriptor_set_layouts[i] = *main_descriptor_layout.p;


	    Vulkan_API::Registered_Descriptor_Set descriptor_sets = Vulkan_API::register_object("descriptor_set.test_descriptor_sets"_hash
												, sizeof(Vulkan_API::Descriptor_Set) * set_count);
	    auto descriptor_sets_separate = descriptor_sets.separate();
	    
	    Vulkan_API::allocate_descriptor_sets(descriptor_sets_separate
						 , Memory_Buffer_View<VkDescriptorSetLayout>{set_count, descriptor_set_layouts}
						 , &vulkan_state.gpu
						 , &vk.descriptor_pool);

	    Vulkan_API::Registered_Image2D image_ptr = Vulkan_API::get_object("image2D.texture"_hash);
	    Vulkan_API::Registered_Buffer ubos = Vulkan_API::get_object("buffer.ubos"_hash);

	    for (u32 i = 0
		     ; i < set_count
		     ; ++i)
	    {
		Vulkan_API::Descriptor_Set *set = &descriptor_sets.p[i];
		
		VkDescriptorBufferInfo buffer_info = {};
		Vulkan_API::init_descriptor_set_buffer_info(&ubos.p[i], 0, &buffer_info);

		VkDescriptorImageInfo image_info	= {};
		Vulkan_API::init_descriptor_set_image_info(image_ptr.p, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &image_info);

		VkWriteDescriptorSet descriptor_writes[2] = {};

		Vulkan_API::init_buffer_descriptor_set_write(set, 0, 0, 1, &buffer_info, &descriptor_writes[0]);
		Vulkan_API::init_image_descriptor_set_write(set, 1, 0, 1, &image_info, &descriptor_writes[1]);

		Vulkan_API::update_descriptor_sets(Memory_Buffer_View<VkWriteDescriptorSet>{2, descriptor_writes}, &vulkan_state.gpu);
	    }

	    rendering_objects.descriptor_sets = descriptor_sets;
	}


	
	internal void
	init_command_buffers(Vulkan_API::Swapchain *swapchain
			     , const Vulkan_API::Registered_Descriptor_Set &descriptor_sets)
	{
	    Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
    
	    vk.command_buffer_count = vulkan_state.swapchain.images.size;

	    vk.command_buffers = (VkCommandBuffer *)allocate_stack(sizeof(VkCommandBuffer) * vk.command_buffer_count
								   , Alignment(1)
								   , "command_buffer_list_allocation");

	    VkCommandBufferAllocateInfo alloc_info = {};
	    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	    alloc_info.commandPool = *command_pool.p;
	    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	    alloc_info.commandBufferCount = vk.command_buffer_count;

	    VK_CHECK(vkAllocateCommandBuffers(vulkan_state.gpu.logical_device
					      , &alloc_info
					      , vk.command_buffers));

	    Vulkan_API::Registered_Render_Pass render_pass = Vulkan_API::get_object("render_pass.test_render_pass"_hash);

	    Vulkan_API::Registered_Graphics_Pipeline pipeline_ptr = Vulkan_API::get_object("pipeline.main_pipeline"_hash);

	    Vulkan_API::Registered_Model model = Vulkan_API::get_object("vulkan_model.test_model"_hash);
	    Vulkan_API::Registered_Buffer &vbo = model.p->bindings[0].buffer;
	    Vulkan_API::Registered_Buffer &ibo = model.p->index_data.index_buffer;
	    
	    for (u32 i = 0
		     ; i < vk.command_buffer_count
		     ; ++i)
	    {
		VkCommandBufferBeginInfo begin_info = {};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.pNext = nullptr;
		// means that a command buffer can be resubmitted to a queue
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		begin_info.pInheritanceInfo = nullptr;

		VK_CHECK(vkBeginCommandBuffer(vk.command_buffers[i]
					      , &begin_info));

		VkRenderPassBeginInfo render_pass_info = {};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		render_pass_info.pNext = nullptr;
		render_pass_info.renderPass = render_pass.p->render_pass;

		Vulkan_API::Registered_Framebuffer fbo = swapchain->framebuffers.extract(i);
	
		render_pass_info.framebuffer = fbo.p->framebuffer;
		render_pass_info.renderArea.offset = {0, 0};
		render_pass_info.renderArea.extent = vulkan_state.swapchain.extent;

		VkClearValue clear_colors[2];
		clear_colors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
		clear_colors[1].depthStencil = {1.0f, 0};
		render_pass_info.clearValueCount = 2;
		render_pass_info.pClearValues = clear_colors;

		vkCmdBeginRenderPass(vk.command_buffers[i]
				     , &render_pass_info
				     , VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(vk.command_buffers[i]
				  , VK_PIPELINE_BIND_POINT_GRAPHICS
				  , pipeline_ptr.p->pipeline);

		VkBuffer vertex_buffers[] = {vbo.p->buffer};
		VkDeviceSize offset[] = {0};
		vkCmdBindVertexBuffers(vk.command_buffers[i]
				       , 0
				       , 1
				       , vertex_buffers
				       , offset);

		vkCmdBindIndexBuffer(vk.command_buffers[i]
				     , ibo.p->buffer
				     , 0
				     , VK_INDEX_TYPE_UINT32);

		Vulkan_API::Descriptor_Set *descriptor_set = &descriptor_sets.p[i];
		vkCmdBindDescriptorSets(vk.command_buffers[i]
					, VK_PIPELINE_BIND_POINT_GRAPHICS
					, pipeline_ptr.p->layout
					, 0
					, 1
					, &descriptor_set->set
					, 0
					, nullptr);

		vkCmdDrawIndexed(vk.command_buffers[i]
				 , sizeof(mesh_indices) / sizeof(u32)
				 , 1
				 , 0
				 , 0
				 , 0);

		vkCmdEndRenderPass(vk.command_buffers[i]);
		VK_CHECK(vkEndCommandBuffer(vk.command_buffers[i]));
	    }
	}


	
	internal constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;


	
	internal void
	init_sync(void)
	{
	    vk.semaphore_count = MAX_FRAMES_IN_FLIGHT;
	    vk.image_ready_semaphores = (VkSemaphore *)allocate_stack(sizeof(VkSemaphore) * vk.semaphore_count
								      , Alignment(1)
								      , "sempahore_image_ready_list_allocation");

	    vk.render_finished_semaphores = (VkSemaphore *)allocate_stack(sizeof(VkSemaphore) * vk.semaphore_count
									  , Alignment(1)
									  , "sempahore_image_ready_list_allocation");

	    vk.fences = (VkFence *)allocate_stack(sizeof(VkFence) * vk.semaphore_count
						  , Alignment(1)
						  , "fence_list_allocation");

	    VkSemaphoreCreateInfo semaphore_info = {};
	    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	    VkFenceCreateInfo fence_info = {};
	    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
	    for (u32 i = 0
		     ; i < MAX_FRAMES_IN_FLIGHT
		     ; ++i)
	    {
		VK_CHECK(vkCreateFence(vulkan_state.gpu.logical_device
				       , &fence_info
				       , nullptr
				       , &vk.fences[i]));

		VK_CHECK(vkCreateSemaphore(vulkan_state.gpu.logical_device
					   , &semaphore_info
					   , nullptr
					   , &vk.render_finished_semaphores[i]));

		VK_CHECK(vkCreateSemaphore(vulkan_state.gpu.logical_device
					   , &semaphore_info
					   , nullptr
					   , &vk.image_ready_semaphores[i]));
	    }
	}


	
	
	internal u32 current_frame = 0;
	
	
	void
	draw_frame(void)
	{
	    vkWaitForFences(vulkan_state.gpu.logical_device
			    , 1
			    , &vk.fences[current_frame]
			    , VK_TRUE
			    , UINT_MAX);

	    u32 image_index;

	    VkResult result = vkAcquireNextImageKHR(vulkan_state.gpu.logical_device
						    , vulkan_state.swapchain.swapchain
						    , UINT_MAX
						    , vk.image_ready_semaphores[current_frame]
						    , vk.fences[current_frame]
						    , &image_index);
    
	    if (result == VK_ERROR_OUT_OF_DATE_KHR)
	    {
		// recreate swapchain
		return;
	    }
	    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	    {
		OUTPUT_DEBUG_LOG("%s\n", "failed to acquire swapchain image");
	    }

	    update_ubo(image_index
		       , &vulkan_state.gpu
		       , &vulkan_state.swapchain
		       , rendering_objects.uniform_buffers);

	    VkSubmitInfo submit_info = {};
	    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	    VkSemaphore wait_semaphores[] = {vk.image_ready_semaphores[current_frame]};
	    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	    submit_info.waitSemaphoreCount = 1;
	    submit_info.pWaitSemaphores = wait_semaphores;
	    submit_info.pWaitDstStageMask = wait_stages;

	    submit_info.commandBufferCount = 1;
	    submit_info.pCommandBuffers = &vk.command_buffers[image_index];

	    VkSemaphore signal_semaphores[] = {vk.render_finished_semaphores[current_frame]};
	    submit_info.signalSemaphoreCount = 1;
	    submit_info.pSignalSemaphores = signal_semaphores;

	    vkResetFences(vulkan_state.gpu.logical_device, 1, &vk.fences[current_frame]);

	    vkQueueSubmit(vulkan_state.gpu.graphics_queue
			  , 1
			  , &submit_info
			  , vk.fences[current_frame]);

	    VkPresentInfoKHR present_info = {};
	    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	    present_info.waitSemaphoreCount = 1;
	    present_info.pWaitSemaphores = signal_semaphores;

	    VkSwapchainKHR swapchains[] = {vulkan_state.swapchain.swapchain};
	    present_info.swapchainCount = 1;
	    present_info.pSwapchains = swapchains;
	    present_info.pImageIndices = &image_index;

	    result = vkQueuePresentKHR(vulkan_state.gpu.present_queue
				       , &present_info);

	    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	    {
		// recreate swapchain
	    }
	    else if (result != VK_SUCCESS)
	    {
		OUTPUT_DEBUG_LOG("%s\n", "failed to present swapchain image");
	    }

	    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}


	
	void
	init_vk(GLFWwindow *window)
	{
	    Vulkan_API::init_state(&vulkan_state
				   , window);

	    Rendering::init_rendering_state(&vulkan_state
					    , &rendering_objects);

	    init_descriptor_pool();
	    init_descriptor_sets();


	    init_command_buffers(&vulkan_state.swapchain
				 , rendering_objects.descriptor_sets);
	    init_sync();
	}

	void
	destroy_vk(void)
	{
	    vkDestroyDevice(vulkan_state.gpu.logical_device, nullptr);
	    vkDestroyInstance(vulkan_state.instance, nullptr);
	}
	
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

	init_command_pool(&vulkan_state->gpu);

	init_depth_texture(&vulkan_state->swapchain, &vulkan_state->gpu);

	init_swapchain_framebuffers(&vulkan_state->gpu, &vulkan_state->swapchain);

	init_object_texture(&vulkan_state->gpu);

	init_vbo(&vulkan_state->gpu);

	init_ibo(&vulkan_state->gpu);

	init_ubos(&vulkan_state->gpu
		  , &vulkan_state->swapchain
		  , cache->uniform_buffers);
	
	cache->test_render_pass = Vulkan_API::get_object("render_pass.test_render_pass"_hash);
	cache->descriptor_set_layout = Vulkan_API::get_object("descriptor_set_layout.test_descriptor_set_layout"_hash);
	cache->graphics_pipeline = Vulkan_API::get_object("pipeline.main_pipeline"_hash);
	cache->graphics_command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	cache->depth_image = Vulkan_API::get_object("image2D.depth_image"_hash);
	cache->texture = Vulkan_API::get_object("image2D.texture"_hash);
    }

}
