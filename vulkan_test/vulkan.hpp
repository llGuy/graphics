#pragma once

#include "core.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_handles.hpp"

namespace Vulkan_API
{
    
    struct Queue_Families
    {
	int32 graphics_family = -1;
	int32 present_family = 1;

	inline bool
	complete(void)
	{
	    return(graphics_family >= 0 && present_family >= 0);
	}
    };

    struct Swapchain_Details
    {
	VkSurfaceCapabilitiesKHR capabilities;
	uint32 available_formats_count;
	VkSurfaceFormatKHR *available_formats;
	uint32 available_present_modes_count;
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

    struct Buffer
    {
	VkBuffer buffer_object;
	VkDeviceMemory buffer_memory;
	VkDeviceSize buffer_size;
    };

    void
    create_buffer(VkDeviceSize buffer_size
		  , VkBufferUsageFlags usage
		  , VkSharingMode sharing_mode
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
    init_image(uint32 width
	       , uint32 height
	       , VkFormat format
	       , VkImageTiling tiling
	       , VkImageUsageFlags usage
	       , VkMemoryPropertyFlags properties
	       , GPU *gpu
	       , VkImage *dest_image);

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
		       , uint32 max_anisotropy
		       , VkBorderColor clamp_border_color
		       , VkBool32 compare_enable
		       , VkCompareOp compare_op
		       , VkSamplerMipmapMode mipmap_mode
		       , float32 mip_lod_bias
		       , float32 min_lod
		       , float32 max_lod
		       , GPU *gpu
		       , VkSampler *dest_sampler);

    struct Swapchain
    {
	VkFormat format;
	VkPresentModeKHR present_mode;
	VkSwapchainKHR swapchain;
	VkExtent2D extent;

	VkImage *images;
	VkImageView *image_views;
	uint32 image_count;

	VkFramebuffer *fbos;
	uint32 fbo_count;
    };
    
    struct Render_Pass
    {
	VkRenderPass render_pass;
	uint32 subpass_count;
    };
    
    void
    init_render_pass(Memory_Buffer_View<VkAttachmentDescription> *attachment_descriptions
		     , Memory_Buffer_View<VkSubpassDescription> *subpass_descriptions
		     , Memory_Buffer_View<VkSubpassDependency> *subpass_dependencies
		     , GPU *gpu
		     , Render_Pass *dest_render_pass);
    void
    init_shader(VkShaderStageFlagBits stage_bits
		, uint32 content_size
		, byte *file_contents
		, GPU *gpu
		, VkShaderModule *dest_shader_module);

    // describes the binding of a buffer to a model VAO
    struct Model_Binding
    {
	// buffer that stores all the attributes
	Buffer_Handle buffer;
	uint32 binding;
	VkVertexInputRate input_rate;

	VkVertexInputAttributeDescription *attribute_list = nullptr;
	uint32 attribute_count = 0;
	uint32 stride = 0;
	
	void
	begin_attributes_creation(VkVertexInputAttributeDescription *attribute_list)
	{
	    this->attribute_list = attribute_list;
	    attribute_count = 0;
	}
	
	void
	push_attribute(uint32 location, VkFormat format, uint32 size)
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
    
    // describes the attributes and bindings of the model
    struct Model
    {
	// model bindings
	uint32 binding_count;
	// allocated on free list allocator
	Model_Binding *bindings;
	// model attriutes
	uint32 attribute_count;
	// allocated on free list also | multiple bindings can push to this buffer
	VkVertexInputAttributeDescription *attributes_buffer;

	VkVertexInputBindingDescription *
	create_binding_descriptions(void)
	{
	    VkVertexInputBindingDescription *descriptions = (VkVertexInputBindingDescription *)allocate_stack(sizeof(VkVertexInputBindingDescription) * binding_count
													      , 1
													      , "binding_total_list_allocation");
	    for (uint32 i = 0; i < binding_count; ++i)
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
	enum Shader_Stages_Bits : int32 { VERTEX_SHADER_BIT = 1 << 0
					  , GEOMETRY_SHADER_BIT = 1 << 1
					  , TESSELATION_SHADER_BIT = 1 << 2
					  , FRAGMENT_SHADER_BIT = 1 << 3};

	int32 stages;
	// "[some_dir]/[name]_"
	const char *base_dir_and_name;

	Vulkan_API::Descriptor_Set_Layout_Handle descriptor_set_layout;
	
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
    init_viewport(uint32 width
		  , uint32 height
		  , float32 min_depth
		  , float32 max_depth
		  , VkViewport *viewport)
    {
	viewport->x = 0.0f;
	viewport->y = 0.0f;
	viewport->width = (float32)width;
	viewport->height = (float32)height;
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
				     , float32 line_width
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
				     , float32 min_depth_bounds
				     , float32 max_depth_bounds
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
			   , uint32 subpass
			   , GPU *gpu
			   , VkPipeline *pipeline);

    void
    allocate_command_pool(uint32 queue_family_index
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
    transition_image_layout(VkImage *image
			    , VkFormat format
			    , VkImageLayout old_layout
			    , VkImageLayout new_layout
			    , VkCommandPool *graphics_command_pool
			    , GPU *gpu);

    struct Framebuffer
    {
	VkFramebuffer framebuffer;

	Memory_Buffer_View<Image2D_Handle> color_attachments;
	Image2D_Handle depth_attachment = UNINITIALIZED_HANDLE;
    };
    
    void
    init_framebuffer(Render_Pass *compatible_render_pass
		     , const Memory_Buffer_View<VkImageView> &attachments
		     , uint32 width
		     , uint32 height
		     , GPU *gpu
		     , Framebuffer *framebuffer); // need to initialize the attachment handles
    
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
