#pragma once

#include "core.hpp"
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
	uint32 width, height;
	VkFormat format;
	VkImageTiling tiling;
	VkImageUsageFlags usage;
	VkMemoryPropertyFlags properties;
    };
    
    extern void
    create_image(Create_Image_Params *params
		 , VkImage *dest_image);

    struct Swapchain
    {
	VkSurfaceFormatKHR format;
	VkPresentModeKHR present_mode;
	VkSwapchainKHR swapchain;
    };

    struct State
    {
	VkInstance instance;
	GPU gpu;
	VkSurfaceKHR surface;
	Swapchain swapchain;
    };



    /* entry point for vulkan stuff */
    extern void
    init_state(State *state);

}
