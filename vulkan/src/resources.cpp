#include "resources.hpp"

extern_impl Vk_Buffer_Resource_Manager buffer_manager;

extern_impl void
create_buffer(VkBuffer *write
	    , VkDeviceSize size
	    , VkBufferUsageFlags usage
	    , VkMemoryPropertyFlags properties
	    , VkDeviceMemory *memory
	    , VkSharingMode sharing_mode)
{
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size;
    buffer_info.usage = usage;
    buffer_info.sharingMode = sharing_mode;
    buffer_info.flags = 0;

    VK_CHECK(vkCreateBuffer(vk.device, &buffer_info, nullptr, write)
	     , "failed to create buffer\n");

    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vk.device, *write, &memory_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = memory_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(memory_requirements.memoryTypeBits, properties);
    
    VK_CHECK(vkAllocateMemory(vk.device, &alloc_info, nullptr, memory)
	     , "failed to allocate buffer memory\n");

    vkBindBufferMemory(vk.device, *write, *memory, 0);
}

buffer_handle
Vk_Buffer_Resource_Manager::release(void)
{
    return(buffer_release_count++);
}
