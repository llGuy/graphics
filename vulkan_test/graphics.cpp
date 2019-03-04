#define GLFW_INCLUDE_VULKAN

#include <cstring>
#include <cassert>
#include "core.hpp"
#include <limits.h>
#include <chrono>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <glm/glm.hpp>
#include "graphics.hpp"
#include "rendering.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_managers.hpp"
#include <glm/gtx/transform.hpp>

// TODO(luc) : remove these globals in the future when ported to the new system
Vulkan_State vk = {};
Vulkan_API::State vulkan_state = {};
Rendering::Rendering_Objects_Handle_Cache rendering_objects = {};

internal constexpr u32 required_device_extension_count = 1;
internal const char *required_physical_device_extensions[required_device_extension_count]
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

internal VkImageView
create_image_view(VkImage image
		  , VkFormat format
		  , VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo view_info		= {};
    view_info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image				= image;
    view_info.viewType				= VK_IMAGE_VIEW_TYPE_2D;
    view_info.format				= format;
    view_info.subresourceRange.aspectMask	= aspect_flags;
    view_info.subresourceRange.baseMipLevel	= 0;
    view_info.subresourceRange.levelCount	= 1;
    view_info.subresourceRange.baseArrayLayer	= 0;
    view_info.subresourceRange.layerCount	= 1;

    VkImageView image_view;
    VK_CHECK(vkCreateImageView(vulkan_state.gpu.logical_device, &view_info, nullptr, &image_view));

    return(image_view);
}

internal VkFormat
find_supported_format(const VkFormat *candidates
		      , u32 candidate_size
		      , VkImageTiling tiling
		      , VkFormatFeatureFlags features)
{
    for (u32 i = 0
	     ; i < candidate_size
	     ; ++i)
    {
	VkFormatProperties properties;
	vkGetPhysicalDeviceFormatProperties(vulkan_state.gpu.hardware, candidates[i], &properties);
	if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
	{
	    return(candidates[i]);
	}
	else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
	{
	    return(candidates[i]);
	}
    }
    OUTPUT_DEBUG_LOG("%s\n", "failed to find supported format");
    assert(false);

    return VkFormat{};
}

internal VkFormat
find_depth_format(void)
{
    VkFormat *formats	= (VkFormat *)allocate_stack(sizeof(VkFormat) * 3
						     , Alignment(1)
						     , "depth_format_list_allocation");

    formats[0]		= VK_FORMAT_D32_SFLOAT;
    formats[1]		= VK_FORMAT_D32_SFLOAT_S8_UINT;
    formats[2]		= VK_FORMAT_D24_UNORM_S8_UINT;
    
    VkFormat result	= find_supported_format(formats
						, 3
						, VK_IMAGE_TILING_OPTIMAL
						, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);

    pop_stack();

    return result;
}

internal void
init_render_pass(void)
{
    VkAttachmentDescription color	= {};
    color.format			= vulkan_state.swapchain.format;
    color.samples			= VK_SAMPLE_COUNT_1_BIT;
    color.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout			= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth	= {};
    depth.format			= find_depth_format();
    depth.samples			= VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp			= VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.stencilLoadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth.stencilStoreOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
    depth.finalLayout			= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference color_ref	= {};
    color_ref.attachment		= 0;
    color_ref.layout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref	= {};
    depth_ref.attachment		= 1;
    depth_ref.layout			= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass	= {};
    subpass.pipelineBindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount	= 1;
    subpass.pColorAttachments		= &color_ref;
    subpass.pDepthStencilAttachment	= &depth_ref;

    VkSubpassDependency dependency	= {};
    dependency.srcSubpass		= VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass		= 0;
    dependency.srcStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask		= 0;
    dependency.dstStageMask		= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask		= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkAttachmentDescription attachments[] = {color, depth};

    VkRenderPassCreateInfo render_pass_info	= {};
    render_pass_info.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount		= sizeof(attachments) / sizeof(attachments[0]);
    render_pass_info.pAttachments		= attachments;
    render_pass_info.subpassCount		= 1;
    render_pass_info.pSubpasses			= &subpass;
    render_pass_info.dependencyCount		= 1;
    render_pass_info.pDependencies		= &dependency;

    //    VK_CHECK(vkCreateRenderPass(vulkan_state.gpu.logical_device, &render_pass_info, nullptr, &vk.render_pass));
}

internal void
init_descriptor_layout(void)
{
    VkDescriptorSetLayoutBinding ubo_binding	= {};
    ubo_binding.binding				= 0;
    ubo_binding.descriptorType			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_binding.descriptorCount			= 1;
    ubo_binding.stageFlags			= VK_SHADER_STAGE_VERTEX_BIT;
    ubo_binding.pImmutableSamplers		= nullptr;

    VkDescriptorSetLayoutBinding sampler_binding	= {};
    sampler_binding.binding				= 1;
    sampler_binding.descriptorCount			= 1;
    sampler_binding.descriptorType			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_binding.pImmutableSamplers			= nullptr;
    sampler_binding.stageFlags				= VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = {ubo_binding, sampler_binding};

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    VkDescriptorSetLayout *descriptor_set_layout = Vulkan_API::get_descriptor_set_layout(rendering_objects.descriptor_set_layout);
    VK_CHECK(vkCreateDescriptorSetLayout(vulkan_state.gpu.logical_device, &layout_info, nullptr, descriptor_set_layout));
}

internal VkShaderModule
create_shader(File_Contents *contents)
{
    VkShaderModuleCreateInfo shader_info	= {};
    shader_info.sType				= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize			= contents->size;
    shader_info.pCode				= reinterpret_cast<const u32 *>(contents->content);

    VkShaderModule shader_module;
    VK_CHECK(vkCreateShaderModule(vulkan_state.gpu.logical_device, &shader_info, nullptr, &shader_module));

    return(shader_module);
}    

struct Vertex
{
    glm::vec3 pos, color;
    glm::vec2 uvs;

    static VkVertexInputBindingDescription
    get_binding_description(void)
    {
	VkVertexInputBindingDescription binding_info	= {};
	binding_info.binding				= 0;
	binding_info.stride				= sizeof(Vertex);
	binding_info.inputRate				= VK_VERTEX_INPUT_RATE_VERTEX;

	return(binding_info);
    }

    struct Vertex_Attributes
    {
	static constexpr u32 attribute_count = 3;
	VkVertexInputAttributeDescription attributes[attribute_count];
    };

    static Vertex_Attributes
    get_attribute_descriptions(void)
    {
	Vertex_Attributes structure;
	structure.attributes[0].binding		= 0;
	structure.attributes[0].location	= 0;
	structure.attributes[0].format		= VK_FORMAT_R32G32B32_SFLOAT;
	structure.attributes[0].offset		= offsetof(Vertex, Vertex::pos);

	structure.attributes[1].binding		= 0;
	structure.attributes[1].location	= 1;
	structure.attributes[1].format		= VK_FORMAT_R32G32B32_SFLOAT;
	structure.attributes[1].offset		= offsetof(Vertex, Vertex::color);

	structure.attributes[2].binding		= 0;
	structure.attributes[2].location	= 2;
	structure.attributes[2].format		= VK_FORMAT_R32G32_SFLOAT;
	structure.attributes[2].offset		= offsetof(Vertex, Vertex::uvs);

	return(structure);
    }
};

internal Vertex vertices[]
{
    {{-1.0f, -0.5f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},
    {{-0.5f, 0.5f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

internal u32 mesh_indices[]
{
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

internal u32
find_memory_type(u32 type_filter
		 , VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(vulkan_state.gpu.hardware
					, &mem_properties);

    for (u32 i = 0
	     ; i < mem_properties.memoryTypeCount
	     ; ++i)
    {
	if (type_filter & (1 << i)
	    && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
	{
	    return(i);
	}
    }

    OUTPUT_DEBUG_LOG("%s\n", "failed to find memory type");
    assert(false);

    return(0);
}

internal void
create_image(u32 width
	     , u32 height
	     , VkFormat format
	     , VkImageTiling tiling
	     , VkImageUsageFlags usage
	     , VkMemoryPropertyFlags properties
	     , VkImage *image
	     , VkDeviceMemory *image_memory)
{
    VkImageCreateInfo image_info = {};
    image_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image_info.imageType = VK_IMAGE_TYPE_2D;
    image_info.extent.width = width;
    image_info.extent.height = height;
    image_info.extent.depth = 1;
    image_info.mipLevels = 1;
    image_info.arrayLayers = 1;
    image_info.format = format;
    image_info.tiling = tiling;
    image_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    image_info.usage = usage;
    image_info.samples = VK_SAMPLE_COUNT_1_BIT;
    image_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VK_CHECK(vkCreateImage(vulkan_state.gpu.logical_device, &image_info, nullptr, image));

    VkMemoryRequirements mem_requirements = {};
    vkGetImageMemoryRequirements(vulkan_state.gpu.logical_device
				 , *image
				 , &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits
						  , properties);

    VK_CHECK(vkAllocateMemory(vulkan_state.gpu.logical_device, &alloc_info, nullptr, image_memory));

    vkBindImageMemory(vulkan_state.gpu.logical_device, *image, *image_memory, 0);
}

internal VkCommandBuffer
begin_single_time_command(void)
{
    VkCommandPool *command_pool = Vulkan_API::get_command_pool(rendering_objects.graphics_command_pool);
    
    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = *command_pool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(vulkan_state.gpu.logical_device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer
			 , &begin_info);

    return(command_buffer);
}

internal void
end_single_time_command(VkCommandBuffer command_buffer)
{
    VkCommandPool *command_pool = Vulkan_API::get_command_pool(rendering_objects.graphics_command_pool);
    
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(vulkan_state.gpu.graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(vulkan_state.gpu.graphics_queue);

    vkFreeCommandBuffers(vulkan_state.gpu.logical_device, *command_pool, 1, &command_buffer);
}

internal bool
has_stencil_component(VkFormat format)
{
    return(format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT);
}

internal void
transition_image_layout(VkImage image
			, VkFormat format
			, VkImageLayout old_layout
			, VkImageLayout new_layout)
{
    VkCommandBuffer command_buffer = begin_single_time_command();

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;

    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   
    barrier.image				= image;
    barrier.subresourceRange.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel	= 0;
    barrier.subresourceRange.levelCount		= 1;
    barrier.subresourceRange.baseArrayLayer	= 0;
    barrier.subresourceRange.layerCount		= 1;

    VkPipelineStageFlags source_stage;
    VkPipelineStageFlags destination_stage;

    if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
    {
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

	source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
    {
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
	destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
	barrier.srcAccessMask = 0;
	barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT
	    | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    }
    else
    {
	OUTPUT_DEBUG_LOG("%s\n", "unsupported layout transition");
	assert(false);
    }

    if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
    {
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	if (has_stencil_component(format))
	{
	    barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
    }
    else
    {
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    vkCmdPipelineBarrier(command_buffer
			 , source_stage
			 , destination_stage
			 , 0
			 , 0
			 , nullptr
			 , 0
			 , nullptr
			 , 1
			 , &barrier);

    end_single_time_command(command_buffer);
}

/*internal void
init_depth_resources(void)
{
    VkFormat depth_format = find_depth_format();

    create_image(vulkan_state.swapchain.extent.width
		 , vulkan_state.swapchain.extent.height
		 , depth_format
		 , VK_IMAGE_TILING_OPTIMAL
		 , VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT
		 , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		 , &vk.depth_image
		 , &vk.depth_image_memory);
    
    vk.depth_image_view = create_image_view(vk.depth_image
					    , depth_format
					    , VK_IMAGE_ASPECT_DEPTH_BIT);
    transition_image_layout(vk.depth_image
			    , depth_format
			    , VK_IMAGE_LAYOUT_UNDEFINED
			    , VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}*/

internal void
init_framebuffers(void)
{
    /*vulkan_state.swapchain.fbos = (VkFramebuffer *)(allocate_stack(sizeof(VkFramebuffer) * vulkan_state.swapchain.images.count
								   , Alignment(1)
								   , "framebuffer_list_allocation"));

    Vulkan_API::Render_Pass *render_pass = Vulkan_API::get_render_pass(rendering_objects.test_render_pass);

    Vulkan_API::Image2D *depth_image = Vulkan_API::get_image2D(rendering_objects.depth_image);
    
    for (u32 i = 0
	     ; i < vulkan_state.swapchain.images.count
	     ; ++i)
    {
	Vulkan_API::Image2D *image = Vulkan_API::get_image2D(vulkan_state.swapchain.images.buffer[i]);

	VkImageView attachments[]
	{
	    image->image_view, depth_image->image_view
	};

	VkFramebufferCreateInfo fbo_info	= {};
	fbo_info.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbo_info.renderPass			= render_pass->render_pass;
	fbo_info.attachmentCount		= sizeof(attachments) / sizeof(attachments[0]);
	fbo_info.pAttachments			= attachments;
	fbo_info.width				= vulkan_state.swapchain.extent.width;
	fbo_info.height				= vulkan_state.swapchain.extent.height;
	fbo_info.layers				= 1;

	VK_CHECK(vkCreateFramebuffer(vulkan_state.gpu.logical_device, &fbo_info, nullptr, &vulkan_state.swapchain.fbos[i]));
    }*/
}

internal void
create_buffer(VkBuffer *write_buffer
	      , VkDeviceMemory *memory
	      , VkDeviceSize size
	      , VkBufferUsageFlags usage
	      , VkMemoryPropertyFlags properties)
{
    VkBufferCreateInfo buffer_info	= {};
    buffer_info.sType			= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size			= size;
    buffer_info.usage			= usage;
    buffer_info.sharingMode		= VK_SHARING_MODE_EXCLUSIVE;
    buffer_info.flags			= 0;

    VK_CHECK(vkCreateBuffer(vulkan_state.gpu.logical_device, &buffer_info, nullptr, write_buffer));

    VkMemoryRequirements mem_requirements;
    vkGetBufferMemoryRequirements(vulkan_state.gpu.logical_device, *write_buffer, &mem_requirements);

    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, properties);

    VK_CHECK(vkAllocateMemory(vulkan_state.gpu.logical_device, &alloc_info, nullptr, memory));

    vkBindBufferMemory(vulkan_state.gpu.logical_device, *write_buffer, *memory, 0);
}

internal void
copy_buffer(VkBuffer *__restrict src_buffer
	    , VkBuffer *__restrict dst_buffer
	    , VkDeviceSize size)
{
    VkCommandBuffer command_buffer = begin_single_time_command();

    VkBufferCopy copy_region = {};
    copy_region.size = size;
    vkCmdCopyBuffer(command_buffer, *src_buffer, *dst_buffer, 1, &copy_region);

    end_single_time_command(command_buffer);
}

internal void
copy_buffer_to_image(VkBuffer buffer
		     , VkImage image
		     , u32 w, u32 h)
{
    VkCommandBuffer command_buffer = begin_single_time_command();

    VkBufferImageCopy region	= {};
    region.bufferOffset		= 0;
    region.bufferRowLength	= 0;
    region.bufferImageHeight	= 0;

    region.imageSubresource.aspectMask		= VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel		= 0;
    region.imageSubresource.baseArrayLayer	= 0;
    region.imageSubresource.layerCount		= 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = { w, h, 1 };

    vkCmdCopyBufferToImage(command_buffer
			  , buffer
			  , image
			  , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			  , 1
			  , &region);

    end_single_time_command(command_buffer);
}

internal void
init_texture_image(void)
{
    persist const char *jpg_file = "../vulkan/textures/texture.jpg";

    s32 w, h, channels;
    stbi_uc *pixels = stbi_load(jpg_file, &w, &h, &channels, STBI_rgb_alpha);

    if (!pixels)
    {
	OUTPUT_DEBUG_LOG("failed to open image : %s\n", jpg_file);
    }

    VkDeviceSize image_size = w * h * 4;
    
    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    create_buffer(&staging_buffer
		  , &staging_buffer_memory
		  , image_size
		  , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		  , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vkMapMemory(vulkan_state.gpu.logical_device, staging_buffer_memory, 0, image_size, 0, &data);
    memcpy(data, pixels, static_cast<u32>(image_size));
    vkUnmapMemory(vulkan_state.gpu.logical_device, staging_buffer_memory);

    stbi_image_free(pixels);

    create_image(w
		 , h
		 , VK_FORMAT_R8G8B8A8_UNORM
		 , VK_IMAGE_TILING_OPTIMAL
		 , VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT
		 , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT
		 , &vk.texture_image
		 , &vk.texture_image_memory);
 
    transition_image_layout(vk.texture_image
			    , VK_FORMAT_R8G8B8A8_UNORM
			    , VK_IMAGE_LAYOUT_UNDEFINED
			    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    copy_buffer_to_image(staging_buffer
			 , vk.texture_image
			 , (u32)w, (u32)h);

    transition_image_layout(vk.texture_image
			    , VK_FORMAT_R8G8B8A8_UNORM
			    , VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL
			    , VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    vkDestroyBuffer(vulkan_state.gpu.logical_device, staging_buffer, nullptr);
    vkFreeMemory(vulkan_state.gpu.logical_device, staging_buffer_memory, nullptr);
}

internal void
init_texture_image_view(void)
{
    vk.texture_image_view = create_image_view(vk.texture_image
					      , VK_FORMAT_R8G8B8A8_UNORM
					      , VK_IMAGE_ASPECT_COLOR_BIT);
}

internal void
init_texture_image_sampler(void)
{
    VkSamplerCreateInfo sampler_info = {};
    sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    sampler_info.magFilter = VK_FILTER_LINEAR;
    sampler_info.minFilter = VK_FILTER_LINEAR;
    sampler_info.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    sampler_info.anisotropyEnable = VK_TRUE;
    sampler_info.maxAnisotropy = 16;
    sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK; // when clamping
    sampler_info.unnormalizedCoordinates = VK_FALSE;
    sampler_info.compareEnable = VK_FALSE;
    sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
    sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    sampler_info.mipLodBias = 0.0f;
    sampler_info.minLod = 0.0f;
    sampler_info.maxLod = 0.0f;

    VK_CHECK(vkCreateSampler(vulkan_state.gpu.logical_device, &sampler_info, nullptr, &vk.texture_image_sampler));
}

internal void
init_vbo(void)
{
    VkDeviceSize buffer_size = sizeof(vertices);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;

    create_buffer(&staging_buffer
		  , &staging_buffer_memory
		  , buffer_size
		  , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		  , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vkMapMemory(vulkan_state.gpu.logical_device
		, staging_buffer_memory
		, 0
		, buffer_size
		, 0
		, &data);
    memcpy(data, vertices, buffer_size);
    vkUnmapMemory(vulkan_state.gpu.logical_device
		  , staging_buffer_memory);

    create_buffer(&vk.vertex_buffer
		  , &vk.vertex_buffer_memory
		  , buffer_size
		  , VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		  , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(&staging_buffer
		, &vk.vertex_buffer
		, buffer_size);

    vkDestroyBuffer(vulkan_state.gpu.logical_device, staging_buffer, nullptr);
    vkFreeMemory(vulkan_state.gpu.logical_device, staging_buffer_memory, nullptr);
}

internal void
init_ibo(void)
{
    VkDeviceSize buffer_size = sizeof(mesh_indices);

    VkBuffer staging_buffer;
    VkDeviceMemory staging_buffer_memory;
    create_buffer(&staging_buffer
		  , &staging_buffer_memory
		  , buffer_size
		  , VK_BUFFER_USAGE_TRANSFER_SRC_BIT
		  , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    void *data;
    vkMapMemory(vulkan_state.gpu.logical_device
		, staging_buffer_memory
		, 0
		, buffer_size
		, 0
		, &data);
    memcpy(data, mesh_indices, buffer_size);
    vkUnmapMemory(vulkan_state.gpu.logical_device, staging_buffer_memory);

    create_buffer(&vk.index_buffer
		  , &vk.index_buffer_memory
		  , buffer_size
		  , VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT
		  , VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    copy_buffer(&staging_buffer
		, &vk.index_buffer
		, buffer_size);

    vkDestroyBuffer(vulkan_state.gpu.logical_device, staging_buffer, nullptr);
    vkFreeMemory(vulkan_state.gpu.logical_device, staging_buffer_memory, nullptr);
}    

struct Uniform_Buffer_Object
{
    alignas(16) glm::mat4 model_matrix;
    alignas(16) glm::mat4 view_matrix;
    alignas(16) glm::mat4 projection_matrix;
};

internal void
init_ubos(void)
{
    VkDeviceSize buffer_size = sizeof(Uniform_Buffer_Object);

    vk.uniform_buffer_count = vulkan_state.swapchain.images.count;

    vk.uniform_buffers = (VkBuffer *)allocate_stack(sizeof(VkBuffer) * vk.uniform_buffer_count
						    , Alignment(1)
						    , "uniform_buffers_list_allocation");
    vk.uniform_buffers_memory = (VkDeviceMemory *)allocate_stack(sizeof(VkDeviceMemory *) * vk.uniform_buffer_count
								 , Alignment(1)
								 , "uniform_buffers_memory_list_allocation");

    for (u32 i = 0
	     ; i < vk.uniform_buffer_count
	     ; ++i)
    {
	create_buffer(&vk.uniform_buffers[i]
		      , &vk.uniform_buffers_memory[i]
		      , buffer_size
		      , VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
		      , VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    }
}

internal void
init_descriptor_pool(void)
{
    VkDescriptorPoolSize pool_sizes[2] = {};

    pool_sizes[0].type			= VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pool_sizes[0].descriptorCount	= vulkan_state.swapchain.images.count;
    
    pool_sizes[1].type			= VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pool_sizes[1].descriptorCount	= vulkan_state.swapchain.images.count;

    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = 2;
    pool_info.pPoolSizes = pool_sizes;

    pool_info.maxSets = vulkan_state.swapchain.images.count;

    VK_CHECK(vkCreateDescriptorPool(vulkan_state.gpu.logical_device, &pool_info, nullptr, &vk.descriptor_pool));
}

internal void
init_descriptor_sets(void)
{
    vk.descriptor_set_count = vulkan_state.swapchain.images.count;

    vk.descriptor_sets = (VkDescriptorSet *)allocate_stack(vk.descriptor_set_count * sizeof(VkDescriptorSet)
							   , Alignment(1)
							   , "descriptor_set_list_allocation");
    VkDescriptorSetLayout *descriptor_set_layouts = (VkDescriptorSetLayout *)allocate_stack(vk.descriptor_set_count * sizeof(VkDescriptorSetLayout)
											    , Alignment(1)
											    , "descriptor_set_layout_list_allocation");
    VkDescriptorSetLayout *main_descriptor_layout = Vulkan_API::get_descriptor_set_layout(rendering_objects.descriptor_set_layout);
    
    for (u32 i = 0
	     ; i < vk.descriptor_set_count
	     ; ++i) descriptor_set_layouts[i] = *main_descriptor_layout;

    VkDescriptorSetAllocateInfo alloc_info	= {};
    alloc_info.sType				= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    alloc_info.descriptorPool			= vk.descriptor_pool;
    alloc_info.descriptorSetCount		= vk.descriptor_set_count;
    alloc_info.pSetLayouts			= descriptor_set_layouts;

    VK_CHECK(vkAllocateDescriptorSets(vulkan_state.gpu.logical_device, &alloc_info, vk.descriptor_sets));
    
    for (u32 i = 0
	     ; i < vk.descriptor_set_count
	     ; ++i)
    {
	VkDescriptorBufferInfo buffer_info = {};
	buffer_info.buffer = vk.uniform_buffers[i];
	buffer_info.offset = 0;
	buffer_info.range = sizeof(Uniform_Buffer_Object);

	VkDescriptorImageInfo image_info	= {};
	image_info.imageLayout			= VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	image_info.imageView			= vk.texture_image_view;
	image_info.sampler			= vk.texture_image_sampler;

	VkWriteDescriptorSet descriptor_writes[2] = {};
	descriptor_writes[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[0].dstSet = vk.descriptor_sets[i];
	descriptor_writes[0].dstBinding = 0;
	descriptor_writes[0].dstArrayElement = 0;
	descriptor_writes[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptor_writes[0].descriptorCount = 1;
	descriptor_writes[0].pBufferInfo = &buffer_info;

	descriptor_writes[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptor_writes[1].dstSet = vk.descriptor_sets[i];
	descriptor_writes[1].dstBinding = 1;
	descriptor_writes[1].dstArrayElement = 0;
	descriptor_writes[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptor_writes[1].descriptorCount = 1;
	descriptor_writes[1].pImageInfo = &image_info;

	vkUpdateDescriptorSets(vulkan_state.gpu.logical_device
			       , 2
			       , descriptor_writes
			       , 0
			       , nullptr);
    }
}

internal void
init_command_buffers(Vulkan_API::Swapchain *swapchain)
{
    VkCommandPool *command_pool = Vulkan_API::get_command_pool(rendering_objects.graphics_command_pool);
    
    vk.command_buffer_count = vulkan_state.swapchain.images.count;

    vk.command_buffers = (VkCommandBuffer *)allocate_stack(sizeof(VkCommandBuffer) * vk.command_buffer_count
							   , Alignment(1)
							   , "command_buffer_list_allocation");

    VkCommandBufferAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.commandPool = *command_pool;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandBufferCount = vk.command_buffer_count;

    VK_CHECK(vkAllocateCommandBuffers(vulkan_state.gpu.logical_device
				      , &alloc_info
				      , vk.command_buffers));

    Vulkan_API::Render_Pass *render_pass = Vulkan_API::get_render_pass(rendering_objects.test_render_pass);

    Vulkan_API::Graphics_Pipeline *pipeline_ptr = Vulkan_API::get_graphics_pipeline(rendering_objects.graphics_pipeline);
    
    for (u32 i = 0
	     ; i < vk.command_buffer_count
	     ; ++i)
    {
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.pNext = nullptr;
	// means that a command buffer can be resubmitted to a queue
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
        begin_info.pInheritanceInfo = nullptr;

	VK_CHECK(vkBeginCommandBuffer(vk.command_buffers[i]
				      , &begin_info));

	VkRenderPassBeginInfo render_pass_info = {};
	render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	render_pass_info.pNext = nullptr;
	render_pass_info.renderPass = render_pass->render_pass;

	Vulkan_API::Framebuffer *fbo = Vulkan_API::get_framebuffer(swapchain->framebuffers.buffer[i]);
	
	render_pass_info.framebuffer = fbo->framebuffer;
	render_pass_info.renderArea.offset = {0, 0};
	render_pass_info.renderArea.extent = vulkan_state.swapchain.extent;

	VkClearValue clear_colors[2];
	clear_colors[0].color = {0.0f, 0.0f, 0.0f, 1.0f};
	clear_colors[1].depthStencil = {1.0f, 0};
	render_pass_info.clearValueCount = 2;
	render_pass_info.pClearValues = clear_colors;

	vkCmdBeginRenderPass(vk.command_buffers[i]
			     , &render_pass_info
			     , VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(vk.command_buffers[i]
			  , VK_PIPELINE_BIND_POINT_GRAPHICS
			  , pipeline_ptr->pipeline);

	VkBuffer vertex_buffers[] = {vk.vertex_buffer};
	VkDeviceSize offset[] = {0};
	vkCmdBindVertexBuffers(vk.command_buffers[i]
			       , 0
			       , 1
			       , vertex_buffers
			       , offset);

	vkCmdBindIndexBuffer(vk.command_buffers[i]
			     , vk.index_buffer
			     , 0
			     , VK_INDEX_TYPE_UINT32);

	vkCmdBindDescriptorSets(vk.command_buffers[i]
				, VK_PIPELINE_BIND_POINT_GRAPHICS
				, pipeline_ptr->layout
				, 0
				, 1
				, &vk.descriptor_sets[i]
				, 0
				, nullptr);

	vkCmdDrawIndexed(vk.command_buffers[i]
			 , sizeof(mesh_indices) / sizeof(u32)
			 , 1
			 , 0
			 , 0
			 , 0);

	vkCmdEndRenderPass(vk.command_buffers[i]);
	VK_CHECK(vkEndCommandBuffer(vk.command_buffers[i]));
    }
}

internal constexpr u32 MAX_FRAMES_IN_FLIGHT = 2;

internal void
init_sync(void)
{
    vk.semaphore_count = MAX_FRAMES_IN_FLIGHT;
    vk.image_ready_semaphores = (VkSemaphore *)allocate_stack(sizeof(VkSemaphore) * vk.semaphore_count
							      , Alignment(1)
							      , "sempahore_image_ready_list_allocation");

    vk.render_finished_semaphores = (VkSemaphore *)allocate_stack(sizeof(VkSemaphore) * vk.semaphore_count
								  , Alignment(1)
								  , "sempahore_image_ready_list_allocation");

    vk.fences = (VkFence *)allocate_stack(sizeof(VkFence) * vk.semaphore_count
					  , Alignment(1)
						  , "fence_list_allocation");

    VkSemaphoreCreateInfo semaphore_info = {};
    semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fence_info = {};
    fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
    
    for (u32 i = 0
	     ; i < MAX_FRAMES_IN_FLIGHT
	     ; ++i)
    {
	VK_CHECK(vkCreateFence(vulkan_state.gpu.logical_device
			       , &fence_info
			       , nullptr
			       , &vk.fences[i]));

	VK_CHECK(vkCreateSemaphore(vulkan_state.gpu.logical_device
				   , &semaphore_info
				   , nullptr
				   , &vk.render_finished_semaphores[i]));

	VK_CHECK(vkCreateSemaphore(vulkan_state.gpu.logical_device
				   , &semaphore_info
				   , nullptr
				   , &vk.image_ready_semaphores[i]));
    }
}

void
init_vk(GLFWwindow *window)
{
    Vulkan_API::init_state(&vulkan_state
			   , window);

    Rendering::init_rendering_state(&vulkan_state
				    , &rendering_objects);
    
    //    init_graphics_pipeline();

    
    //init_command_pool();

    
    //    init_depth_resources();

    
    //    init_framebuffers();


    init_texture_image();
    init_texture_image_view();
    init_texture_image_sampler();

    init_vbo();
    init_ibo();
    init_ubos();

    init_descriptor_pool();
    init_descriptor_sets();


    init_command_buffers(&vulkan_state.swapchain);
    init_sync();
}

internal void
update_ubo(u32 current_image)
{
    persist auto start_time = std::chrono::high_resolution_clock::now();

    auto current_time = std::chrono::high_resolution_clock::now();
    float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();

    Uniform_Buffer_Object ubo = {};

    ubo.model_matrix = glm::rotate(time * glm::radians(90.0f)
				   , glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.view_matrix = glm::lookAt(glm::vec3(2.0f)
				  , glm::vec3(0.0f)
				  , glm::vec3(0.0f, 0.0f, 1.0f));
    ubo.projection_matrix = glm::perspective(glm::radians(60.0f)
					     , (float)vulkan_state.swapchain.extent.width / (float)vulkan_state.swapchain.extent.height
					     , 0.1f
					     , 10.0f);

    ubo.projection_matrix[1][1] *= -1;

    void *data;
    vkMapMemory(vulkan_state.gpu.logical_device, vk.uniform_buffers_memory[current_image], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(vulkan_state.gpu.logical_device, vk.uniform_buffers_memory[current_image]);
}

internal u32 current_frame = 0;

void
draw_frame(void)
{
    vkWaitForFences(vulkan_state.gpu.logical_device
		    , 1
		    , &vk.fences[current_frame]
		    , VK_TRUE
		    , UINT_MAX);

    u32 image_index;

    VkResult result = vkAcquireNextImageKHR(vulkan_state.gpu.logical_device
					    , vulkan_state.swapchain.swapchain
					    , UINT_MAX
					    , vk.image_ready_semaphores[current_frame]
					    , vk.fences[current_frame]
					    , &image_index);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR)
    {
	// recreate swapchain
	return;
    }
    else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    {
	OUTPUT_DEBUG_LOG("%s\n", "failed to acquire swapchain image");
    }

    update_ubo(image_index);

    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore wait_semaphores[] = {vk.image_ready_semaphores[current_frame]};
    VkPipelineStageFlags wait_stages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = wait_semaphores;
    submit_info.pWaitDstStageMask = wait_stages;

    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &vk.command_buffers[image_index];

    VkSemaphore signal_semaphores[] = {vk.render_finished_semaphores[current_frame]};
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = signal_semaphores;

    vkResetFences(vulkan_state.gpu.logical_device, 1, &vk.fences[current_frame]);

    vkQueueSubmit(vulkan_state.gpu.graphics_queue
			   , 1
			   , &submit_info
			   , vk.fences[current_frame]);

    VkPresentInfoKHR present_info = {};
    present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = signal_semaphores;

    VkSwapchainKHR swapchains[] = {vulkan_state.swapchain.swapchain};
    present_info.swapchainCount = 1;
    present_info.pSwapchains = swapchains;
    present_info.pImageIndices = &image_index;

    result = vkQueuePresentKHR(vulkan_state.gpu.present_queue
			       , &present_info);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
    {
	// recreate swapchain
    }
    else if (result != VK_SUCCESS)
    {
	OUTPUT_DEBUG_LOG("%s\n", "failed to present swapchain image");
    }

    current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void
recreate(void)
{
    
}

void
destroy_vk(void)
{
    //    vkDestroyPipeline(vulkan_state.gpu.logical_device, vk.graphics_pipeline, nullptr);
    //vkDestroyPipelineLayout(vulkan_state.gpu.logical_device, vk.pipeline_layout, nullptr);
    Vulkan_API::Render_Pass *render_pass = Vulkan_API::get_render_pass(rendering_objects.test_render_pass);
    vkDestroyRenderPass(vulkan_state.gpu.logical_device, render_pass->render_pass, nullptr);

    for (u32 i = 0
	     ; i < vulkan_state.swapchain.images.count
	     ; ++i)
    {
	// images are destroyed by the vkDestroySwapchainKHR()
	//	vkDestroyImageView(vulkan_state.gpu.logical_device, vulkan_state.swapchain.image_views[i], nullptr);
    }

    vkDestroySwapchainKHR(vulkan_state.gpu.logical_device, vulkan_state.swapchain.swapchain, nullptr);
    vkDestroyDevice(vulkan_state.gpu.logical_device, nullptr);
    vkDestroySurfaceKHR(vulkan_state.instance, vulkan_state.surface, nullptr);
    vkDestroyInstance(vulkan_state.instance, nullptr);
}
