#pragma once

#include "core.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_handles.hpp"

// struct data with "r_" are required, data with "o_" are optional
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
	struct Allocate_GPU_Memory_Params
	{
	    GPU *r_gpu;
	    VkDeviceSize r_allocation_size;
	    VkMemoryPropertyFlags r_properties;
	    VkMemoryRequirements r_memory_requirements;
	};

	extern void
	allocate_gpu_memory(Allocate_GPU_Memory_Params *params
			    , VkDeviceMemory *dest_memory);
    }

    struct Buffer
    {
	VkBuffer buffer_object;
	VkDeviceMemory buffer_memory;
	VkDeviceSize buffer_size;
    };

    struct Create_Buffer_Params
    {
	VkDeviceSize r_buffer_size;
	VkBufferUsageFlags r_usage;
	VkSharingMode r_sharing_mode;
    };

    extern void
    create_buffer(Create_Buffer_Params *params
		  , Buffer *dest_buffer);

    struct Texture
    {
	VkImage image;
	VkImageView image_view;
	// optional : ex: swapchain images won't store the VkDeviceMemory objects in the Image struct
	VkDeviceMemory memory;
    };

    struct Create_Image_Params
    {
	uint32 r_width, r_height;
	VkFormat r_format;
	VkImageTiling r_tiling;
	VkImageUsageFlags r_usage;
	VkMemoryPropertyFlags r_properties;
	GPU *r_gpu;
    };
    
    extern void
    init_image(Create_Image_Params *params
		 , VkImage *dest_image);

    struct Create_Image_View_Params
    {
	VkImage *r_image;
	VkFormat r_format;
	VkImageAspectFlags r_aspect_flags;
	GPU *r_gpu;
    };

    extern void
    init_image_view(Create_Image_View_Params *params
		      , VkImageView *dest_image_view);

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
    
    struct Render_Pass_Create_Params
    {
	uint32 r_attachment_description_count;
	VkAttachmentDescription *r_attachment_descriptions;
	uint32 r_subpass_count;
	VkSubpassDescription *r_subpasses;
	uint32 r_dependency_count;
	VkSubpassDependency *r_dependencies;

	GPU *r_gpu;
    };
    
    extern void
    init_render_pass(Render_Pass_Create_Params *params
		     , Render_Pass *dest_render_pass);

    struct Shader_Module_Create_Params
    {
	VkShaderStageFlagBits r_stage_bits;
	uint32 r_content_size;
	byte *r_file_contents;
	GPU *r_gpu;
    };
    
    struct Shader_Module
    {
	VkShaderStageFlagBits stage_bits;
	VkShaderModule shader;
    };

    extern void
    init_shader(Shader_Module_Create_Params *params
		, Shader_Module *dest_shader_module);

    extern void
    init_shader_pipeline_info(Shader_Module *module
			      , VkPipelineShaderStageCreateInfo *dest_info);

    // describes the binding of a buffer to a model VAO
    struct Model_Binding
    {
	// buffer that stores all the attributes
	Buffer_Handle buffer;
	uint32 binding;
	VkVertexInputRate input_rate;

	VkVertexInputAttributeDescription *attribute_list = nullptr;
	uint32 stride = 0;
	
	void
	begin_attributes_creation(VkVertexInputAttributeDescription *attribute_list)
	{
	    this->attribute_list = attribute_list;
	}
	
	void
	push_attribute(uint32 location, VkFormat format, uint32 size)
	{
	    VkVertexInputAttributeDescription attribute = {};
	    attribute.binding = binding;
	    attribute.location = location;
	    attribute.format = format;
	    attribute.offset = stride;

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
	enum Shader_Stages_Bits { VERTEX_SHADER_BIT = 1 << 0
				  , GEOMETRY_SHADER_BIT = 1 << 1
				  , TESSELATION_SHADER_BIT = 1 << 2
				  , FRAGMENT_SHADER = 1 << 3};

	Shader_Stages_Bits stages;
	// "[some_dir]/[name]_[vert/frag/tess/geom].spv"
	const char *base_dir_and_name;

	Vulkan_API::Descriptor_Set_Layout_Handle descriptor_set_layout;
	VkPipelineLayout layout;
    };

    // creating pipelines takes a LOT of params
    extern void
    init_pipeline_vertex_input_info(Model *model /* model contains input information required */
				    , VkPipelineVertexInputStateCreateInfo *info);

    struct Input_Assembly_Create_Params
    {
	// maybe for future use?
	VkPipelineInputAssemblyStateCreateFlags flags;
	VkPrimitiveTopology topology;
	VkBool32 primitive_restart;
    };
    
    extern void
    init_pipeline_input_assembly_info(Input_Assembly_Create_Params *params
				      , VkPipelineInputAssemblyStateCreateInfo *info);

    extern void
    init_pipeline_viewport_info();

    extern void
    init_pipeline_rasterization_info();

    extern void
    init_pipeline_multisampling_info();

    extern void
    init_pipeline_blending_info();

    extern void
    init_pipeline_dynamic_states_info();

    extern void
    init_pipeline_layout_info();

    extern void
    init_pipeline_depth_stencil_info();

    extern void
    init_graphics_pipeline( /* ... */ );
    
    struct State
    {
	VkInstance instance;
	VkDebugUtilsMessengerEXT debug_messenger;
	GPU gpu;
	VkSurfaceKHR surface;
	Swapchain swapchain;
    };

    /* entry point for vulkan stuff */
    extern void
    init_state(State *state
	       , GLFWwindow *window);

}
