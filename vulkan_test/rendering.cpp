#include <chrono>
#include "vulkan.hpp"
#include <glm/glm.hpp>
#include "rendering.hpp"
#include "file_system.hpp"
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
	
	VkAttachmentDescription descriptions[2]	= {};
	descriptions[COLOR_DESCRIPTION] = Vulkan_API::init_attachment_description(swapchain->format
										  , VK_SAMPLE_COUNT_1_BIT
										  , VK_ATTACHMENT_LOAD_OP_CLEAR
										  , VK_ATTACHMENT_STORE_OP_STORE
										  , VK_ATTACHMENT_LOAD_OP_DONT_CARE
										  , VK_ATTACHMENT_STORE_OP_DONT_CARE
										  , VK_IMAGE_LAYOUT_UNDEFINED
										  , VK_IMAGE_LAYOUT_PRESENT_SRC_KHR); 

	descriptions[DEPTH_DESCRIPTION] = Vulkan_API::init_attachment_description(gpu->supported_depth_format
										  , VK_SAMPLE_COUNT_1_BIT
										  , VK_ATTACHMENT_LOAD_OP_CLEAR
										  , VK_ATTACHMENT_STORE_OP_DONT_CARE
										  , VK_ATTACHMENT_LOAD_OP_DONT_CARE
										  , VK_ATTACHMENT_STORE_OP_DONT_CARE
										  , VK_IMAGE_LAYOUT_UNDEFINED
										  , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	
	VkAttachmentReference references[2] = {};
	
	references[COLOR_DESCRIPTION] = Vulkan_API::init_attachment_reference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	references[DEPTH_DESCRIPTION] = Vulkan_API::init_attachment_reference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	
	VkSubpassDescription subpass = Vulkan_API::init_subpass_description(single_buffer(&references[COLOR_DESCRIPTION])
									    , &references[DEPTH_DESCRIPTION]
									    , null_buffer<VkAttachmentReference>());

	VkSubpassDependency dependencies[2] = {};
	
	dependencies[0] = Vulkan_API::init_subpass_dependency(VK_SUBPASS_EXTERNAL
							      , 0
							      , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
							      , 0
							      , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
							      , VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
							      | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
							      , VK_DEPENDENCY_BY_REGION_BIT);

	dependencies[1] = Vulkan_API::init_subpass_dependency(0
							      , VK_SUBPASS_EXTERNAL
							      , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
							      , VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
							      , VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
							      , VK_ACCESS_MEMORY_READ_BIT
							      , VK_DEPENDENCY_BY_REGION_BIT);

	Memory_Buffer_View<VkAttachmentDescription> descriptions_array	= {2, descriptions};
	Memory_Buffer_View<VkSubpassDescription> subpasses_array		= {1, &subpass};
	Memory_Buffer_View<VkSubpassDependency> dependencies_array		= {2, dependencies};
	
	Vulkan_API::init_render_pass(descriptions_array
				     , subpasses_array
				     , dependencies_array
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

	//	model.p->create_vbo_list();
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
	File_Contents vsh_bytecode = read_file("shaders/SPV/triangle.vert.spv");
	File_Contents fsh_bytecode = read_file("shaders/SPV/triangle.frag.spv");
	
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

	VkPushConstantRange push_k_rng  = {};
	Vulkan_API::init_push_constant_range(VK_SHADER_STAGE_VERTEX_BIT
					     , sizeof(glm::mat4)
					     , 0
					     , &push_k_rng);
	Memory_Buffer_View<VkPushConstantRange> ranges = {1, &push_k_rng};
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

	VkDescriptorSetLayoutBinding bindings[] =
	{
	    Vulkan_API::init_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, 1, VK_SHADER_STAGE_VERTEX_BIT)
	    , Vulkan_API::init_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	
	Vulkan_API::init_descriptor_set_layout(Memory_Buffer_View<VkDescriptorSetLayoutBinding>{2, bindings}
					       , gpu
					       , layout.p);
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

	glm::vec3 green = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::vec3 blue = glm::vec3(1.0f, 0.0f, 0.0f);
	
	f32 radius = 2.0f;
	
	persist Vertex vertices[]
	{
	    {{-radius, -radius, radius	}, green},
	    {{radius, -radius, radius	}, green},
	    {{radius, radius, radius	}, green},
	    {{-radius, radius, radius	}, green},
												 
	    {{-radius, -radius, -radius	}, blue},
	    {{radius, -radius, -radius	}, blue},
	    {{radius, radius, -radius	}, blue},
	    {{-radius, radius, -radius	}, blue}
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

	object_model.p->create_vbo_list();
    }

    persist u32 mesh_indices[] = 
    {
	0, 1, 2,
	2, 3, 0,

	1, 5, 6,
	6, 2, 1,

	7, 6, 5,
	5, 4, 7,
	    
	4, 0, 3,
	3, 7, 4,
	    
	4, 5, 1,
	1, 0, 4,
	    
	3, 2, 6,
	6, 7, 3	
    };    

    internal void
    init_ibo(Vulkan_API::GPU *gpu)
    {
	Vulkan_API::Registered_Model model = Vulkan_API::get_object("vulkan_model.test_model"_hash);
	Vulkan_API::Registered_Buffer ibo = Vulkan_API::register_object("buffer.test_model_ibo"_hash
									, sizeof(Vulkan_API::Buffer));

	model.p->index_data.index_buffer = ibo;
	model.p->index_data.index_type = VK_INDEX_TYPE_UINT32;
	model.p->index_data.index_offset = 0;
	model.p->index_data.index_count = sizeof(mesh_indices) / sizeof(mesh_indices[0]);

	Memory_Byte_Buffer byte_buffer{sizeof(mesh_indices), mesh_indices};
	    
	Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	    
	Vulkan_API::invoke_staging_buffer_for_device_local_buffer(byte_buffer
								  , command_pool.p
								  , ibo.p
								  , gpu);
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

    struct Uniform_Buffer_Object
    {
	alignas(16) glm::mat4 model_matrix;
	alignas(16) glm::mat4 view_matrix;
	alignas(16) glm::mat4 projection_matrix;
    };





	
    internal void
    init_descriptor_pool(Vulkan_API::Swapchain *swapchain
			 , Vulkan_API::GPU *gpu)
    {
	Vulkan_API::Registered_Descriptor_Pool descriptor_pool = Vulkan_API::register_object("descriptor_pool.test_descriptor_pool"_hash
											     , sizeof(Vulkan_API::Descriptor_Pool));
	    
	VkDescriptorPoolSize pool_sizes[2] = {};

	Vulkan_API::init_descriptor_pool_size(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, swapchain->images.size, &pool_sizes[0]);
	Vulkan_API::init_descriptor_pool_size(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, swapchain->images.size, &pool_sizes[1]);
    
	Vulkan_API::init_descriptor_pool(Memory_Buffer_View<VkDescriptorPoolSize>{2, pool_sizes}, swapchain->images.size, gpu, descriptor_pool.p);
    } 

    internal void
    init_descriptor_sets(Vulkan_API::Swapchain *swapchain
			 , Vulkan_API::GPU *gpu)
    {
	u32 set_count = swapchain->images.size;

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

	Vulkan_API::Registered_Descriptor_Pool descriptor_pool = Vulkan_API::get_object("descriptor_pool.test_descriptor_pool"_hash);
	    
	Vulkan_API::allocate_descriptor_sets(descriptor_sets_separate
					     , Memory_Buffer_View<VkDescriptorSetLayout>{set_count, descriptor_set_layouts}
					     , gpu
					     , &descriptor_pool.p->pool);

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

	    Vulkan_API::update_descriptor_sets(Memory_Buffer_View<VkWriteDescriptorSet>{2, descriptor_writes}, gpu);
	}
    }


	
    internal void
    init_command_buffers(Vulkan_API::Swapchain *swapchain
			 , Vulkan_API::GPU *gpu
			 , const Vulkan_API::Registered_Descriptor_Set &descriptor_sets)
    {
	Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);

	Vulkan_API::Registered_Command_Buffer command_buffers = Vulkan_API::register_object("command_buffer.main_command_buffers"_hash
											    , sizeof(VkCommandBuffer) * swapchain->images.size);
	    
	Vulkan_API::allocate_command_buffers(command_pool.p
					     , VK_COMMAND_BUFFER_LEVEL_PRIMARY
					     , gpu
					     , Memory_Buffer_View<VkCommandBuffer>{command_buffers.size, command_buffers.p});

	Vulkan_API::Registered_Render_Pass render_pass = Vulkan_API::get_object("render_pass.test_render_pass"_hash);

	Vulkan_API::Registered_Graphics_Pipeline pipeline_ptr = Vulkan_API::get_object("pipeline.main_pipeline"_hash);

	Vulkan_API::Registered_Model model = Vulkan_API::get_object("vulkan_model.test_model"_hash);
	Vulkan_API::Registered_Buffer &vbo = model.p->bindings[0].buffer;
	Vulkan_API::Registered_Buffer &ibo = model.p->index_data.index_buffer;

	for (u32 i = 0
		 ; i < command_buffers.size
		 ; ++i)
	{
	    Vulkan_API::begin_command_buffer(&command_buffers.p[i], VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT, nullptr);

	    Vulkan_API::Registered_Framebuffer fbo = swapchain->framebuffers.extract(i);

	    VkClearValue clears[2] {Vulkan_API::init_clear_color_color(0, 0, 0, 0), Vulkan_API::init_clear_color_depth(1.0f, 0)};
		
	    Vulkan_API::command_buffer_begin_render_pass(render_pass.p
							 , fbo.p
							 , Vulkan_API::init_render_area({0, 0}, swapchain->extent)
							 , Memory_Buffer_View<VkClearValue>{2, clears}
							 , VK_SUBPASS_CONTENTS_INLINE
							 , &command_buffers.p[i]);

	    Vulkan_API::command_buffer_bind_pipeline(pipeline_ptr.p, &command_buffers.p[i]);

	    VkDeviceSize offset[] = {0};
	    Vulkan_API::command_buffer_bind_vbos(model.p->raw_cache_for_rendering
						 , Memory_Buffer_View<VkDeviceSize>{model.p->raw_cache_for_rendering.count, offset}
						 , 0, 1
						 , &command_buffers.p[i]);

	    Vulkan_API::command_buffer_bind_ibo(model.p->index_data
						, &command_buffers.p[i]);

	    Vulkan_API::Descriptor_Set *descriptor_set = &descriptor_sets.p[i];
	    Vulkan_API::command_buffer_bind_descriptor_sets(pipeline_ptr.p
							    , Memory_Buffer_View<VkDescriptorSet>{1, &descriptor_set->set}
							    , &command_buffers.p[i]);

	    Vulkan_API::Draw_Indexed_Data index_data;
	    index_data.index_count = model.p->index_data.index_count;
	    index_data.instance_count = 1;
	    index_data.first_index = 0;
	    index_data.vertex_offset = 0;
	    index_data.first_instance = 0;
	    Vulkan_API::command_buffer_draw_indexed(&command_buffers.p[i]
						    , index_data);
	    
	    Vulkan_API::command_buffer_end_render_pass(&command_buffers.p[i]);
	    Vulkan_API::end_command_buffer(&command_buffers.p[i]);
	}
    }
    
    internal constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;
	
    internal void
    init_sync(Vulkan_API::GPU *gpu)
    {
	Vulkan_API::Registered_Fence fences = Vulkan_API::register_object("fence.main_fences"_hash, sizeof(VkFence) * MAX_FRAMES_IN_FLIGHT);
	Vulkan_API::Registered_Semaphore image_ready_semaphores = Vulkan_API::register_object("semaphore.image_ready"_hash, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);
	Vulkan_API::Registered_Semaphore render_finished_semaphores = Vulkan_API::register_object("semaphore.render_finished"_hash, sizeof(VkSemaphore) * MAX_FRAMES_IN_FLIGHT);

	for (u32 i = 0
		 ; i < MAX_FRAMES_IN_FLIGHT
		 ; ++i)
	{
	    Vulkan_API::init_fence(gpu, VK_FENCE_CREATE_SIGNALED_BIT, &fences.p[i]);
	    Vulkan_API::init_semaphore(gpu, &render_finished_semaphores.p[i]);
	    Vulkan_API::init_semaphore(gpu, &image_ready_semaphores.p[i]);
	}
    }
    
    void
    init_rendering_state(Vulkan_API::State *vulkan_state
			 , Rendering_State *cache)
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

	init_descriptor_pool(&vulkan_state->swapchain
			     , &vulkan_state->gpu);

	init_descriptor_sets(&vulkan_state->swapchain
			     , &vulkan_state->gpu);

	init_command_buffers(&vulkan_state->swapchain
			     , &vulkan_state->gpu
			     , Vulkan_API::get_object("descriptor_set.test_descriptor_sets"_hash));

	init_sync(&vulkan_state->gpu);
	
	cache->test_render_pass = Vulkan_API::get_object("render_pass.test_render_pass"_hash);
	cache->descriptor_set_layout = Vulkan_API::get_object("descriptor_set_layout.test_descriptor_set_layout"_hash);
	cache->descriptor_sets = Vulkan_API::get_object("descriptor_set.test_descriptor_sets"_hash);
	cache->graphics_pipeline = Vulkan_API::get_object("pipeline.main_pipeline"_hash);
	cache->graphics_command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	cache->depth_image = Vulkan_API::get_object("image2D.depth_image"_hash);
	cache->texture = Vulkan_API::get_object("image2D.texture"_hash);
	cache->command_buffers = Vulkan_API::get_object("command_buffer.main_command_buffers"_hash);

	cache->fences = Vulkan_API::get_object("fence.main_fences"_hash);
	cache->image_ready_semaphores = Vulkan_API::get_object("semaphore.image_ready"_hash);
	cache->render_finished_semaphores = Vulkan_API::get_object("semaphore.render_finished"_hash);
	cache->test_model = Vulkan_API::get_object("vulkan_model.test_model"_hash);
    }







    // almost acts like the actual render component object itself
    struct Material
    {
	// possibly uniform data or data to be submitted to a instance buffer of something
	void *data = nullptr;
	u32 data_size = 0;

	Vulkan_API::Registered_Model model;
	Vulkan_API::Draw_Indexed_Data draw_info;
    };

    struct Renderer // renders a material type
    {
	// shader, pipieline states...
	Vulkan_API::Registered_Graphics_Pipeline ppln;
	Memory_Buffer_View<VkDescriptorSet> sets;
	VkShaderStageFlags mtrl_unique_data_stage_dst;
	
	Memory_Buffer_View<Material> mtrls;
	
	u32 mtrl_count;

	enum Render_Method { INLINE, INSTANCED } mthd;

	// reuse for different render passes (e.g. for example for rendering shadow maps, reflection / refraction...)
	VkCommandBuffer rndr_cmdbuf;
	
	void
	init(Renderer_Init_Data *data
	     , VkCommandPool *cmdpool
	     , Vulkan_API::GPU *gpu)
	{
	    mtrl_count = 0;
	    allocate_memory_buffer(mtrls, data->mtrl_max);

	    u32 set_count = 0;
	    for (u32 i = 0; i < data->descriptor_sets.count; ++i)
	    {
		Vulkan_API::Registered_Descriptor_Set tmp_set = Vulkan_API::get_object(data->descriptor_sets[i]);
		set_count += tmp_set.size;
	    }
	    
	    allocate_memory_buffer(sets, set_count);
	    for (u32 i = 0; i < sets.count; ++i)
	    {
		u32 p = i;
		Vulkan_API::Registered_Descriptor_Set tmp_set = Vulkan_API::get_object(data->descriptor_sets[i]);
		for (; i < p + tmp_set.size; ++i)
		{
		    sets[i] = tmp_set.p[i].set;
		}
	    }

	    ppln = Vulkan_API::get_object(data->ppln_id);
	    mtrl_unique_data_stage_dst = data->mtrl_unique_data_stage_dst;

	    Vulkan_API::allocate_command_buffers(cmdpool
						 , VK_COMMAND_BUFFER_LEVEL_SECONDARY
						 , gpu
						 , Memory_Buffer_View<VkCommandBuffer>{1, &rndr_cmdbuf});
	}
	
	void
	update(const Memory_Buffer_View<VkDescriptorSet> &additional_sets)
	{
	    Vulkan_API::begin_command_buffer(&rndr_cmdbuf, 0, nullptr);
	    
	    switch(mthd)
	    {	
	    case Render_Method::INLINE:
		{
		    Vulkan_API::command_buffer_bind_pipeline(ppln.p, &rndr_cmdbuf);

		    Vulkan_API::command_buffer_bind_descriptor_sets(ppln.p
								    , additional_sets
								    , &rndr_cmdbuf);

		    for (u32 i = 0; i < mtrl_count; ++i)
		    {
			Material *mtrl = &mtrls[i];

			Memory_Buffer_View<VkDeviceSize> offsets;
			allocate_memory_buffer_tmp(offsets, mtrl->model.p->binding_count);
			offsets.zero();
			
			Vulkan_API::command_buffer_bind_vbos(mtrl->model.p->raw_cache_for_rendering
							     , offsets
							     , 0
							     , mtrl->model.p->binding_count
							     , &rndr_cmdbuf);
			
			Vulkan_API::command_buffer_bind_ibo(mtrl->model.p->index_data
							    , &rndr_cmdbuf);

			Vulkan_API::command_buffer_push_constant(mtrl->data
								 , mtrl->data_size
								 , 0
								 , mtrl_unique_data_stage_dst
								 , ppln.p
								 , &rndr_cmdbuf);

			Vulkan_API::command_buffer_draw_indexed(&rndr_cmdbuf
								, mtrl->draw_info);
		    }
		}break;
	    case Render_Method::INSTANCED:
		{
		    
		}break;
	    };

	    Vulkan_API::end_command_buffer(&rndr_cmdbuf);
	}
    };
    
    global_var struct Render_System
    {
	static constexpr u32 MAX_RNDRS = 20;
	
	// 1 renderer per material type
	Memory_Buffer_View<Renderer> rndrs;
	u32 rndr_count;
	Hash_Table_Inline<s32, 30, 4, 3> rndr_id_map;


	Vulkan_API::Registered_Render_Pass deferred_rndr_pass;
	
	Vulkan_API::Registered_Framebuffer fbos;
	
	Vulkan_API::Registered_Graphics_Pipeline deferred_pipeline;
	Vulkan_API::Registered_Descriptor_Set_Layout deferred_descriptor_set_layout;
	Vulkan_API::Registered_Descriptor_Set deferred_descriptor_set;

	Vulkan_API::Registered_Buffer ppfx_quad;
	
	
	Render_System(void) :rndr_id_map("") {}

	void
	init_system(Vulkan_API::Swapchain *swapchain
		    , Vulkan_API::GPU *gpu)
	{
	    allocate_memory_buffer(rndrs, MAX_RNDRS);
	    rndr_count = 0;
	}
	
	void
	add_renderer(Renderer_Init_Data *init_data, VkCommandPool *cmdpool, Vulkan_API::GPU *gpu)
	{
	    rndrs[rndr_count].init(init_data, cmdpool, gpu);
	    rndr_id_map.insert(init_data->rndr_id.hash, rndr_count++);
	}
	
	void
	update(VkCommandBuffer *cmdbuf
	       , VkExtent2D swapchain_extent
	       , u32 image_index
	       , const Memory_Buffer_View<VkDescriptorSet> &additional_sets
	       , Vulkan_API::Registered_Render_Pass rndr_pass)
	{
	    VkClearValue clears[] {Vulkan_API::init_clear_color_color(0, 0, 0, 0)
		    , Vulkan_API::init_clear_color_depth(1.0f, 0)};
		    //		    , Vulkan_API::init_clear_color_color(0, 0, 0, 0)
		    //		    , Vulkan_API::init_clear_color_color(0, 0, 0, 0)
		    //		    , Vulkan_API::init_clear_color_color(0, 0, 0, 0)};

	    //	    VkClearValue clears[2] {Vulkan_API::init_clear_color_color(0, 0, 0, 0), Vulkan_API::init_clear_color_depth(1.0f, 0)};
	    
	    Vulkan_API::command_buffer_begin_render_pass(deferred_rndr_pass.p
							 , &fbos.p[image_index]
							 , Vulkan_API::init_render_area({0, 0}, swapchain_extent)
							 , Memory_Buffer_View<VkClearValue> {sizeof(clears) / sizeof(clears[0]), clears}
							 , VK_SUBPASS_CONTENTS_INLINE
							 , cmdbuf);

	    Memory_Buffer_View<VkCommandBuffer> cmds;
	    allocate_memory_buffer_tmp(cmds, rndr_count);
	    cmds.zero();
	    
	    for (u32 i = 0; i < rndr_count; ++i)
	    {
		rndrs.buffer[i].update(additional_sets);
		cmds.buffer[i] = rndrs.buffer[i].rndr_cmdbuf;
	    }

	    Vulkan_API::command_buffer_execute_commands(cmdbuf
							, cmds);
	    
	    /*	    Vulkan_API::command_buffer_next_subpass(cmdbuf
						    , VK_SUBPASS_CONTENTS_INLINE);
	    
	    Vulkan_API::command_buffer_bind_pipeline(deferred_pipeline.p
						     , cmdbuf);
	    
	    Vulkan_API::command_buffer_bind_descriptor_sets(deferred_pipeline.p
							    , Memory_Buffer_View<VkDescriptorSet>{1, &deferred_descriptor_set.p->set}
							    , cmdbuf);
	    Vulkan_API::command_buffer_draw(cmdbuf
					    , 3, 1, 0, 0);*/

	    /*	    Vulkan_API::command_buffer_execute_commands(cmdbuf
		    , cmds);*/
	    
	    Vulkan_API::command_buffer_end_render_pass(cmdbuf);
	}

	Material_Access
	init_material(const Constant_String &rndr_id
		      , const Material_Data *i)
	{
	    s32 rndr_index = *rndr_id_map.get(rndr_id.hash);
	    Renderer *rndr = &rndrs[rndr_index];

	    s32 mtrl_index = rndr->mtrl_count;
	    Material *mtrl = &rndr->mtrls[mtrl_index];
	    mtrl->data = i->data;
	    mtrl->data_size = i->data_size;

	    mtrl->model = i->model;
	    mtrl->draw_info = i->draw_info;

	    ++rndr->mtrl_count;

	    return(Material_Access{rndr_index, mtrl_index});
	}
    } rndr_sys;

    void
    init_deferred_render_pass(Vulkan_API::GPU *gpu
			      , Vulkan_API::Swapchain *swapchain)
    {
	/* init main renderpass */
	rndr_sys.deferred_rndr_pass = Vulkan_API::register_object("render_pass.deferred_render_pass"_hash
								  , sizeof(Vulkan_API::Render_Pass));
	
	enum G_Buffer_Attachment :u32 {FINAL_ATTACHMENT
				       , DEPTH_ATTACHMENT
				       , INVALID_ATTACHMENT};
	/*				       , ALBEDO_ATTACHMENT
				       , POSITION_ATTACHMENT
				       , NORMAL_ATTACHMENT*/
	
	VkAttachmentDescription descriptions[INVALID_ATTACHMENT] = {};
	descriptions[FINAL_ATTACHMENT] = Vulkan_API::init_attachment_description(swapchain->format
										 , VK_SAMPLE_COUNT_1_BIT
										 , VK_ATTACHMENT_LOAD_OP_CLEAR
										 , VK_ATTACHMENT_STORE_OP_STORE
										 , VK_ATTACHMENT_LOAD_OP_DONT_CARE
										 , VK_ATTACHMENT_STORE_OP_DONT_CARE
										 , VK_IMAGE_LAYOUT_UNDEFINED
										 , VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);

	/*	descriptions[ALBEDO_ATTACHMENT] = Vulkan_API::init_attachment_description(VK_FORMAT_R8G8B8A8_UNORM
										  , VK_SAMPLE_COUNT_1_BIT
										  , VK_ATTACHMENT_LOAD_OP_CLEAR
										  , VK_ATTACHMENT_STORE_OP_STORE
										  , VK_ATTACHMENT_LOAD_OP_DONT_CARE
										  , VK_ATTACHMENT_STORE_OP_DONT_CARE
										  , VK_IMAGE_LAYOUT_UNDEFINED
										  , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	descriptions[POSITION_ATTACHMENT] = Vulkan_API::init_attachment_description(VK_FORMAT_R16G16B16A16_SFLOAT
										    , VK_SAMPLE_COUNT_1_BIT
										    , VK_ATTACHMENT_LOAD_OP_CLEAR
										    , VK_ATTACHMENT_STORE_OP_STORE
										    , VK_ATTACHMENT_LOAD_OP_DONT_CARE
										    , VK_ATTACHMENT_STORE_OP_DONT_CARE
										    , VK_IMAGE_LAYOUT_UNDEFINED
										    , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

	descriptions[NORMAL_ATTACHMENT] = Vulkan_API::init_attachment_description(VK_FORMAT_R16G16B16A16_SFLOAT
										  , VK_SAMPLE_COUNT_1_BIT
										  , VK_ATTACHMENT_LOAD_OP_CLEAR
										  , VK_ATTACHMENT_STORE_OP_STORE
										  , VK_ATTACHMENT_LOAD_OP_DONT_CARE
										  , VK_ATTACHMENT_STORE_OP_DONT_CARE
										  , VK_IMAGE_LAYOUT_UNDEFINED
										  , VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);*/
	
	descriptions[DEPTH_ATTACHMENT] = Vulkan_API::init_attachment_description(gpu->supported_depth_format
										 , VK_SAMPLE_COUNT_1_BIT
										 , VK_ATTACHMENT_LOAD_OP_CLEAR
										 , VK_ATTACHMENT_STORE_OP_DONT_CARE
										 , VK_ATTACHMENT_LOAD_OP_DONT_CARE
										 , VK_ATTACHMENT_STORE_OP_DONT_CARE
										 , VK_IMAGE_LAYOUT_UNDEFINED
										 , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	enum G_Buffer_Reference :u32 {FINAL_REFERENCE
				      , DEPTH_REFERENCE
				      , INVALID_REFERENCE};
	/*				      , ALBEDO_REFERENCE
				      , POSITION_REFERENCE
				      , NORMAL_REFERENCE*/
	VkAttachmentReference references[INVALID_REFERENCE] = {};
	
	references[FINAL_REFERENCE] = Vulkan_API::init_attachment_reference(FINAL_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	/*	references[ALBEDO_REFERENCE] = Vulkan_API::init_attachment_reference(ALBEDO_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	references[POSITION_REFERENCE] = Vulkan_API::init_attachment_reference(POSITION_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
	references[NORMAL_REFERENCE] = Vulkan_API::init_attachment_reference(NORMAL_ATTACHMENT, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);*/
	references[DEPTH_REFERENCE] = Vulkan_API::init_attachment_reference(DEPTH_ATTACHMENT, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

	enum Subpass_Stage :u32 {G_BUFFER_INIT, LIGHTING, INVALID_STAGE};
	VkSubpassDescription subpasses[INVALID_STAGE] = {};
	
	subpasses[G_BUFFER_INIT] = Vulkan_API::init_subpass_description(Memory_Buffer_View<VkAttachmentReference>{1, references}
									    , &references[DEPTH_ATTACHMENT]
									    , null_buffer<VkAttachmentReference>());

	/*subpasses[LIGHTING] = Vulkan_API::init_subpass_description(single_buffer(references)
								   , &references[DEPTH_REFERENCE]
								   , Memory_Buffer_View<VkAttachmentReference>{3, &references[ALBEDO_REFERENCE]});*/

	VkSubpassDependency dependencies[3] = {};
	dependencies[0] = Vulkan_API::init_subpass_dependency(VK_SUBPASS_EXTERNAL
							      , 0
							      , VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
							      , VK_ACCESS_MEMORY_READ_BIT
							      , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
							      , VK_ACCESS_COLOR_ATTACHMENT_READ_BIT
							      | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
							      , VK_DEPENDENCY_BY_REGION_BIT);

	/*dependencies[1] = Vulkan_API::init_subpass_dependency(0
							      , 1
							      , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
							      , VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
							      , VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT
							      , VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
							      , VK_DEPENDENCY_BY_REGION_BIT);*/

	dependencies[1] = Vulkan_API::init_subpass_dependency(0
							      , VK_SUBPASS_EXTERNAL
							      , VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
							      , VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT
							      , VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT
							      , VK_ACCESS_MEMORY_READ_BIT
							      , VK_DEPENDENCY_BY_REGION_BIT);
	
	Memory_Buffer_View<VkAttachmentDescription> descriptions_array	= {2, descriptions};
	Memory_Buffer_View<VkSubpassDescription> subpasses_array	= {1, subpasses};
	Memory_Buffer_View<VkSubpassDependency> dependencies_array	= {2, dependencies};
	
	Vulkan_API::init_render_pass(descriptions_array
				     , subpasses_array
				     , dependencies_array
				     , gpu
				     , rndr_sys.deferred_rndr_pass.p);
    }

    void
    init_deferred_fbos(Vulkan_API::GPU *gpu
		       , Vulkan_API::Swapchain *swapchain
		       , Vulkan_API::Registered_Render_Pass rndr_pass)
    {
	enum class G_Buffer_Attachment :u32 {FINAL, ALBEDO, POSITION, NORMAL, DEPTH, INVALID};

	rndr_sys.fbos = Vulkan_API::register_object("framebuffer.main_fbo"_hash, sizeof(Vulkan_API::Framebuffer) * swapchain->images.size);
	for (u32 i = 0; i < swapchain->images.size; ++i)
	{
	    //allocate_memory_buffer(rndr_sys.fbos.p[i].color_attachments, (u32)G_Buffer_Attachment::INVALID - 1);
	    allocate_memory_buffer(rndr_sys.fbos.p[i].color_attachments, 1);
	}

	auto create_attachment = [&gpu, &swapchain](const Constant_String &name
						    , VkFormat format
						    , VkImageUsageFlagBits usage
						    , G_Buffer_Attachment attachment) -> Vulkan_API::Registered_Image2D
	{
	    Vulkan_API::Registered_Image2D img = Vulkan_API::register_object(name, sizeof(Vulkan_API::Image2D));

	    Vulkan_API::init_framebuffer_attachment(swapchain->extent.width
						    , swapchain->extent.height
						    , format
						    , usage
						    , gpu
						    , img.p);

	    return(img);
	};

	auto albedo = create_attachment("image2D.fbo_albedo"_hash, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, G_Buffer_Attachment::ALBEDO);
	auto position = create_attachment("image2D.fbo_position"_hash, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, G_Buffer_Attachment::POSITION);
	auto normal = create_attachment("image2D.fbo_normal"_hash, VK_FORMAT_R16G16B16A16_SFLOAT, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, G_Buffer_Attachment::NORMAL);
	auto depth = create_attachment("image2D.fbo_depth"_hash, gpu->supported_depth_format, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, G_Buffer_Attachment::DEPTH);

	for (u32 i = 0; i < swapchain->images.size; ++i)
	{ 
	    rndr_sys.fbos.p[i].color_attachments[0] = swapchain->images.extract(i);
	    /*	    rndr_sys.fbos.p[i].color_attachments[1] = albedo;
	    rndr_sys.fbos.p[i].color_attachments[2] = position;
	    rndr_sys.fbos.p[i].color_attachments[3] = normal;*/

	    rndr_sys.fbos.p[i].depth_attachment = depth;

	    Vulkan_API::init_framebuffer(rndr_pass.p
					 , swapchain->extent.width
					 , swapchain->extent.height
					 , gpu
					 , &rndr_sys.fbos.p[i]);
	    /*	    Vulkan_API::init_framebuffer(rndr_sys.deferred_rndr_pass.p
					 , swapchain->extent.width
					 , swapchain->extent.height
					 , gpu
					 , &rndr_sys.fbos.p[i]);*/
	}
    }

    void
    init_ppfx(Vulkan_API::GPU *gpu)
    {
	rndr_sys.ppfx_quad = Vulkan_API::register_object("buffer.ppfx_quad"_hash, sizeof(Vulkan_API::Buffer));
	
	f32 vertices [] =
	{
	    -1.0f, -1.0f, 0.0f, 0.0f,
	    -1.0f, +1.0f, 0.0f, 1.0f,
	    +1.0f, -1.0f, 1.0f, 0.0f,
	    +1.0f, +1.0f, 1.0f, 1.0f
	};
	    
	Vulkan_API::Registered_Command_Pool command_pool = Vulkan_API::get_object("command_pool.graphics_command_pool"_hash);
	    
	Vulkan_API::invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer{sizeof (vertices), vertices }
								      , command_pool.p
								      , rndr_sys.ppfx_quad.p
								      , gpu);
    }
    
    void
    init_deferred_ppln(Vulkan_API::GPU *gpu
		       , Vulkan_API::Swapchain *swapchain)
    {
	// create descriptor set
	rndr_sys.deferred_descriptor_set_layout = Vulkan_API::register_object("descriptor_set_layout.deferred_layout"_hash, sizeof(VkDescriptorSetLayout));
	    
	VkDescriptorSetLayoutBinding bindings[] = 
	{
	    Vulkan_API::init_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 0, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
	    , Vulkan_API::init_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
	    , Vulkan_API::init_descriptor_set_layout_binding(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, 1, VK_SHADER_STAGE_FRAGMENT_BIT)
	};
	    
	Vulkan_API::init_descriptor_set_layout(Memory_Buffer_View<VkDescriptorSetLayoutBinding>{3, bindings}
						   , gpu
						   , rndr_sys.deferred_descriptor_set_layout.p);

	Vulkan_API::Registered_Descriptor_Pool descriptor_pool = Vulkan_API::register_object("descriptor_pool.deferred_sets_pool"_hash, sizeof(Vulkan_API::Descriptor_Pool));
	rndr_sys.deferred_descriptor_set = Vulkan_API::register_object("descriptor_set.deferred_descriptor_sets"_hash, sizeof(Vulkan_API::Descriptor_Set));

	VkDescriptorPoolSize pool_sizes[1] = {};

	Vulkan_API::init_descriptor_pool_size(VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3, &pool_sizes[0]);
	Vulkan_API::init_descriptor_pool(Memory_Buffer_View<VkDescriptorPoolSize>{1, pool_sizes}, 3, gpu, descriptor_pool.p);

	auto sets_sb = single_buffer(&rndr_sys.deferred_descriptor_set);
	
	Vulkan_API::allocate_descriptor_sets(sets_sb
					     , Memory_Buffer_View<VkDescriptorSetLayout>{1, rndr_sys.deferred_descriptor_set_layout.p}
					     , gpu
					     , &descriptor_pool.p->pool);
	
	

	VkDescriptorImageInfo image_infos [3] = {};
	Vulkan_API::Registered_Image2D albedo_image = Vulkan_API::get_object("image2D.fbo_albedo"_hash);
	Vulkan_API::Registered_Image2D position_image = Vulkan_API::get_object("image2D.fbo_position"_hash);
	Vulkan_API::Registered_Image2D normal_image = Vulkan_API::get_object("image2D.fbo_normal"_hash);
	
	Vulkan_API::init_descriptor_set_image_info(albedo_image.p, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &image_infos[0]);
	Vulkan_API::init_descriptor_set_image_info(position_image.p, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &image_infos[1]);
	Vulkan_API::init_descriptor_set_image_info(normal_image.p, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, &image_infos[2]);

	VkWriteDescriptorSet descriptor_writes[3] = {};
	Vulkan_API::init_input_attachment_descriptor_set_write(rndr_sys.deferred_descriptor_set.p, 0, 0, 1, &image_infos[0], &descriptor_writes[0]);
	Vulkan_API::init_input_attachment_descriptor_set_write(rndr_sys.deferred_descriptor_set.p, 1, 0, 1, &image_infos[1], &descriptor_writes[1]);
	Vulkan_API::init_input_attachment_descriptor_set_write(rndr_sys.deferred_descriptor_set.p, 2, 0, 1, &image_infos[2], &descriptor_writes[2]);

	Vulkan_API::update_descriptor_sets(Memory_Buffer_View<VkWriteDescriptorSet>{3, descriptor_writes}, gpu);

	// actually create the pipeline
	rndr_sys.deferred_pipeline = Vulkan_API::register_object("pipeline.deferred_pipeline"_hash
								 , sizeof(Vulkan_API::Graphics_Pipeline));
	
	Vulkan_API::Registered_Graphics_Pipeline &graphics_pipeline = rndr_sys.deferred_pipeline;
	
	graphics_pipeline.p->stages = Vulkan_API::Graphics_Pipeline::Shader_Stages_Bits::VERTEX_SHADER_BIT
	    | Vulkan_API::Graphics_Pipeline::Shader_Stages_Bits::FRAGMENT_SHADER_BIT;
	graphics_pipeline.p->base_dir_and_name = "../vulkan/shaders/";
	graphics_pipeline.p->descriptor_set_layout = Vulkan_API::get_object("descriptor_set_layout.deferred_layout"_hash);

	auto set_layout_sb = single_buffer(graphics_pipeline.p->descriptor_set_layout.p);
	auto null_b = null_buffer<VkPushConstantRange>();
	Vulkan_API::init_pipeline_layout(&set_layout_sb
					 , &null_b
					 , gpu
					 , &graphics_pipeline.p->layout);
	
	// create shaders
	File_Contents vsh_bytecode = read_file("shaders/SPV/deferred_lighting.vert.spv");
	File_Contents fsh_bytecode = read_file("shaders/SPV/deferred_lighting.frag.spv");
	
	VkShaderModule vsh_module;
	Vulkan_API::init_shader(VK_SHADER_STAGE_VERTEX_BIT, vsh_bytecode.size, vsh_bytecode.content, gpu, &vsh_module);
	
	VkShaderModule fsh_module;
	Vulkan_API::init_shader(VK_SHADER_STAGE_FRAGMENT_BIT, fsh_bytecode.size, fsh_bytecode.content, gpu, &fsh_module);

	VkPipelineShaderStageCreateInfo module_infos[2] = {};
	Vulkan_API::init_shader_pipeline_info(&vsh_module, VK_SHADER_STAGE_VERTEX_BIT, &module_infos[0]);
	Vulkan_API::init_shader_pipeline_info(&fsh_module, VK_SHADER_STAGE_FRAGMENT_BIT, &module_infos[1]);

	// init vertex input stuff
	VkVertexInputBindingDescription binding_description = Vulkan_API::init_binding_description(0, sizeof(f32) * 4, VK_VERTEX_INPUT_RATE_VERTEX);
	VkVertexInputAttributeDescription attribute_descriptions[] =
	{
	    Vulkan_API::init_attribute_description(0, 0, VK_FORMAT_R32G32_SFLOAT, 0),
	    Vulkan_API::init_attribute_description(0, 1, VK_FORMAT_R32G32_SFLOAT, sizeof(f32) * 2)
	};
	
	VkPipelineVertexInputStateCreateInfo vertex_input_info = Vulkan_API::init_pipeline_vertex_input_info(Memory_Buffer_View<VkVertexInputBindingDescription>{1, &binding_description}
													     , Memory_Buffer_View<VkVertexInputAttributeDescription>{2, attribute_descriptions});

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
	Vulkan_API::init_pipeline_rasterization_info(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, 1.0f, 0, &rasterization_info);

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

	// init depth stencil info
	VkPipelineDepthStencilStateCreateInfo depth_stencil_info = {};
	Vulkan_API::init_pipeline_depth_stencil_info(VK_TRUE, VK_TRUE, 0.0f, 1.0f, VK_FALSE, &depth_stencil_info);

	// init pipeline object
	Vulkan_API::Registered_Render_Pass render_pass = rndr_sys.deferred_rndr_pass;
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
					   , 0 // now should be 0
					   , gpu
					   , &graphics_pipeline.p->pipeline);

	vkDestroyShaderModule(gpu->logical_device, vsh_module, nullptr);
	vkDestroyShaderModule(gpu->logical_device, fsh_module, nullptr);
    }
    void
    init_rendering_system(Vulkan_API::Swapchain *swapchain, Vulkan_API::GPU *gpu, Vulkan_API::Registered_Render_Pass rndr_pass)
    {
	rndr_sys.init_system(swapchain, gpu);
	
	init_deferred_render_pass(gpu, swapchain);
	init_deferred_fbos(gpu, swapchain, rndr_pass);
	init_ppfx(gpu);
	init_deferred_ppln(gpu, swapchain);
    }

    void
    add_renderer(Renderer_Init_Data *init_data, VkCommandPool *cmdpool, Vulkan_API::GPU *gpu)
    {
	rndr_sys.add_renderer(init_data, cmdpool, gpu);
    }

    void
    update_renderers(VkCommandBuffer *cmdbuf
		     , VkExtent2D swapchain_extent
		     , u32 image_index
		     , const Memory_Buffer_View<VkDescriptorSet> &additional_sets
		     , Vulkan_API::Registered_Render_Pass rndr_pass)
    {
	rndr_sys.update(cmdbuf, swapchain_extent, image_index, additional_sets, rndr_pass);
    }

    Material_Access
    init_material(const Constant_String &id
		  , const Material_Data *data)
    {
	return(rndr_sys.init_material(id, data));
    }

}
