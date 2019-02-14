#pragma once

#include "core.hpp"
#include <vulkan/vulkan.h>

struct Queue_Family_Indices
{
    int32 graphics_family = -1;
    int32 present_family = -1;

    inline bool
    complete(void)
    {
	return graphics_family >= 0 && present_family >= 0;
    }
};

struct Vulkan_GPU
{
    VkPhysicalDevice hardware;
    Queue_Family_Indices queue_families;

    void
    find_families(void);
};

extern struct Vulkan_State
{
    
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    Vulkan_GPU gpu;
    VkDevice device;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapchain;

} vk;

extern void
init_vk(void);
