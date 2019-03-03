#define GLFW_INCLUDE_VULKAN

#include <cstring>

#include "vulkan.hpp"
#include <limits.h>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include "vulkan_managers.hpp"

namespace Vulkan_API
{

    namespace Memory
    {

	internal uint32
	find_memory_type_according_to_requirements(GPU *gpu
						   , VkMemoryPropertyFlags properties
						   , VkMemoryRequirements memory_requirements)
	{
	    VkPhysicalDeviceMemoryProperties *gpu_mem_properties = &gpu->memory_properties;

	    for (uint32 i = 0
		     ; i < gpu_mem_properties->memoryTypeCount
		     ; ++i)
	    {
		if (memory_requirements.memoryTypeBits & (1 << i)
		    && (gpu_mem_properties->memoryTypes[i].propertyFlags & properties) == properties)
		{
		    return(i);
		}
	    }
	    
	    OUTPUT_DEBUG_LOG("%s\n", "failed to find suitable memory type");
	    assert(false);
	    return(0);
	}
	
	void
	allocate_gpu_memory(VkMemoryPropertyFlags properties
			    , VkMemoryRequirements memory_requirements
			    , GPU *gpu
			    , VkDeviceMemory *dest_memory)
	{
	    VkMemoryAllocateInfo alloc_info = {};
	    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	    alloc_info.allocationSize = memory_requirements.size;
	    alloc_info.memoryTypeIndex = find_memory_type_according_to_requirements(gpu
										    , properties
										    , memory_requirements);

	    VK_CHECK(vkAllocateMemory(gpu->logical_device
				      , &alloc_info
				      , nullptr
				      , dest_memory));
	}
	
    }

    void
    GPU::find_queue_families(VkSurfaceKHR *surface)
    {
	uint32 queue_family_count = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(hardware
						 , &queue_family_count
						 , nullptr);

	VkQueueFamilyProperties *queue_properties = (VkQueueFamilyProperties *)allocate_stack(sizeof(VkQueueFamilyProperties) * queue_family_count
											      , Alignment(1)
											      , "queue_family_list_allocation");
	vkGetPhysicalDeviceQueueFamilyProperties(hardware
						 , &queue_family_count
						 , queue_properties);

	for (uint32 i = 0
		 ; i < queue_family_count
		 ; ++i)
	{
	    if (queue_properties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && queue_properties[i].queueCount > 0)
	    {
		queue_families.graphics_family = i;
	    }

	    VkBool32 present_support = false;
	    vkGetPhysicalDeviceSurfaceSupportKHR(hardware
						 , i
						 , *surface
						 , &present_support);
	
	    if (queue_properties[i].queueCount > 0 && present_support)
	    {
		queue_families.present_family = i;
	    }
	
	    if (queue_families.complete())
	    {
		break;
	    }
	}

	pop_stack();
    }

    struct Instance_Create_Validation_Layer_Params
    {
	bool r_enable;
	uint32 o_layer_count;
	const char **o_layer_names;
    };

    struct Instance_Create_Extension_Params
    {
	uint32 r_extension_count;
	const char **r_extension_names;
    };

    // TODO(luc) : make validation layers truly optional, enable / disable when requested
    internal void
    init_instance(VkInstance *instance
		  , VkApplicationInfo *app_info
		  , Instance_Create_Validation_Layer_Params *validation_params
		  , Instance_Create_Extension_Params *extension_params)
    {
	VkInstanceCreateInfo instance_info = {};
	instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	instance_info.pApplicationInfo = app_info;

	uint32 layer_count;
	vkEnumerateInstanceLayerProperties(&layer_count
					   , nullptr);

	VkLayerProperties *properties = (VkLayerProperties *)allocate_stack(sizeof(VkLayerProperties) * layer_count
									    , Alignment(1)
									    , "validation_layer_list_allocation");
	vkEnumerateInstanceLayerProperties(&layer_count
					   , properties);

	for (uint32 r = 0; r < validation_params->o_layer_count; ++r)
	{
	    bool found_layer = false;
	    for (uint32 l = 0; l < layer_count; ++l)
	    {
		if (!strcmp(properties[l].layerName, validation_params->o_layer_names[r])) found_layer = true;
	    }

	    if (!found_layer) assert(false);
	}

	// if found then add to the instance information
	instance_info.enabledLayerCount = validation_params->o_layer_count;
	instance_info.ppEnabledLayerNames = validation_params->o_layer_names;

	// get extensions needed

	instance_info.enabledExtensionCount = extension_params->r_extension_count;
	instance_info.ppEnabledExtensionNames = extension_params->r_extension_names;

	VK_CHECK(vkCreateInstance(&instance_info
				  , nullptr
				  , instance)
		 , "failed to create instance\n");

	pop_stack();
    }

    internal VKAPI_ATTR VkBool32 VKAPI_CALL
    vulkan_debug_proc(VkDebugUtilsMessageSeverityFlagBitsEXT message_severity
		      , VkDebugUtilsMessageTypeFlagsEXT message_type
		      , const VkDebugUtilsMessengerCallbackDataEXT *message_data
		      , void *user_data)
    {
	OUTPUT_DEBUG_LOG("validation layer - %s\n", message_data->pMessage);

	return(VK_FALSE);
    }

    internal void
    init_debug_messenger(VkInstance *instance
			 , VkDebugUtilsMessengerEXT *messenger)
    {
	// setup debugger
	VkDebugUtilsMessengerCreateInfoEXT debug_info = {};
	debug_info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	debug_info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
	    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT
	    | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	debug_info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT
	    | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
	    | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	debug_info.pfnUserCallback = vulkan_debug_proc;
	debug_info.pUserData = nullptr;

	PFN_vkCreateDebugUtilsMessengerEXT vk_create_debug_utils_messenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(*instance, "vkCreateDebugUtilsMessengerEXT");
	assert(vk_create_debug_utils_messenger != nullptr);
	VK_CHECK(vk_create_debug_utils_messenger(*instance, &debug_info, nullptr, messenger));
    }

    internal void
    get_swapchain_support(VkSurfaceKHR *surface
			  , GPU *gpu)
    {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->hardware, *surface, &gpu->swapchain_support.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->hardware, *surface, &gpu->swapchain_support.available_formats_count, nullptr);

	if (gpu->swapchain_support.available_formats_count != 0)
	{
	    gpu->swapchain_support.available_formats = (VkSurfaceFormatKHR *)allocate_stack(sizeof(VkSurfaceFormatKHR) * gpu->swapchain_support.available_formats_count
											    , Alignment(1)
											    , "surface_format_list_allocation");
	    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->hardware
						 , *surface
						 , &gpu->swapchain_support.available_formats_count
						 , gpu->swapchain_support.available_formats);
	}

	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu->hardware, *surface, &gpu->swapchain_support.available_present_modes_count, nullptr);
	if (gpu->swapchain_support.available_present_modes_count != 0)
	{
	    gpu->swapchain_support.available_present_modes = (VkPresentModeKHR *)allocate_stack(sizeof(VkPresentModeKHR) * gpu->swapchain_support.available_present_modes_count
												, Alignment(1)
												, "surface_present_mode_list_allocation");
	    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu->hardware
						      , *surface
						      , &gpu->swapchain_support.available_present_modes_count
						      , gpu->swapchain_support.available_present_modes);
	}
    }
    
    struct Physical_Device_Extensions_Params
    {
	uint32 r_extension_count;
	const char **r_extension_names;
    };

    internal bool
    check_if_physical_device_supports_extensions(Physical_Device_Extensions_Params *extension_params
						 , VkPhysicalDevice gpu)
    {
	uint32 extension_count;
	vkEnumerateDeviceExtensionProperties(gpu
					     , nullptr
					     , &extension_count
					     , nullptr);

	VkExtensionProperties *extension_properties = (VkExtensionProperties *)allocate_stack(sizeof(VkExtensionProperties) * extension_count
											      , Alignment(1)
											      , "gpu_extension_properties_list_allocation");
	vkEnumerateDeviceExtensionProperties(gpu
					     , nullptr
					     , &extension_count
					     , extension_properties);
    
	uint32 required_extensions_left = extension_params->r_extension_count;
	for (uint32 i = 0
		 ; i < extension_count && required_extensions_left > 0
		 ; ++i)
	{
	    for (uint32 j = 0
		     ; j < extension_params->r_extension_count
		     ; ++j)
	    {
		if (!strcmp(extension_properties[i].extensionName, extension_params->r_extension_names[j]))
		{
		    --required_extensions_left;
		}
	    }
	}
	pop_stack();

	return(!required_extensions_left);
    }
    
    internal bool
    check_if_physical_device_is_suitable(Physical_Device_Extensions_Params *extension_params
					 , VkSurfaceKHR *surface
					 , GPU *gpu)
    {
	gpu->find_queue_families(surface);

	VkPhysicalDeviceProperties device_properties;
	vkGetPhysicalDeviceProperties(gpu->hardware
				      , &device_properties);
    
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceFeatures(gpu->hardware
				    , &device_features);

	bool swapchain_supported = check_if_physical_device_supports_extensions(extension_params
										, gpu->hardware);

	bool swapchain_usable = false;
	if (swapchain_supported)
	{
	    get_swapchain_support(surface, gpu);
	    swapchain_usable = gpu->swapchain_support.available_formats_count && gpu->swapchain_support.available_present_modes_count;
	}

	return(swapchain_supported && swapchain_usable
	       && (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	       && gpu->queue_families.complete()
	       && device_features.geometryShader
	       && device_features.samplerAnisotropy);
    }

    internal void
    choose_gpu(Physical_Device_Extensions_Params *extension_params
	       , VkSurfaceKHR *surface
	       , VkInstance *instance
	       , GPU *gpu_result)
    {
	uint32 device_count = 0;
	vkEnumeratePhysicalDevices(*instance
				   , &device_count
				   , nullptr);
    
	VkPhysicalDevice *devices = (VkPhysicalDevice *)allocate_stack(sizeof(VkPhysicalDevice) * device_count
								       , Alignment(1)
								       , "physical_device_list_allocation");
	vkEnumeratePhysicalDevices(*instance
				   , &device_count
				   , devices);

	OUTPUT_DEBUG_LOG("available physical hardware devices count : %d\n", device_count);

	for (uint32 i = 0
		 ; i < device_count
		 ; ++i)
	{
	    GPU gpu;
	    gpu.hardware = devices[i];
	
	    // check if device is suitable
	    if (check_if_physical_device_is_suitable(extension_params
						     , surface
						     , &gpu))
	    {
		*gpu_result = gpu;
		break;
	    }
	}

	assert(gpu_result->hardware != VK_NULL_HANDLE);
	OUTPUT_DEBUG_LOG("%s\n", "found gpu compatible with application");
    }

    internal void
    init_device(Physical_Device_Extensions_Params *gpu_extensions
		, Instance_Create_Validation_Layer_Params *validation_layers
		, GPU *gpu)
    {
	// create the logical device
	Queue_Families *indices = &gpu->queue_families;

	Bitset_32 bitset;
	bitset.set1(indices->graphics_family);
	bitset.set1(indices->present_family);

	uint32 unique_sets = bitset.pop_count();

	uint32 *unique_family_indices = (uint32 *)allocate_stack(sizeof(uint32) * unique_sets
								 , Alignment(1)
								 , "unique_queue_family_indices_allocation");
	VkDeviceQueueCreateInfo *unique_queue_infos = (VkDeviceQueueCreateInfo *)allocate_stack(sizeof(VkDeviceCreateInfo) * unique_sets
												, Alignment(1)
												, "unique_queue_list_allocation");

	// fill the unique_family_indices with the indices
	for (uint32 b = 0, set_bit = 0
		 ; b < 32 && set_bit < unique_sets
		 ; ++b)
	{
	    if (bitset.get(b))
	    {
		unique_family_indices[set_bit++] = b;
	    }
	}
    
	float32 priority1 = 1.0f;
	for (uint32 i = 0
		 ; i < unique_sets
		 ; ++i)
	{
	    VkDeviceQueueCreateInfo queue_info = {};
	    queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	    queue_info.queueFamilyIndex = unique_family_indices[i];
	    queue_info.queueCount = 1;
	    queue_info.pQueuePriorities = &priority1;
	    unique_queue_infos[i] = queue_info;
	}

	VkPhysicalDeviceFeatures device_features = {};
	device_features.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo device_info = {};
	device_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	device_info.pQueueCreateInfos = unique_queue_infos;
	device_info.queueCreateInfoCount = unique_sets;
	device_info.pEnabledFeatures = &device_features;
	device_info.enabledExtensionCount = gpu_extensions->r_extension_count;
	device_info.ppEnabledExtensionNames = gpu_extensions->r_extension_names;
	device_info.ppEnabledLayerNames = validation_layers->o_layer_names;
	device_info.enabledLayerCount = validation_layers->o_layer_count;

	VK_CHECK(vkCreateDevice(gpu->hardware
				, &device_info
				, nullptr
				, &gpu->logical_device));
	pop_stack();
	pop_stack();

	vkGetDeviceQueue(gpu->logical_device, gpu->queue_families.graphics_family, 0, &gpu->graphics_queue);
	vkGetDeviceQueue(gpu->logical_device, gpu->queue_families.present_family, 0, &gpu->present_queue);
    }

    internal VkSurfaceFormatKHR
    choose_surface_format(VkSurfaceFormatKHR *available_formats
			  , uint32 format_count)
    {
	if (format_count == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
	{
	    VkSurfaceFormatKHR format;
	    format.format		= VK_FORMAT_B8G8R8A8_UNORM;
	    format.colorSpace	= VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
	}
	for (uint32 i = 0
		 ; i < format_count
		 ; ++i)
	{
	    if (available_formats[i].format == VK_FORMAT_B8G8R8A8_UNORM && available_formats[i].colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
	    {
		return(available_formats[i]);
	    }
	}

	return(available_formats[0]);
    }

    internal VkPresentModeKHR
    choose_surface_present_mode(const VkPresentModeKHR *available_present_modes
				, uint32 present_modes_count)
    {
	// supported by most hardware
	VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;
	for (uint32 i = 0
		 ; i < present_modes_count
		 ; ++i)
	{
	    if (available_present_modes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
	    {
		return(available_present_modes[i]);
	    }
	    else if (available_present_modes[i] == VK_PRESENT_MODE_IMMEDIATE_KHR)
	    {
		best_mode = available_present_modes[i];
	    }
	}
	return(best_mode);
    }

    internal VkExtent2D
    choose_swapchain_extent(GLFWwindow *window
			    , const VkSurfaceCapabilitiesKHR *capabilities)
    {
	if (capabilities->currentExtent.width != UINT_MAX)
	{
	    return(capabilities->currentExtent);
	}
	else
	{
	    int32 width, height;
	    glfwGetFramebufferSize(window, &width, &height);

	    VkExtent2D actual_extent	= { (uint32)width, (uint32)height };
	    actual_extent.width		= MAX(capabilities->minImageExtent.width, MIN(capabilities->maxImageExtent.width, actual_extent.width));
	    actual_extent.height	= MAX(capabilities->minImageExtent.height, MIN(capabilities->maxImageExtent.height, actual_extent.height));

	    return(actual_extent);
	}
    }

    void
    init_image(uint32 width
	       , uint32 height
	       , VkFormat format
	       , VkImageTiling tiling
	       , VkImageUsageFlags usage
	       , VkMemoryPropertyFlags properties
	       , GPU *gpu
	       , VkImage *dest_image)
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

	VK_CHECK(vkCreateImage(gpu->logical_device, &image_info, nullptr, dest_image));
    }
    
    void
    init_image_view(VkImage *image
		    , VkFormat format
		    , VkImageAspectFlags aspect_flags
		    , GPU *gpu
		    , VkImageView *dest_image_view)
    {
	VkImageViewCreateInfo view_info			= {};
	view_info.sType					= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	view_info.image					= *image;
	view_info.viewType				= VK_IMAGE_VIEW_TYPE_2D;
	view_info.format				= format;
	view_info.subresourceRange.aspectMask		= aspect_flags;
	view_info.subresourceRange.baseMipLevel		= 0;
	view_info.subresourceRange.levelCount		= 1;
	view_info.subresourceRange.baseArrayLayer	= 0;
	view_info.subresourceRange.layerCount		= 1;

	VK_CHECK(vkCreateImageView(gpu->logical_device, &view_info, nullptr, dest_image_view));
    }

    void
    init_image_sampler(VkFilter mag_filter
		       , VkFilter min_filter
		       , VkSamplerAddressMode u_sampler_address_mode
		       , VkSamplerAddressMode v_sampler_address_mode
		       , VkSamplerAddressMode w_sampler_address_mode
		       , VkBool32 anisotropy_enable
		       , uint32 max_anisotropy
		       , VkBorderColor clamp_border_color
		       , VkBool32 compare_enable
		       , VkCompareOp compare_op
		       , VkSamplerMipmapMode mipmap_mode
		       , float32 mip_lod_bias
		       , float32 min_lod
		       , float32 max_lod
		       , GPU *gpu
		       , VkSampler *dest_sampler)
    {
	VkSamplerCreateInfo sampler_info = {};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = mag_filter;
	sampler_info.minFilter = min_filter;
	sampler_info.addressModeU = u_sampler_address_mode;
	sampler_info.addressModeV = v_sampler_address_mode;
	sampler_info.addressModeW = w_sampler_address_mode;
	sampler_info.anisotropyEnable = anisotropy_enable;
	sampler_info.maxAnisotropy = max_anisotropy;
	sampler_info.borderColor = clamp_border_color; // when clamping
	sampler_info.compareEnable = compare_enable;
	sampler_info.compareOp = compare_op;
	sampler_info.mipmapMode = mipmap_mode;
	sampler_info.mipLodBias = mip_lod_bias;
	sampler_info.minLod = min_lod;
	sampler_info.maxLod = max_lod;

	VK_CHECK(vkCreateSampler(gpu->logical_device, &sampler_info, nullptr, dest_sampler));
    }

    void
    begin_command_buffer(VkCommandBuffer *command_buffer
			 , VkCommandBufferUsageFlags usage_flags
			 , VkCommandBufferInheritanceInfo *inheritance)
    {
	VkCommandBufferBeginInfo begin_info = {};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = usage_flags;
	begin_info.pInheritanceInfo = inheritance;

	VK_CHECK(vkBeginCommandBuffer(*command_buffer
				      , &begin_info));
    }

    void
    allocate_command_buffers(VkCommandPool *command_pool_source
			     , VkCommandBufferLevel level
			     , GPU *gpu
			     , const Memory_Buffer_View<VkCommandBuffer> &command_buffers)
    {
	VkCommandBufferAllocateInfo allocate_info = {};
	allocate_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocate_info.level = level;
	allocate_info.commandPool = *command_pool_source;
	allocate_info.commandBufferCount = command_buffers.count;

	vkAllocateCommandBuffers(gpu->logical_device
				 , &allocate_info
				 , command_buffers.buffer);
    }
    
    void
    submit(const Memory_Buffer_View<VkCommandBuffer> &command_buffers
	   , VkQueue *queue)
    {
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = command_buffers.count;
	submit_info.pCommandBuffers = command_buffers.buffer;

	vkQueueSubmit(*queue, 1, &submit_info, VK_NULL_HANDLE);
    }

    internal bool
    has_stencil_component(VkFormat format)
    {
	return(format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT);
    }
    
    void
    transition_image_layout(VkImage *image
			    , VkFormat format
			    , VkImageLayout old_layout
			    , VkImageLayout new_layout
			    , VkCommandPool *graphics_command_pool
			    , GPU *gpu)
    {
	VkCommandBuffer single_command;
	allocate_command_buffers(graphics_command_pool
				 , VK_COMMAND_BUFFER_LEVEL_PRIMARY
				 , gpu
				 , Memory_Buffer_View<VkCommandBuffer>{1, &single_command});
	
	begin_command_buffer(&single_command, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT, nullptr);
	
	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;

	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
   
	barrier.image				= *image;
	barrier.subresourceRange.aspectMask	= VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseMipLevel	= 0;
	barrier.subresourceRange.levelCount	= 1;
	barrier.subresourceRange.baseArrayLayer	= 0;
	barrier.subresourceRange.layerCount	= 1;

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

	vkCmdPipelineBarrier(single_command
			     , source_stage
			     , destination_stage
			     , 0
			     , 0
			     , nullptr
			     , 0
			     , nullptr
			     , 1
			     , &barrier);

	end_command_buffer(&single_command);
	submit(Memory_Buffer_View<VkCommandBuffer>{1, &single_command}, &gpu->graphics_queue);
	free_command_buffer(Memory_Buffer_View<VkCommandBuffer>{1, &single_command}, graphics_command_pool, gpu);
    }

    internal void
    init_swapchain(GLFWwindow *window
		   , VkSurfaceKHR *surface
		   , GPU *gpu
		   , Swapchain *swapchain)
    {
	Swapchain_Details *swapchain_details = &gpu->swapchain_support;
	VkSurfaceFormatKHR surface_format = choose_surface_format(swapchain_details->available_formats, swapchain_details->available_formats_count);
	VkExtent2D surface_extent = choose_swapchain_extent(window, &swapchain_details->capabilities);
	VkPresentModeKHR present_mode = choose_surface_present_mode(swapchain_details->available_present_modes, swapchain_details->available_present_modes_count);

	// add 1 to the minimum images supported in the swapchain
	uint32 image_count = swapchain_details->capabilities.minImageCount + 1;
	if (image_count > swapchain_details->capabilities.maxImageCount)
	{
	    image_count = swapchain_details->capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR swapchain_info = {};
	swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_info.surface = *surface;
	swapchain_info.minImageCount = image_count;
	swapchain_info.imageFormat = surface_format.format;
	swapchain_info.imageColorSpace = surface_format.colorSpace;
	swapchain_info.imageExtent = surface_extent;
	swapchain_info.imageArrayLayers = 1;
	swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

	uint32 queue_family_indices[] = { (uint32)gpu->queue_families.graphics_family, (uint32)gpu->queue_families.present_family };

	if (gpu->queue_families.graphics_family != gpu->queue_families.present_family)
	{
	    swapchain_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
	    swapchain_info.queueFamilyIndexCount = 2;
	    swapchain_info.pQueueFamilyIndices = queue_family_indices;
	}
	else
	{
	    swapchain_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	    swapchain_info.queueFamilyIndexCount = 0;
	    swapchain_info.pQueueFamilyIndices = nullptr;
	}

	swapchain_info.preTransform = swapchain_details->capabilities.currentTransform;
	swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	swapchain_info.presentMode = present_mode;
	swapchain_info.clipped = VK_TRUE;
	swapchain_info.oldSwapchain = VK_NULL_HANDLE;

	VK_CHECK(vkCreateSwapchainKHR(gpu->logical_device, &swapchain_info, nullptr, &swapchain->swapchain));

	vkGetSwapchainImagesKHR(gpu->logical_device, swapchain->swapchain, &image_count, nullptr);
	VkImage *images_array = (VkImage *)allocate_stack(sizeof(VkImage) * image_count
						    , Alignment(1)
						    , "swapchain_images_list_allocation");
	vkGetSwapchainImagesKHR(gpu->logical_device, swapchain->swapchain, &image_count, images_array);
	swapchain->images.count = image_count;
	swapchain->images.buffer = (Image2D_Handle *)allocate_free_list(sizeof(Image2D_Handle) * swapchain->images.count
									, 1
									, "allocation.swapchain_image_handles_allocation");

	char image2D_hash_name[] = "image2D.swapchain_image0";
	enum { NUMBER_STRING_INDEX = 23 };
	for (uint32 i = 0
		 ; i < image_count
		 ; ++i)
	{
	    image2D_hash_name[NUMBER_STRING_INDEX] = '0' + i;
	    auto hash = compile_hash(image2D_hash_name, sizeof(image2D_hash_name));
	    swapchain->images.buffer[i] = add_image2D(Constant_String{image2D_hash_name, sizeof(image2D_hash_name), hash});
	}
	
	swapchain->extent = surface_extent;
	swapchain->format = surface_format.format;
	swapchain->present_mode = present_mode;

	for (uint32 i = 0
		 ; i < image_count
		 ; ++i)
	{
	    Image2D *image = get_image2D(swapchain->images.buffer[i]);
	    image->image = images_array[i];
	    init_image_view(&image->image
			    , swapchain->format
			    , VK_IMAGE_ASPECT_COLOR_BIT
			    , gpu
			    , &image->image_view);
	}
    }
    
    void
    init_render_pass(Memory_Buffer_View<VkAttachmentDescription> *attachment_descriptions
		     , Memory_Buffer_View<VkSubpassDescription> *subpass_descriptions
		     , Memory_Buffer_View<VkSubpassDependency> *subpass_dependencies
		     , GPU *gpu
		     , Render_Pass *dest_render_pass)
    {
	VkRenderPassCreateInfo render_pass_info	= {};
	render_pass_info.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	render_pass_info.attachmentCount	= attachment_descriptions->count;
	render_pass_info.pAttachments		= attachment_descriptions->buffer;
	render_pass_info.subpassCount		= subpass_descriptions->count;
	render_pass_info.pSubpasses		= subpass_descriptions->buffer;
	render_pass_info.dependencyCount	= subpass_dependencies->count;
	render_pass_info.pDependencies		= subpass_dependencies->buffer;

	VK_CHECK(vkCreateRenderPass(gpu->logical_device, &render_pass_info, nullptr, &dest_render_pass->render_pass));
	dest_render_pass->subpass_count = subpass_descriptions->count;
    }

    // find gpu supported depth format
    internal VkFormat
    find_supported_format(const VkFormat *candidates
			  , uint32 candidate_size
			  , VkImageTiling tiling
			  , VkFormatFeatureFlags features
			  , GPU *gpu)
    {
	for (uint32 i = 0
		 ; i < candidate_size
		 ; ++i)
	{
	    VkFormatProperties properties;
	    vkGetPhysicalDeviceFormatProperties(gpu->hardware, candidates[i], &properties);
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

    internal void
    find_depth_format(GPU *gpu)
    {
	VkFormat formats[] = 
	    {
		VK_FORMAT_D32_SFLOAT
		, VK_FORMAT_D32_SFLOAT_S8_UINT
		, VK_FORMAT_D24_UNORM_S8_UINT
	    };
    
	gpu->supported_depth_format	= find_supported_format(formats
								, 3
								, VK_IMAGE_TILING_OPTIMAL
								, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
								, gpu);
    }
    
    void
    init_shader(VkShaderStageFlagBits stage_bits
		, uint32 content_size
		, byte *file_contents
		, GPU *gpu
		, VkShaderModule *dest_shader_module)
    {
	VkShaderModuleCreateInfo shader_info = {};
	shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	shader_info.codeSize = content_size;
	shader_info.pCode = reinterpret_cast<const uint32 *>(file_contents);

	VK_CHECK(vkCreateShaderModule(gpu->logical_device
				      , &shader_info
				      , nullptr
				      , dest_shader_module));
    }

    void
    init_pipeline_layout(Memory_Buffer_View<VkDescriptorSetLayout> *layouts
			 , Memory_Buffer_View<VkPushConstantRange> *ranges
			 , GPU *gpu
			 , VkPipelineLayout *pipeline_layout)
    {
	VkPipelineLayoutCreateInfo layout_info = {};
	layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	layout_info.setLayoutCount = layouts->count;
	layout_info.pSetLayouts = layouts->buffer;
	layout_info.pushConstantRangeCount = ranges->count;
	layout_info.pPushConstantRanges = ranges->buffer;

	VK_CHECK(vkCreatePipelineLayout(gpu->logical_device
					, &layout_info
					, nullptr
					, pipeline_layout));
    }

    void
    init_graphics_pipeline(Memory_Buffer_View<VkPipelineShaderStageCreateInfo> *shaders
			   , VkPipelineVertexInputStateCreateInfo *vertex_input_info
			   , VkPipelineInputAssemblyStateCreateInfo *input_assembly_info
			   , VkPipelineViewportStateCreateInfo *viewport_info
			   , VkPipelineRasterizationStateCreateInfo *rasterization_info
			   , VkPipelineMultisampleStateCreateInfo *multisample_info
			   , VkPipelineColorBlendStateCreateInfo *blend_info
			   , VkPipelineDynamicStateCreateInfo *dynamic_state_info
			   , VkPipelineDepthStencilStateCreateInfo *depth_stencil_info
			   , VkPipelineLayout *pipeline_layout
			   , Render_Pass *render_pass
			   , uint32 subpass
			   , GPU *gpu
			   , VkPipeline *pipeline)
    {
	VkGraphicsPipelineCreateInfo pipeline_info = {};
	pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeline_info.stageCount = shaders->count;
	pipeline_info.pStages = shaders->buffer;
	pipeline_info.pVertexInputState = vertex_input_info;
	pipeline_info.pInputAssemblyState = input_assembly_info;
	pipeline_info.pViewportState = viewport_info;
	pipeline_info.pRasterizationState = rasterization_info;
	pipeline_info.pMultisampleState = multisample_info;
	pipeline_info.pDepthStencilState = depth_stencil_info;
	pipeline_info.pColorBlendState = blend_info;
	pipeline_info.pDynamicState = dynamic_state_info;

	pipeline_info.layout = *pipeline_layout;
	pipeline_info.renderPass = render_pass->render_pass;
	pipeline_info.subpass = subpass;

	pipeline_info.basePipelineHandle = VK_NULL_HANDLE;
	pipeline_info.basePipelineIndex = -1;

	VK_CHECK (vkCreateGraphicsPipelines(gpu->logical_device
					    , VK_NULL_HANDLE
					    , 1
					    , &pipeline_info
					    , nullptr
					    , pipeline) != VK_SUCCESS);
    }

    void
    allocate_command_pool(uint32 queue_family_index
			  , GPU *gpu
			  , VkCommandPool *command_pool)
    {
	VkCommandPoolCreateInfo pool_info = {};
	pool_info.sType			= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	pool_info.queueFamilyIndex	= queue_family_index;
	pool_info.flags			= 0;

	VK_CHECK(vkCreateCommandPool(gpu->logical_device, &pool_info, nullptr, command_pool));
    }

    void
    init_framebuffer(Render_Pass *compatible_render_pass
		     , uint32 width
		     , uint32 height
		     , GPU *gpu
		     , Framebuffer *framebuffer)
    {
	Memory_Buffer_View<VkImageView> image_view_attachments;
	
	image_view_attachments.count = framebuffer->color_attachments.count;
	image_view_attachments.buffer = (VkImageView *)allocate_stack(sizeof(VkImageView) * image_view_attachments.count);
	for (uint32 i = 0
		 ; i < image_view_attachments.count
		 ; ++i)
	{
	    VkImageView *image = &get_image2D(framebuffer->color_attachments.buffer[i])->image_view;
	    image_view_attachments.buffer[i] = *image;
	}

	if (framebuffer->depth_attachment != UNINITIALIZED_HANDLE)
	{
	    VkImageView *depth_image = &get_image2D(framebuffer->depth_attachment)->image_view;
	    extend_stack_top(sizeof(VkImageView));
	    image_view_attachments.buffer[image_view_attachments.count++] = *depth_image;
	}
	
	VkFramebufferCreateInfo fbo_info	= {};
	fbo_info.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbo_info.renderPass			= compatible_render_pass->render_pass;
	fbo_info.attachmentCount		= image_view_attachments.count;
	fbo_info.pAttachments			= image_view_attachments.buffer;
	fbo_info.width				= width;
	fbo_info.height				= height;
	// for the moment, framebuffer layer count is 1
	fbo_info.layers				= 1;

	VK_CHECK(vkCreateFramebuffer(gpu->logical_device, &fbo_info, nullptr, &framebuffer->framebuffer));
    }
    
    void
    init_state(State *state
	       , GLFWwindow *window)
    {
	// initialize instance
	persist constexpr uint32 layer_count = 1;
	const char *layer_names[layer_count] = { "VK_LAYER_LUNARG_standard_validation" };

	Instance_Create_Validation_Layer_Params validation_params = {};
	validation_params.r_enable = true;
	validation_params.o_layer_count = layer_count;
	validation_params.o_layer_names = layer_names;

	uint32 extension_count;
	const char **extension_names = glfwGetRequiredInstanceExtensions(&extension_count);
	const char **total_extension_buffer = (const char **)allocate_stack(sizeof(const char *) * (extension_count + 1)
									    , Alignment(1)
									    , "vulkan_instanc_extension_names_list_allocation");
	memcpy(total_extension_buffer, extension_names, sizeof(const char *) * extension_count);
	total_extension_buffer[extension_count++] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;
	
	Instance_Create_Extension_Params extension_params = {};
	extension_params.r_extension_count = extension_count;
	extension_params.r_extension_names = total_extension_buffer;

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan Engine";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "No Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_API_VERSION_1_0;
	
 	init_instance(&state->instance
		      , &app_info
		      , &validation_params
		      , &extension_params);
	
	pop_stack();

	init_debug_messenger(&state->instance
			     , &state->debug_messenger);

	// create the surface
	VK_CHECK(glfwCreateWindowSurface(state->instance
					 , window
					 , nullptr
					 , &state->surface));

	// choose hardware and create device
	persist constexpr uint32 gpu_extension_count = 1;
	const char *gpu_extension_names[gpu_extension_count] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
	Physical_Device_Extensions_Params gpu_extensions = {};
	gpu_extensions.r_extension_count = gpu_extension_count;
	gpu_extensions.r_extension_names = gpu_extension_names;
	choose_gpu(&gpu_extensions	// function initializes the queue families in the GPU struct
		   , &state->surface
		   , &state->instance
		   , &state->gpu);
	vkGetPhysicalDeviceMemoryProperties(state->gpu.hardware, &state->gpu.memory_properties);
	find_depth_format(&state->gpu);
	init_device(&gpu_extensions
		    , &validation_params
		    , &state->gpu);

	// create swapchain
	init_swapchain(window
		       , &state->surface
		       , &state->gpu
		       , &state->swapchain);
    }

}
