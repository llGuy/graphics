#define GLFW_INCLUDE_VULKAN

#include <cstring>
#include <cassert>
#include "core.hpp"
#include <limits.h>
#include "graphics.hpp"
#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

extern_impl Vulkan_State vk = {};

void
Vulkan_GPU::find_families(void)
{
    uint32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(hardware
					     , &queue_family_count
					     , nullptr);

    VkQueueFamilyProperties *queue_properties = (VkQueueFamilyProperties *)allocate_stack(sizeof(VkQueueFamilyProperties) * queue_family_count
											  , 1
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
					     , vk.surface
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

global constexpr uint32 required_device_extension_count = 1;
global const char *required_physical_device_extensions[required_device_extension_count]
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

internal bool
check_if_physical_device_supports_extensions(VkPhysicalDevice gpu)
{
    uint32 extension_count;
    vkEnumerateDeviceExtensionProperties(gpu
					 , nullptr
					 , &extension_count
					 , nullptr);

    VkExtensionProperties *extension_properties = (VkExtensionProperties *)allocate_stack(sizeof(VkExtensionProperties) * extension_count
											  , 1
											  , "gpu_extension_properties_list_allocation");
    vkEnumerateDeviceExtensionProperties(gpu
					 , nullptr
					 , &extension_count
					 , extension_properties);
    
    uint32 required_extensions_left = required_device_extension_count;
    for (uint32 i = 0
	     ; i < extension_count && required_extensions_left > 0
	     ; ++i)
    {
	for (uint32 j = 0
		 ; j < required_device_extension_count
		 ; ++j)
	{
	    if (!strcmp(extension_properties[i].extensionName, required_physical_device_extensions[j]))
	    {
		--required_extensions_left;
	    }
	}
    }
    pop_stack();

    return(!required_extensions_left);
}

struct Swapchain_Details
{
    VkSurfaceCapabilitiesKHR capabilities;
    VkSurfaceFormatKHR *formats;
    uint32 format_count;
    VkPresentModeKHR *present_modes;
    uint32 mode_count;
};

internal Swapchain_Details
get_swapchain_support(VkPhysicalDevice gpu)
{
    Swapchain_Details details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, vk.surface, &details.capabilities);

    uint32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(gpu
					 , vk.surface
					 , &format_count
					 , nullptr);
    if (format_count != 0)
    {
	details.formats = (VkSurfaceFormatKHR *)allocate_stack(sizeof(VkSurfaceFormatKHR) * format_count
							       , 1
							       , "surface_format_list_allocation");
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu
					     , vk.surface
					     , &format_count
					     , details.formats);
	details.format_count = format_count;
    }

    uint32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(gpu
					      , vk.surface
					      , &present_mode_count
					      , nullptr);
    if (present_mode_count != 0)
    {
	details.present_modes = (VkPresentModeKHR *)allocate_stack(sizeof(VkPresentModeKHR) * present_mode_count
								   , 1
								   , "surface_present_mode_list_allocation");
	vkGetPhysicalDeviceSurfacePresentModesKHR(gpu
						  , vk.surface
						  , &present_mode_count
						  , details.present_modes);
	details.mode_count = present_mode_count;
    }

    return(details);
}

internal bool
check_if_physical_device_is_suitable(Vulkan_GPU *gpu)
{
    gpu->find_families();

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(gpu->hardware
				  , &device_properties);
    
    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(gpu->hardware
				, &device_features);

    bool swapchain_supported = check_if_physical_device_supports_extensions(gpu->hardware);

    bool swapchain_usable = false;
    if (swapchain_supported)
    {
	Swapchain_Details details = get_swapchain_support(gpu->hardware);
	swapchain_usable = details.format_count && details.mode_count;

	pop_stack();
	pop_stack();
    }

    return(swapchain_supported && swapchain_usable
	   && (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	   && gpu->queue_families.complete()
	   && device_features.geometryShader
	   && device_features.samplerAnisotropy);
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

internal VkSurfaceFormatKHR
choose_surface_format(const VkSurfaceFormatKHR *available_formats
		      , uint32 format_count)
{
    if (format_count == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
    {
	VkSurfaceFormatKHR format;
	format.format = VK_FORMAT_B8G8R8A8_UNORM;
	format.colorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
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
			    , uint32 mode_count)
{
    // supported by most hardware
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;
    for (uint32 i = 0
	     ; i < mode_count
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
choose_swapchain_extent(const VkSurfaceCapabilitiesKHR *capabilities)
{
    if (capabilities->currentExtent.width != UINT_MAX)
    {
	return(capabilities->currentExtent);
    }
    else
    {
	int32 width, height;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actual_extent = { width, height };
	actual_extent.width = MAX(capabilities->minImageExtent.width, MIN(capabilities->maxImageExtent.width, actual_extent.width));
	actual_extent.height = MAX(capabilities->minImageExtent.height, MIN(capabilities->maxImageExtent.height, actual_extent.height));

	return(actual_extent);
    }
}

internal VkImageView
create_image_view(VkImage image
		  , VkFormat format
		  , VkImageAspectFlags aspect_flags)
{
    VkImageViewCreateInfo view_info = {};
    view_info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image = image;
    view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
    view_info.format = format;
    view_info.subresourceRange.aspectMask = aspect_flags;
    view_info.subresourceRange.baseMipLevel = 0;
    view_info.subresourceRange.levelCount = 1;
    view_info.subresourceRange.baseArrayLayer = 0;
    view_info.subresourceRange.layerCount = 1;

    VkImageView image_view;
    VK_CHECK(vkCreateImageView(vk.device, &view_info, nullptr, &image_view));

    return(image_view);
}

internal void
init_render_pass(void)
{
    VkAttachmentDescription color	= {};
    color.format			= vk.swapchain.format;
    color.samples			= VK_SAMPLE_COUNT_1_BIT;
    color.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
    color.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
    color.stencilLoadOp			= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color.stencilStoreOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color.initialLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
    color.finalLayout			= VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth	= {};
    depth.format			= vk.swapchain.format;
    depth.samples			= VK_SAMPLE_COUNT_1_BIT;
    depth.loadOp			= VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth.storeOp			= VK_ATTACHMENT_STORE_OP_STORE;
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

    VK_CHECK(vkCreateRenderPass(vk.device, &render_pass_info, nullptr, &vk.render_pass));
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

    VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &layout_info, nullptr, &vk.descriptor_layout));
}

internal VkShaderModule
create_shader(File_Contents *contents)
{
    VkShaderModuleCreateInfo shader_info = {};
    shader_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    shader_info.codeSize = size;
    shader_info.pCode = reinterpret_cast<const uint32 *>(contents->content);

    VkShaderModule shader_module;
    VK_CHECK(vkCreateShaderModule(vk.device, &shader_info, nullptr, &shader_info));

    return(shader_module);
}    

struct Vertex
{
    glm::vec3 pos, color;
    glm::vec2 uvs;

    static VkVertexInputBindingDescription
    get_binding_desecription(void)
    {
	
    }	
};

internal void
init_graphics_pipeline(void)
{
    File_Contents vsh_bytecode = read_file("shaders/vert.spv");
    File_Contents fsh_bytecode = read_file("shaders/frag.spv");

    VkShaderModule v_module = create_module(&vsh_bytecode);
    VkShaderModule f_module = create_module(&fsh_bytecode);

    VkPipelineShaderStageCreateInfo vsh_stage_info = {};
    vsh_stage_info.sType VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vsh_stage_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vsh_stage_info.module = v_module;
    vsh_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo fsh_stage_info = {};
    fsh_stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fsh_stage_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fsh_stage_info.module = f_module;
    fsh_stage_info.pName = "main";

    VkPipelineShaderStageCreateInfo shader_infos[] = {vsh_stage_info, fsh_stage_info};

    VkPipelineVertexInputStateCreateInfo vertex_input_info = {};
    vertex_intput_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    
}

extern_impl void
init_vk(void)
{
    VkInstanceCreateInfo instance_info = {};
    instance_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName = "Vulkan Engine";
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName = "No Engine";
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_0;

    instance_info.pApplicationInfo = &app_info;
    
    // get validation support
    persist constexpr uint32 requested_layer_count = 1;
    persist const char *requested_layers[requested_layer_count] { "VK_LAYER_LUNARG_standard_validation" };

    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count
				       , nullptr);

    VkLayerProperties *properties = (VkLayerProperties *)allocate_stack(sizeof(VkLayerProperties) * layer_count
									, 1
									, "validation_layer_list_allocation");
    vkEnumerateInstanceLayerProperties(&layer_count
				       , properties);

    for (uint32 r = 0; r < requested_layer_count; ++r)
    {
	bool found_layer = false;
	for (uint32 l = 0; l < layer_count; ++l)
	{
	    if (!strcmp(properties[l].layerName, requested_layers[r])) found_layer = true;
	}

	if (!found_layer) assert(false);
    }

    // if found then add to the instance information
    instance_info.enabledLayerCount = requested_layer_count;
    instance_info.ppEnabledLayerNames = requested_layers;

    // get extensions needed
    uint32 glfw_extension_count = 0;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&glfw_extension_count);

    const char **extensions = (const char **)allocate_stack(sizeof(const char *) * glfw_extension_count + 1
							    , 1
							    , "extension_layer_list_allocation");

    memcpy(extensions, glfw_extensions, sizeof(const char *) * 2);
    
    // add the debug utils extension to the list
    extensions[glfw_extension_count] = VK_EXT_DEBUG_UTILS_EXTENSION_NAME;

    instance_info.enabledExtensionCount = glfw_extension_count + 1;
    instance_info.ppEnabledExtensionNames = extensions;

    VK_CHECK(vkCreateInstance(&instance_info
			      , nullptr
			      , &vk.instance)
	     , "failed to create instance\n");

    pop_stack();
    pop_stack();

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

    PFN_vkCreateDebugUtilsMessengerEXT vk_create_debug_utils_messenger = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(vk.instance, "vkCreateDebugUtilsMessengerEXT");
    assert(vk_create_debug_utils_messenger != nullptr);
    VK_CHECK(vk_create_debug_utils_messenger(vk.instance, &debug_info, nullptr, &vk.debug_messenger));
    
    // create the surface
    VK_CHECK(glfwCreateWindowSurface(vk.instance
				     , window
				     , nullptr
				     , &vk.surface));

    uint32 device_count = 0;
    vkEnumeratePhysicalDevices(vk.instance
			       , &device_count
			       , nullptr);
    
    VkPhysicalDevice *devices = (VkPhysicalDevice *)allocate_stack(sizeof(VkPhysicalDevice) * device_count
								   , 1
								   , "physical_device_list_allocation");
    vkEnumeratePhysicalDevices(vk.instance
			       , &device_count
			       , devices);

    OUTPUT_DEBUG_LOG("available physical hardware devices count : %d\n", device_count);

    for (uint32 i = 0
	     ; i < device_count
	     ; ++i)
    {
	Vulkan_GPU gpu;
	gpu.hardware = devices[i];
	
	// check if device is suitable
	if (check_if_physical_device_is_suitable(&gpu))
	{
	    vk.gpu = gpu;
	    break;
	}
    }

    assert(vk.gpu.hardware != VK_NULL_HANDLE);
    OUTPUT_DEBUG_LOG("%s\n", "found gpu compatible with application");

    // create the logical device
    Queue_Family_Indices *indices = &vk.gpu.queue_families;

    Bitset_32 bitset;
    bitset.set1(indices->graphics_family);
    bitset.set1(indices->present_family);

    uint32 unique_sets = bitset.pop_count();

    uint32 *unique_family_indices = (uint32 *)allocate_stack(sizeof(uint32) * unique_sets
							     , 1
							     , "unique_queue_family_indices_allocation");
    VkDeviceQueueCreateInfo *unique_queue_infos = (VkDeviceQueueCreateInfo *)allocate_stack(sizeof(VkDeviceCreateInfo) * unique_sets
											    , 1
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
    device_info.enabledExtensionCount = required_device_extension_count;
    device_info.ppEnabledExtensionNames = required_physical_device_extensions;
    device_info.ppEnabledLayerNames = requested_layers;
    device_info.enabledLayerCount = requested_layer_count;

    VK_CHECK(vkCreateDevice(vk.gpu.hardware
			    , &device_info
			    , nullptr
			    , &vk.device));
    pop_stack();
    pop_stack();

    vkGetDeviceQueue(vk.device, vk.gpu.queue_families.graphics_family, 0, &vk.graphics_queue);
    vkGetDeviceQueue(vk.device, vk.gpu.queue_families.present_family, 0, &vk.present_queue);

    // create swapchain
    Swapchain_Details swapchain_details = get_swapchain_support(vk.gpu.hardware);
    VkSurfaceFormatKHR surface_format = choose_surface_format(swapchain_details.formats, swapchain_details.format_count);
    VkExtent2D surface_extent = choose_swapchain_extent(&swapchain_details.capabilities);
    VkPresentModeKHR present_mode = choose_surface_present_mode(swapchain_details.present_modes
								, swapchain_details.mode_count);

    // add 1 to the minimum images supported in the swapchain
    uint32 image_count = swapchain_details.capabilities.minImageCount + 1;
    if (image_count > swapchain_details.capabilities.maxImageCount)
    {
	image_count = swapchain_details.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info = {};
    swapchain_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface = vk.surface;
    swapchain_info.minImageCount = image_count;
    swapchain_info.imageFormat = surface_format.format;
    swapchain_info.imageColorSpace = surface_format.colorSpace;
    swapchain_info.imageExtent = surface_extent;
    swapchain_info.imageArrayLayers = 1;
    swapchain_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32 queue_family_indices[] = { vk.gpu.queue_families.graphics_family, vk.gpu.queue_families.present_family };

    if (vk.gpu.queue_families.graphics_family != vk.gpu.queue_families.present_family)
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

    swapchain_info.preTransform = swapchain_details.capabilities.currentTransform;
    swapchain_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode = present_mode;
    swapchain_info.clipped = VK_TRUE;
    swapchain_info.oldSwapchain = VK_NULL_HANDLE;

    VK_CHECK(vkCreateSwapchainKHR(vk.device, &swapchain_info, nullptr, &vk.swapchain.swapchain));

    vkGetSwapchainImagesKHR(vk.device, vk.swapchain.swapchain, &image_count, nullptr);
    vk.swapchain.images = (VkImage *)allocate_stack(sizeof(VkImage) * image_count
						    , 1
						    , "swapchain_images_list_allocation");
    vkGetSwapchainImagesKHR(vk.device, vk.swapchain.swapchain, &image_count, vk.swapchain.images);
    vk.swapchain.image_count = image_count;

    vk.swapchain.extent = surface_extent;
    vk.swapchain.format = surface_format.format;
    vk.swapchain.present_mode = present_mode;

    vk.swapchain.image_views = (VkImageView *)allocate_stack(sizeof(VkImageView) * image_count
							     , 1
							     , "swapchain_image_views_list_allocation");

    for (uint32 i = 0
	     ; i < image_count
	     ; ++i)
    {
	vk.swapchain.image_views[i] = create_image_view(vk.swapchain.images[i]
							, surface_format.format
							, VK_IMAGE_ASPECT_COLOR_BIT);
    }

    init_render_pass();
    init_descriptor_layout();

    
}

