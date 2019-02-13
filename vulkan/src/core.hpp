#ifndef _CORE_HPP_
#define _CORE_HPP_

#include <vector>
#include <vulkan/vulkan.h>
#include <stdio.h>
#include <malloc.h>
#include <stdint.h>

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float float32;

typedef uint8 byte;

#define global static
#define persist static
#define internal static
#define extern_impl

global constexpr int32 WINDOW_WIDTH = 800;
global constexpr int32 WINDOW_HEIGHT = 600;

#define VK_CHECK(f, ...) \
    if (f != VK_SUCCESS) \
    { \
     printf("[%s:%d] error : %s - ", __FILE__, __LINE__, #f);	\
     printf(__VA_ARGS__); \
     throw std::runtime_error(""); \
     } \
    else\
    { \
     printf("[%s:%d] success : %s\n", __FILE__, __LINE__, #f);	\
    }
#define API_CHECK(f, ...) if (!f) { printf("error : %s - ", #f); printf(__VA_ARGS__); throw std::runtime_error(""); }
#define STACK_ALLOC(type, count) (type *)alloca(sizeof(type) * count)

struct Queue_Family_Indices
{
    int32 graphics_queue_family {-1};
    int32 present_queue_family {-1};

    void
    find(VkPhysicalDevice device);

    inline bool
    complete(void) { return graphics_queue_family >= 0 && present_queue_family >= 0; }
};

extern struct Swapchain
{
    VkSwapchainKHR handle;
    VkExtent2D extent;
    VkFormat format;
    std::vector<VkImage> images;
    std::vector<VkImageView> image_views;
} swapchain;

// describes the basic vulkan pillars
extern struct Vulkan_State
{
    VkInstance instance;
    VkPhysicalDevice physical_device;
    VkDevice device;
    VkSurfaceKHR surface;

    Queue_Family_Indices queue_family_indices;
    VkQueue graphics_queue;
    VkQueue present_queue;
} vk;

struct Swapchain_Support_Details
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> present_modes;
};

struct Graphics_Pipeline
{
    VkDescriptorSetLayout set_layout;
    VkPipelineLayout pipeline_layout;
    
    VkPipeline handle;
};

extern struct Game_Graphics
{
    Graphics_Pipeline pipeline;
    VkRenderPass main_render_pass;
} graphics;

struct Byte_Array
{
    byte *heap_array;
    uint32 size;
};

internal Byte_Array
read_file(const char *file_name)
{
    FILE *file = fopen(file_name, "rb");
    if (!file)
    {
	printf("unable to open file %s\n", file_name);
	return(Byte_Array{nullptr, 0});
    }
    fseek(file, 0L, SEEK_END);
    uint32 size = ftell(file);

    rewind(file);

    Byte_Array array { (byte *)malloc(sizeof(byte) * size), size };
    fread(array.heap_array, size * sizeof(byte), 1, file);

    return(array);
}

internal uint32
find_memory_type(uint32 type_filter
		 , VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(vk.physical_device, &mem_properties);

    for (uint32 i = 0
	     ; i < mem_properties.memoryTypeCount
	     ; ++i)
    {
	if (type_filter & (1 << i)
	    && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
	{
	    return(i);
	}
    }

    throw std::runtime_error("failed to find suitable memory type");
}

#endif
