#pragma once

#include "core.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

#include "vulkan.hpp"

extern Vulkan_API::State vulkan_state;

extern struct Vulkan_State
{
    //    VkInstance instance;
    //    VkDebugUtilsMessengerEXT debug_messenger;
    //    Vulkan_GPU gpu;
    //    VkDevice device;
    //    VkSurfaceKHR surface;

    //    Swapchain swapchain;

    //    VkQueue graphics_queue;
    //    VkQueue present_queue;

    //    VkRenderPass render_pass;
    //VkDescriptorSetLayout descriptor_layout;
    //    VkPipeline graphics_pipeline;
    //VkPipelineLayout pipeline_layout;

    //    VkCommandPool command_pool;

    //    VkImage depth_image;
    //    VkDeviceMemory depth_image_memory;
    //    VkImageView depth_image_view;

    VkImage texture_image;
    VkDeviceMemory texture_image_memory;
    VkImageView texture_image_view;
    VkSampler texture_image_sampler;

    VkBuffer vertex_buffer;
    VkDeviceMemory vertex_buffer_memory;

    VkBuffer index_buffer;
    VkDeviceMemory index_buffer_memory;

    u32 uniform_buffer_count;
    VkBuffer *uniform_buffers;
    VkDeviceMemory *uniform_buffers_memory;

    VkDescriptorPool descriptor_pool;
    VkDescriptorSet *descriptor_sets;
    u32 descriptor_set_count;

    u32 command_buffer_count;
    VkCommandBuffer *command_buffers;

    u32 semaphore_count;
    VkSemaphore *image_ready_semaphores;
    VkSemaphore *render_finished_semaphores;

    u32 fence_count;
    VkFence *fences;
} vk;

extern void
init_vk(GLFWwindow *window);

extern void
draw_frame(void);

extern void
recreate(void);

extern void
destroy_vk(void);
