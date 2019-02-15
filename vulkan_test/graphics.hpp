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

struct Swapchain
{
    VkSwapchainKHR swapchain;
    VkFormat format;
    VkPresentModeKHR present_mode;
    VkExtent2D extent;

    uint32 image_count;
    VkImage *images;
    VkImageView *image_views;

    VkFramebuffer *fbos;
    uint32 fbo_count;
};

extern struct Vulkan_State
{
    VkInstance instance;
    VkDebugUtilsMessengerEXT debug_messenger;
    Vulkan_GPU gpu;
    VkDevice device;
    VkSurfaceKHR surface;

    Swapchain swapchain;

    VkQueue graphics_queue;
    VkQueue present_queue;

    VkRenderPass render_pass;
    VkDescriptorSetLayout descriptor_layout;
    VkPipeline graphics_pipeline;
    VkPipelineLayout pipeline_layout;

    VkCommandPool command_pool;

    VkImage depth_image;
    VkImageView depth_image_view;

    VkImage texture_image;
    VkImageView texture_image_view;
} vk;

extern void
init_vk(void);

extern void
recreate(void);

extern void
destroy_vk(void);
