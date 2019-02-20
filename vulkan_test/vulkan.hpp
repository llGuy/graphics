#pragma once

#include "core.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

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
    
    extern_impl void
    init_render_pass(Render_Pass_Create_Params *params
		     , Render_Pass *dest_render_pass);
    
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
