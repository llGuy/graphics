#pragma once

#include <memory.h>
#include "core.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_managers.hpp"

namespace Vulkan_API
{
    
    struct Queue_Families
    {
	s32 graphics_family = -1;
	s32 present_family = 1;

	inline bool
	complete(void)
	{
	    return(graphics_family >= 0 && present_family >= 0);
	}
    };

    struct Swapchain_Details
    {
	VkSurfaceCapabilitiesKHR capabilities;
	u32 available_formats_count;
	VkSurfaceFormatKHR *available_formats;
	u32 available_present_modes_count;
	VkPresentModeKHR *available_present_modes;
    };
    
    struct GPU
    {
	VkPhysicalDevice hardware;
	VkDevice logical_device;

	VkPhysicalDeviceMemoryProperties memory_properties;

	Queue_Families queue_families;
	VkQueue graphics_queue;
	VkQueue present_queue;

	Swapchain_Details swapchain_support;
	VkFormat supported_depth_format;

	void
	find_queue_families(VkSurfaceKHR *surface);
    };

    namespace Memory
    {

	// allocates memory for stuff like buffers and images
	void
	allocate_gpu_memory(VkMemoryPropertyFlags properties
			    , VkMemoryRequirements memory_requirements
			    , GPU *gpu
			    , VkDeviceMemory *dest_memory);
    }
    
    struct Mapped_GPU_Memory
    {
	u32 offset;
	VkDeviceSize size;
	VkDeviceMemory *memory;
	void *data;

	FORCEINLINE void
	begin(GPU *gpu)
	{
	    vkMapMemory(gpu->logical_device, *memory, offset, size, 0, &data);
	}

	FORCEINLINE void
	fill(Memory_Byte_Buffer byte_buffer)
	{
	    memcpy(data, byte_buffer.ptr, size);
	}
	
	FORCEINLINE void
	end(GPU *gpu)
	{
	    vkUnmapMemory(gpu->logical_device, *memory);
	}
    };

    struct Buffer
    {
	VkBuffer buffer;
	VkDeviceMemory memory;
	VkDeviceSize size;

	FORCEINLINE Mapped_GPU_Memory
	construct_map(void)
	{
	    return(Mapped_GPU_Memory{0, size, &memory});
	}
    };

    void
    init_buffer(VkDeviceSize buffer_size
		  , VkBufferUsageFlags usage
		  , VkSharingMode sharing_mode
		  , VkMemoryPropertyFlags memory_properties
		  , GPU *gpu
		  , Buffer *dest_buffer);

    
    struct Image2D
    {
	VkImage image = VK_NULL_HANDLE;
	VkImageView image_view = VK_NULL_HANDLE;
	VkSampler image_sampler = VK_NULL_HANDLE;
	VkDeviceMemory device_memory = VK_NULL_HANDLE;

	inline VkMemoryRequirements
	get_memory_requirements(GPU *gpu)
	{
	    VkMemoryRequirements requirements = {};
	    vkGetImageMemoryRequirements(gpu->logical_device, image, &requirements);

	    return(requirements);
	}
    };
    
    void
    init_image(u32 width
	       , u32 height
	       , VkFormat format
	       , VkImageTiling tiling
	       , VkImageUsageFlags usage
	       , VkMemoryPropertyFlags properties
	       , GPU *gpu
	       , Image2D *dest_image);

    void
    init_image_view(VkImage *image
		    , VkFormat format
		    , VkImageAspectFlags aspect_flags
		    , GPU *gpu
		    , VkImageView *dest_image_view);

    void
    init_image_sampler(VkFilter mag_filter
		       , VkFilter min_filter
		       , VkSamplerAddressMode u_sampler_address_mode
		       , VkSamplerAddressMode v_sampler_address_mode
		       , VkSamplerAddressMode w_sampler_address_mode
		       , VkBool32 anisotropy_enable
		       , u32 max_anisotropy
		       , VkBorderColor clamp_border_color
		       , VkBool32 compare_enable
		       , VkCompareOp compare_op
		       , VkSamplerMipmapMode mipmap_mode
		       , f32 mip_lod_bias
		       , f32 min_lod
		       , f32 max_lod
		       , GPU *gpu
		       , VkSampler *dest_sampler);

    struct Swapchain
    {
	VkFormat format;
	VkPresentModeKHR present_mode;
	VkSwapchainKHR swapchain;
	VkExtent2D extent;
	
	Registered_Image2D images;
	Registered_Framebuffer framebuffers;
    };
    
    struct Render_Pass
    {
	VkRenderPass render_pass;
	u32 subpass_count;
    };
    
    void
    init_render_pass(Memory_Buffer_View<VkAttachmentDescription> *attachment_descriptions
		     , Memory_Buffer_View<VkSubpassDescription> *subpass_descriptions
		     , Memory_Buffer_View<VkSubpassDependency> *subpass_dependencies
		     , GPU *gpu
		     , Render_Pass *dest_render_pass);
    void
    init_shader(VkShaderStageFlagBits stage_bits
		, u32 content_size
		, byte *file_contents
		, GPU *gpu
		, VkShaderModule *dest_shader_module);

    // describes the binding of a buffer to a model VAO
    struct Model_Binding
    {
	// buffer that stores all the attributes
	Registered_Buffer buffer;
	u32 binding;
	VkVertexInputRate input_rate;

	VkVertexInputAttributeDescription *attribute_list = nullptr;
	u32 attribute_count = 0;
	u32 stride = 0;
	
	void
	begin_attributes_creation(VkVertexInputAttributeDescription *attribute_list)
	{
	    this->attribute_list = attribute_list;
	    attribute_count = 0;
	}
	
	void
	push_attribute(u32 location, VkFormat format, u32 size)
	{
	    VkVertexInputAttributeDescription *attribute = &attribute_list[attribute_count++];
	    
	    attribute->binding = binding;
	    attribute->location = location;
	    attribute->format = format;
	    attribute->offset = stride;

	    stride += size;
	}

	void
	end_attributes_creation(void)
	{
	    attribute_list = nullptr;
	}
    };

    struct Model_Index_Data
    {
	Registered_Buffer index_buffer;
	u32 index_count;
    };
    
    // describes the attributes and bindings of the model
    struct Model
    {
	// model bindings
	u32 binding_count;
	// allocated on free list allocator
	Model_Binding *bindings;
	// model attriutes
	u32 attribute_count;
	// allocated on free list also | multiple bindings can push to this buffer
	VkVertexInputAttributeDescription *attributes_buffer;

	Model_Index_Data index_data;
	
	VkVertexInputBindingDescription *
	create_binding_descriptions(void)
	{
	    VkVertexInputBindingDescription *descriptions = (VkVertexInputBindingDescription *)allocate_stack(sizeof(VkVertexInputBindingDescription) * binding_count
													      , 1
													      , "binding_total_list_allocation");
	    for (u32 i = 0; i < binding_count; ++i)
	    {
		descriptions[i].binding = bindings[i].binding;
		descriptions[i].stride = bindings[i].stride;
		descriptions[i].inputRate = bindings[i].input_rate;
	    }
	    return(descriptions);
	}

	void
	create_vertex_input_state_info(VkPipelineVertexInputStateCreateInfo *info)
	{
	    VkVertexInputBindingDescription *binding_descriptions = create_binding_descriptions();

	    info->sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	
	    info->vertexBindingDescriptionCount = binding_count;
	    info->pVertexBindingDescriptions = binding_descriptions;
	    info->vertexAttributeDescriptionCount = attribute_count;
	    info->pVertexAttributeDescriptions = attributes_buffer;
	}
    };

    struct Graphics_Pipeline
    {
	enum Shader_Stages_Bits : s32 { VERTEX_SHADER_BIT = 1 << 0
					  , GEOMETRY_SHADER_BIT = 1 << 1
					  , TESSELATION_SHADER_BIT = 1 << 2
					  , FRAGMENT_SHADER_BIT = 1 << 3};

	s32 stages;
	// "[some_dir]/[name]_"
	const char *base_dir_and_name;

	Registered_Descriptor_Set_Layout descriptor_set_layout;
	
	VkPipelineLayout layout;

	VkPipeline pipeline;
    };

    // creating pipelines takes a LOT of params
    internal inline void
    init_shader_pipeline_info(VkShaderModule *module
			      , VkShaderStageFlagBits stage_bits
			      , VkPipelineShaderStageCreateInfo *dest_info)
    {
	dest_info->sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	dest_info->stage = stage_bits;
	dest_info->module = *module;
	dest_info->pName = "main";	
    }
    
    internal inline void
    init_pipeline_vertex_input_info(Model *model /* model contains input information required */
				    , VkPipelineVertexInputStateCreateInfo *info)
    {
	model->create_vertex_input_state_info(info);
    }
    
    internal inline void
    init_pipeline_input_assembly_info(VkPipelineInputAssemblyStateCreateFlags flags
				      , VkPrimitiveTopology topology
				      , VkBool32 primitive_restart
				      , VkPipelineInputAssemblyStateCreateInfo *info)
    {
	info->sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	info->topology = topology;
	info->primitiveRestartEnable = primitive_restart;
    }

    internal inline void
    init_viewport(u32 width
		  , u32 height
		  , f32 min_depth
		  , f32 max_depth
		  , VkViewport *viewport)
    {
	viewport->x = 0.0f;
	viewport->y = 0.0f;
	viewport->width = (f32)width;
	viewport->height = (f32)height;
	viewport->minDepth = min_depth;
	viewport->maxDepth = max_depth;
    }

    internal inline void
    init_rect_2D(VkOffset2D offset
		 , VkExtent2D extent
		 , VkRect2D *rect)
    {
	rect->offset = offset;
	rect->extent = extent;
    }
    
    internal inline void
    init_pipeline_viewport_info(Memory_Buffer_View<VkViewport> *viewports
				, Memory_Buffer_View<VkRect2D> *scissors
				, VkPipelineViewportStateCreateInfo *info)
    {
	info->sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	info->viewportCount = viewports->count;
	info->pViewports = viewports->buffer;
	info->scissorCount = scissors->count;
	info->pScissors = scissors->buffer;;
    }
				
    
    internal inline void
    init_pipeline_rasterization_info(VkPolygonMode polygon_mode
				     , VkCullModeFlags cull_flags
				     , f32 line_width
				     , VkPipelineRasterizationStateCreateFlags flags
				     , VkPipelineRasterizationStateCreateInfo *info)
    {
	info->sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	info->depthClampEnable = VK_FALSE;
	info->rasterizerDiscardEnable = VK_FALSE;
	info->polygonMode = polygon_mode;
	info->lineWidth = line_width;
	info->cullMode = cull_flags;
	info->frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	info->depthBiasEnable = VK_FALSE;
	info->depthBiasConstantFactor = 0.0f;
	info->depthBiasClamp = 0.0f;
	info->depthBiasSlopeFactor = 0.0f;
	info->flags = flags;
    }
    
    internal inline void
    init_pipeline_multisampling_info(VkSampleCountFlagBits rasterization_samples
				     , VkPipelineMultisampleStateCreateFlags flags
				     , VkPipelineMultisampleStateCreateInfo *info)
    {
	info->sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	info->sampleShadingEnable = VK_FALSE;
	info->rasterizationSamples = rasterization_samples;
	info->minSampleShading = 1.0f;
	info->pSampleMask = nullptr;
	info->alphaToCoverageEnable = VK_FALSE;
	info->alphaToOneEnable = VK_FALSE;
	info->flags = flags;
    }

    internal inline void
    init_blend_state_attachment(VkColorComponentFlags color_write_flags
				, VkBool32 enable_blend
				, VkBlendFactor src_color
				, VkBlendFactor dst_color
				, VkBlendOp color_op
				, VkBlendFactor src_alpha
				, VkBlendFactor dst_alpha
				, VkBlendOp alpha_op
				, VkPipelineColorBlendAttachmentState *attachment)
    {

	attachment->colorWriteMask = color_write_flags;
	attachment->blendEnable = enable_blend;
	attachment->srcColorBlendFactor = src_color;
	attachment->dstColorBlendFactor = dst_color;
	attachment->colorBlendOp = color_op;
	attachment->srcAlphaBlendFactor = src_alpha;
	attachment->dstAlphaBlendFactor = dst_alpha;
	attachment->alphaBlendOp = alpha_op;
    }
    
    internal inline void
    init_pipeline_blending_info(VkBool32 enable_logic_op
				, VkLogicOp logic_op
				, Memory_Buffer_View<VkPipelineColorBlendAttachmentState> *states
				, VkPipelineColorBlendStateCreateInfo *info)
    {
	info->sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	info->logicOpEnable = enable_logic_op;
	info->logicOp = logic_op;
	info->attachmentCount = states->count;
	info->pAttachments = states->buffer;
	info->blendConstants[0] = 0.0f;
	info->blendConstants[1] = 0.0f;
	info->blendConstants[2] = 0.0f;
	info->blendConstants[3] = 0.0f;
    }

    internal inline void
    init_pipeline_dynamic_states_info(Memory_Buffer_View<VkDynamicState> *dynamic_states
				      , VkPipelineDynamicStateCreateInfo *info)
    {
	info->sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	info->dynamicStateCount = dynamic_states->count;
	info->pDynamicStates = dynamic_states->buffer;
    }

    void
    init_pipeline_layout(Memory_Buffer_View<VkDescriptorSetLayout> *layouts
			 , Memory_Buffer_View<VkPushConstantRange> *ranges
			 , GPU *gpu
			 , VkPipelineLayout *pipeline_layout);

    internal inline void
    init_pipeline_depth_stencil_info(VkBool32 depth_test_enable
				     , VkBool32 depth_write_enable
				     , f32 min_depth_bounds
				     , f32 max_depth_bounds
				     , VkBool32 stencil_enable
				     , VkPipelineDepthStencilStateCreateInfo *info)
    {
	info->sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	info->depthTestEnable = depth_test_enable;
	info->depthWriteEnable = depth_write_enable;
	info->depthCompareOp = VK_COMPARE_OP_LESS;
	info->depthBoundsTestEnable = VK_FALSE;
	info->minDepthBounds = min_depth_bounds;
	info->maxDepthBounds = max_depth_bounds;
	info->stencilTestEnable = VK_FALSE;
	info->front = {};
	info->back = {};
    }

    void
    init_graphics_pipeline(Memory_Buffer_View<VkPipelineShaderStageCreateInfo> *shaders
			   , VkPipelineVertexInputStateCreateInfo *vertex_input_info
			   , VkPipelineInputAssemblyStateCreateInfo *input_assembly_info
			   , VkPipelineViewportStateCreateInfo *viewport_info
			   , VkPipelineRasterizationStateCreateInfo *rasterization_info
			   , VkPipelineMultisampleStateCreateInfo *multisample_info
			   , VkPipelineColorBlendStateCreateInfo *blend_info
			   , VkPipelineDynamicStateCreateInfo *dynamic_state_info
			   , VkPipelineDepthStencilStateCreateInfo *depth_stencil_info
			   , VkPipelineLayout *layout
			   , Render_Pass *render_pass
			   , u32 subpass
			   , GPU *gpu
			   , VkPipeline *pipeline);

    void
    allocate_command_pool(u32 queue_family_index
			  , GPU *gpu
			  , VkCommandPool *command_pool);

    void
    allocate_command_buffers(VkCommandPool *command_pool_source
			     , VkCommandBufferLevel level
			     , GPU *gpu
			     , const Memory_Buffer_View<VkCommandBuffer> &command_buffers);

    inline void
    free_command_buffer(const Memory_Buffer_View<VkCommandBuffer> &command_buffers
			, VkCommandPool *pool
			, GPU *gpu)
    {
	vkFreeCommandBuffers(gpu->logical_device
			     , *pool
			     , command_buffers.count
			     , command_buffers.buffer);
    }
    
    void
    begin_command_buffer(VkCommandBuffer *command_buffer
			 , VkCommandBufferUsageFlags usage_flags
			 , VkCommandBufferInheritanceInfo *inheritance);

    inline void
    end_command_buffer(VkCommandBuffer *command_buffer)
    {
	vkEndCommandBuffer(*command_buffer);
    }

    void
    submit(const Memory_Buffer_View<VkCommandBuffer> &command_buffers
	   , VkQueue *queue);

    void
    init_single_use_command_buffer(VkCommandPool *command_pool
				   , GPU *gpu
				   , VkCommandBuffer *dst);

    void
    destroy_single_use_command_buffer(VkCommandBuffer *command_buffer
				      , VkCommandPool *command_pool
				      , GPU *gpu);
    
    void
    transition_image_layout(VkImage *image
			    , VkFormat format
			    , VkImageLayout old_layout
			    , VkImageLayout new_layout
			    , VkCommandPool *graphics_command_pool
			    , GPU *gpu);

    void
    copy_buffer_into_image(Buffer *src_buffer
			   , Image2D *dst_image
			   , u32 width
			   , u32 height
			   , VkCommandPool *command_pool
			   , GPU *gpu);

    void
    copy_buffer(Buffer *src_buffer
		, Buffer *dst_buffer
		, VkCommandPool *command_pool
		, GPU *gpu);

    void
    invoke_staging_buffer_for_device_local_buffer(Memory_Byte_Buffer items
						  , VkCommandPool *transfer_command_pool
						  , Buffer *dst_buffer
						  , GPU *gpu);

    void
    invoke_staging_buffer_for_device_local_image();
    
    struct Framebuffer
    {
	VkFramebuffer framebuffer;

	// for color attachments only
	Registered_Image2D color_attachments;
	Registered_Image2D depth_attachment;
    };
    
    void
    init_framebuffer(Render_Pass *compatible_render_pass
		     , u32 width
		     , u32 height
		     , GPU *gpu
		     , Framebuffer *framebuffer); // need to initialize the attachment handles
    
    struct Descriptor_Set
    {
	Registered_Descriptor_Set_Layout layouts;
	VkDescriptorSet set;

	void
	init_buffer_descriptor_write(u32 binding
				     , VkDescriptorBufferInfo *buffer_info
				     , VkWriteDescriptorSet *descriptor_write)
	{
	    descriptor_write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    descriptor_write->dstSet = set;
	    descriptor_write->dstBinding = binding;
	    descriptor_write->dstArrayElement = 0;
	    descriptor_write->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	    descriptor_write->descriptorCount = 1;
	    descriptor_write->pBufferInfo = buffer_info;
	}

	void
	init_image_descriptor_write(u32 binding
				    , VkDescriptorImageInfo *image_info
				    , VkWriteDescriptorSet *descriptor_write)
	{
	    descriptor_write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	    descriptor_write->dstSet = set;
	    descriptor_write->dstBinding = binding;
	    descriptor_write->dstArrayElement = 0;
	    descriptor_write->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	    descriptor_write->descriptorCount = 1;
	    descriptor_write->pImageInfo = image_info;
	}
    };

    void
    allocate_descriptor_sets(Memory_Buffer_View<Descriptor_Set *> &descriptor_sets
			     , const Memory_Buffer_View<VkDescriptorSetLayout> &layouts
			     , GPU *gpu
			     , VkDescriptorPool *descriptor_pool);
    
    internal void
    init_descriptor_set_buffer_info(Buffer *buffer
				    , u32 offset_in_ubo
				    , VkDescriptorBufferInfo *buffer_info)
    {
	buffer_info->buffer = buffer->buffer;
	buffer_info->offset = offset_in_ubo;
	buffer_info->range = buffer->size;
    }

    internal void
    init_buffer_descriptor_set_write(Descriptor_Set *set
				     , u32 binding
				     , u32 dst_array_element
				     , u32 count
				     , VkDescriptorBufferInfo *infos
				     , VkWriteDescriptorSet *write)
    {
	write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write->dstSet = set->set;
	write->dstBinding = binding;
	write->dstArrayElement = dst_array_element;
	write->descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	write->descriptorCount = count;
	write->pBufferInfo = infos;
    }

    internal void
    init_image_descriptor_set_write(Descriptor_Set *set
				    , u32 binding
				    , u32 dst_array_element
				    , u32 count
				    , VkDescriptorImageInfo *infos
				    , VkWriteDescriptorSet *write)
    {
	write->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	write->dstSet = set->set;
	write->dstBinding = binding;
	write->dstArrayElement = dst_array_element;
	write->descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	write->descriptorCount = count;
	write->pImageInfo = infos;
    }
    
    internal void
    init_descriptor_set_image_info(Image2D *image
				   , VkImageLayout expected_layout
				   , VkDescriptorImageInfo *image_info)
    {
	image_info->imageLayout = expected_layout;
	image_info->imageView = image->image_view;
	image_info->sampler = image->image_sampler;
    }

    internal void
    init_descriptor_pool_size(VkDescriptorType type
			      , u32 count
			      , VkDescriptorPoolSize *size)
    {
	size->type = type;
	size->descriptorCount = count;
    }

    struct Descriptor_Pool
    {
	VkDescriptorPool pool;
    };
    
    void
    init_descriptor_pool(const Memory_Buffer_View<VkDescriptorPoolSize> &sizes
			 , u32 max_sets
			 , GPU *gpu
			 , Descriptor_Pool *pool);
    
    
    void
    update_descriptor_sets(const Memory_Buffer_View<VkWriteDescriptorSet> &writes
			   , GPU *gpu);
    
    struct State
    {
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	GPU gpu;
	VkSurfaceKHR surface;
	Swapchain swapchain;
    };

    /* entry point for vulkan stuff */
    void
    init_state(State *state
	       , GLFWwindow *window);

}
