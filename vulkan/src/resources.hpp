#ifndef _RESOURCES_H_
#define _RESOURCES_H_

#include "core.hpp"
#include <vulkan/vulkan.h>

#define MAX_BUFFERS 20

typedef uint32 buffer_handle;

// user accesses the buffer through a uint32 handle
struct Vk_Buffer
{
    VkBuffer handle;
    VkDeviceMemory memory;
};

extern struct Vk_Buffer_Resource_Manager
{
    uint32 buffer_release_count = 0;
    Vk_Buffer buffers[MAX_BUFFERS] = {};

    buffer_handle
    release(void);
} buffer_manager;

extern void
create_buffer(VkBuffer *write
	    , VkDeviceSize size
	    , VkBufferUsageFlags usage
	    , VkMemoryPropertyFlags properties
	    , VkDeviceMemory *memory
	    , VkSharingMode sharing_mode);

#endif
