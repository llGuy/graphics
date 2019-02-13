#define GLFW_INCLUDE_VULKAN
#include <cstring>
#include <stdio.h>
#include <malloc.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vulkan/vulkan.h>
#include <vector>
#include <set>
#include <limits>
#include <algorithm>
#include <string>
#include <iostream>
#include "core.hpp"
#include "model.hpp"
#include "resources.hpp"

extern_impl Vulkan_State vk = {};
extern_impl Game_Graphics graphics = {};
extern_impl Swapchain swapchain = {};
GLFWwindow *window;

global VkBuffer vbo;
global VkDeviceMemory vbo_memory;
global VkBuffer ibo;

void
Queue_Family_Indices::find(VkPhysicalDevice physical_device)
{
    uint32 queue_family_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, nullptr);

    VkQueueFamilyProperties *families = STACK_ALLOC(VkQueueFamilyProperties, queue_family_count);
    vkGetPhysicalDeviceQueueFamilyProperties(physical_device, &queue_family_count, families);

    for (uint32 i = 0
	     ; i < queue_family_count
	     ; ++i)
    {
	if (families[i].queueCount > 0 && families[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
	{
	    graphics_queue_family = i;
	}

	VkBool32 present_support = false;
	vkGetPhysicalDeviceSurfaceSupportKHR(physical_device
					     , i
					     , vk.surface
					     , &present_support);

	if (families[i].queueCount > 0 && present_support)
	{
	    present_queue_family = i;
	}
	if (this->complete())
	{
	    break;
	}
    }
}

std::vector<const char *> requested_layers{ "VK_LAYER_LUNARG_standard_validation" };

internal bool
check_vk_validation_support(void)
{
    uint32 layer_count;
    vkEnumerateInstanceLayerProperties(&layer_count, nullptr);

    VkLayerProperties *available_layers = STACK_ALLOC(VkLayerProperties, layer_count);
    vkEnumerateInstanceLayerProperties(&layer_count, available_layers);

    // check if requested layers exist
    for (uint32 requested = 0
	     ; requested < requested_layers.size()
	     ; ++requested)
    {
	bool layer_found = false;
	for (uint32 available = 0
		 ; available < layer_count
		 ; ++available)
	{
	    if (!strcmp(available_layers[available].layerName, requested_layers[requested]))
	    {
		layer_found = true;
	    }
	}
	if (!layer_found) return(false);
    }
    return(true);
}

struct GLFW_Extensions
{
    std::vector<const char *> ptr;
};

internal GLFW_Extensions
get_required_extensions(void)
{
    uint32 count;
    GLFW_Extensions extensions;
    const char **glfw_extensions = glfwGetRequiredInstanceExtensions(&count);

    extensions.ptr = std::vector<const char *>(glfw_extensions, glfw_extensions + count);

    extensions.ptr.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return(extensions);
}

global const std::vector<const char *> device_extensions =
{
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

internal bool
check_device_extension_support(VkPhysicalDevice device)
{
    uint32 extension_count;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, nullptr);

    VkExtensionProperties *available_extensions = STACK_ALLOC(VkExtensionProperties, extension_count);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extension_count, available_extensions);

    std::set<std::string> required(device_extensions.begin(), device_extensions.end());

    for (uint32 i = 0
	     ; i < extension_count
	     ; ++i)
    {
	required.erase(std::string(available_extensions[i].extensionName));
    }

    return required.empty();
}

Swapchain_Support_Details
get_swapchain_support(VkPhysicalDevice device)
{
    Swapchain_Support_Details details;

    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, vk.surface, &details.capabilities);

    uint32 format_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk.surface, &format_count, nullptr);

    if (format_count != 0)
    {
	details.formats.resize(format_count);
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, vk.surface, &format_count, details.formats.data());
    }

    uint32 present_mode_count;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk.surface, &present_mode_count, nullptr);

    if (present_mode_count != 0)
    {
	details.present_modes.resize(present_mode_count);
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, vk.surface, &present_mode_count, details.present_modes.data());
    }

    return details;
}

internal bool
device_is_suitable(VkPhysicalDevice physical_device)
{
    Queue_Family_Indices indices;
    indices.find(physical_device);

    VkPhysicalDeviceProperties device_properties;
    vkGetPhysicalDeviceProperties(physical_device, &device_properties);

    VkPhysicalDeviceFeatures device_features;
    vkGetPhysicalDeviceFeatures(physical_device, &device_features);

    bool extension_supported = check_device_extension_support(physical_device);

    bool swapchain_adequate = false;
    if (extension_supported)
    {
	Swapchain_Support_Details swapchain_support = get_swapchain_support(physical_device);
	swapchain_adequate = !swapchain_support.formats.empty() && !swapchain_support.present_modes.empty();
    }

    return(extension_supported && swapchain_adequate
	   && (device_properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	   && device_features.geometryShader
	   && device_features.samplerAnisotropy
	   && indices.complete());
}

internal void
init_vk(void)
{
    // check validation support
    API_CHECK(check_vk_validation_support(), "failed to find layers\n");

    // initialize vulkan instance
    VkInstanceCreateInfo	vk_instance_info	= {};
    vk_instance_info.sType				= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;

    VkApplicationInfo	app_info			= {};
    app_info.sType					= VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pApplicationName				= "Vulkan";
    app_info.applicationVersion				= VK_MAKE_VERSION(1, 0, 0);
    app_info.pEngineName				= "No Engine";
    app_info.engineVersion				= VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion					= VK_API_VERSION_1_0;

    vk_instance_info.pApplicationInfo			= &app_info;

    vk_instance_info.enabledLayerCount			= requested_layers.size();
    vk_instance_info.ppEnabledLayerNames		= requested_layers.data();

    auto	extensions				= get_required_extensions();
    vk_instance_info.enabledExtensionCount		= extensions.ptr.size();
    vk_instance_info.ppEnabledExtensionNames		= extensions.ptr.data();

    VK_CHECK(vkCreateInstance(&vk_instance_info
			      , nullptr
			      , &vk.instance), "failed to create vulkan instance\n");

    // initialize vulkan surface with GLFW
    VK_CHECK(glfwCreateWindowSurface(vk.instance, window, nullptr, &vk.surface)
	     , "failed to create surface");

    // find physical device
    uint32 device_count = 0;
    vkEnumeratePhysicalDevices(vk.instance
			       , &device_count
			       , nullptr);
    API_CHECK(device_count, "failed to find physical devices\n");

    VkPhysicalDevice *possible_devices = STACK_ALLOC(VkPhysicalDevice, device_count);
    vkEnumeratePhysicalDevices(vk.instance
			       , &device_count
			       , possible_devices);

    for (uint32 i = 0
	     ; i < device_count
	     ; ++i)
    {
	if (device_is_suitable(possible_devices[i]))
	{
	    vk.physical_device = possible_devices[i];
	    break;
	}
    }

    if (vk.physical_device == VK_NULL_HANDLE)
    {
	printf("couldn't find GPU\n");
	std::runtime_error("");
    }

    vk.queue_family_indices.find(vk.physical_device);

    std::vector<VkDeviceQueueCreateInfo> queue_infos;
    std::set<int32> unique_queue_families = { vk.queue_family_indices.graphics_queue_family
					      , vk.queue_family_indices.present_queue_family};

    // always want queues to have top priority
    float queue_priority = 1.0f;
    for (uint32 queue_family :unique_queue_families)
    {
	VkDeviceQueueCreateInfo queue_create_info	= {};
	queue_create_info.sType				= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queue_create_info.queueFamilyIndex		= queue_family;
	queue_create_info.queueCount			= 1;
	queue_create_info.pQueuePriorities		= &queue_priority;
	queue_infos.push_back(queue_create_info);
    }

    VkPhysicalDeviceFeatures device_features	= {};
    device_features.samplerAnisotropy		= VK_TRUE;

    VkDeviceCreateInfo device_info		= {};
    device_info.sType				= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    device_info.pQueueCreateInfos		= queue_infos.data();
    device_info.queueCreateInfoCount		= queue_infos.size();
    device_info.pEnabledFeatures		= &device_features;
    device_info.enabledExtensionCount		= device_extensions.size();
    device_info.ppEnabledExtensionNames		= device_extensions.data();

    // if enable layers
    device_info.enabledLayerCount = requested_layers.size();
    device_info.ppEnabledLayerNames = requested_layers.data();

    VK_CHECK(vkCreateDevice(vk.physical_device, &device_info, nullptr, &vk.device)
	     , "failed to create device");
}

internal VkSurfaceFormatKHR
choose_swapchain_surface_format(const std::vector<VkSurfaceFormatKHR> &available_formats)
{
    if (available_formats.size() == 1 && available_formats[0].format == VK_FORMAT_UNDEFINED)
    {
	return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
    }
    // if there is more than one check for the suitable one
    for (const auto &available_format :available_formats)
    {
	if (available_format.format == VK_FORMAT_B8G8R8A8_UNORM && available_format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
	{
	    return available_format;
	}
    }
    return available_formats[0];
}

internal VkPresentModeKHR
choose_swapchain_present_mode(const std::vector<VkPresentModeKHR> &modes)
{
    VkPresentModeKHR best_mode = VK_PRESENT_MODE_FIFO_KHR;

    for (const auto &available_present_mode :modes)
    {
	if (available_present_mode == VK_PRESENT_MODE_MAILBOX_KHR)
	{
	    return available_present_mode;
	}
	else if (available_present_mode == VK_PRESENT_MODE_IMMEDIATE_KHR)
	{
	    best_mode = available_present_mode;
	}
    }
    return best_mode;
}

internal VkExtent2D
choose_swapchain_extent(VkSurfaceCapabilitiesKHR capabilities)
{
    if (capabilities.currentExtent.width != std::numeric_limits<uint32>::max())
    {
	return capabilities.currentExtent;
    }
    else
    {
	int32 width, height;
	glfwGetFramebufferSize(window, &width, &height);

	VkExtent2D actual_extent	= {(uint32)width, (uint32)height};

	actual_extent.width		= std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actual_extent.width));
	actual_extent.height		= std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actual_extent.height));

	return actual_extent;
    }
}

internal VkImageView
create_image_view(VkImage image
		  , VkFormat format
		  , VkImageAspectFlags flags)
{
    VkImageViewCreateInfo view_info		= {};
    view_info.sType				= VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    view_info.image				= image;
    view_info.viewType				= VK_IMAGE_VIEW_TYPE_2D;
    view_info.format				= format;
    view_info.subresourceRange.aspectMask	= flags;
    view_info.subresourceRange.baseMipLevel	= 0;
    view_info.subresourceRange.levelCount	= 1;
    view_info.subresourceRange.baseArrayLayer	= 0;
    view_info.subresourceRange.layerCount	= 1;

    VkImageView view;
    VK_CHECK(vkCreateImageView(vk.device, &view_info, nullptr, &view)
	     , "failed to create image view\n");

    return view;
}

internal void
init_swapchain(void)
{
    Swapchain_Support_Details swapchain_support = get_swapchain_support(vk.physical_device);

    VkSurfaceFormatKHR surface_format	= choose_swapchain_surface_format(swapchain_support.formats);
    VkPresentModeKHR present_mode	= choose_swapchain_present_mode(swapchain_support.present_modes);
    VkExtent2D extent			= choose_swapchain_extent(swapchain_support.capabilities);

    uint32 image_count = swapchain_support.capabilities.minImageCount + 1;
    if (swapchain_support.capabilities.maxImageCount > 0 && image_count > swapchain_support.capabilities.maxImageCount)
    {
	image_count = swapchain_support.capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchain_info	= {};

    swapchain_info.sType			= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchain_info.surface			= vk.surface;
    swapchain_info.minImageCount		= image_count;
    swapchain_info.imageFormat			= surface_format.format;
    swapchain_info.imageColorSpace		= surface_format.colorSpace;
    swapchain_info.imageExtent			= extent;
    swapchain_info.imageArrayLayers		= 1;
    swapchain_info.imageUsage			= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    auto indices = vk.queue_family_indices;
    uint32 queue_family_indices[] = { (uint32)indices.graphics_queue_family
				      , (uint32)indices.present_queue_family };

    if (indices.graphics_queue_family != indices.present_queue_family)
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

    swapchain_info.preTransform		= swapchain_support.capabilities.currentTransform;
    swapchain_info.compositeAlpha	= VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchain_info.presentMode		= present_mode;
    swapchain_info.clipped		= VK_TRUE;
    swapchain_info.oldSwapchain		= VK_NULL_HANDLE;

    swapchain.format			= surface_format.format;
    swapchain.extent			= extent;

    VK_CHECK(vkCreateSwapchainKHR(vk.device
				  , &swapchain_info
				  , nullptr
				  , &swapchain.handle)
	     , "failed to create swapchain\n");

    // create swapchain images
    vkGetSwapchainImagesKHR(vk.device, swapchain.handle, &image_count, nullptr);
    swapchain.images.resize(image_count);
    vkGetSwapchainImagesKHR(vk.device, swapchain.handle, &image_count, swapchain.images.data());

    // create swapchain image views
    swapchain.image_views.resize(swapchain.images.size());
    for (uint32 i = 0
	     ; i < swapchain.images.size()
	     ; ++i)
    {
	swapchain.image_views[i] = create_image_view(swapchain.images[i]
						     , swapchain.format
						     , VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

internal VkFormat
find_supported_format(const std::vector<VkFormat> &candidates
		      , VkImageTiling tiling
		      , VkFormatFeatureFlags features)
{
    for (VkFormat format :candidates)
    {
	VkFormatProperties props;
	vkGetPhysicalDeviceFormatProperties(vk.physical_device, format, &props);

	if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
	{
	    return format;
	}
	else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
	{
	    return format;
	}
    }

    VK_CHECK(VK_SUCCESS + 1, "failed to find supported format");
}

internal VkFormat
find_depth_format(void)
{
    return find_supported_format({VK_FORMAT_D32_SFLOAT
		, VK_FORMAT_D32_SFLOAT_S8_UINT
		, VK_FORMAT_D24_UNORM_S8_UINT}
	, VK_IMAGE_TILING_OPTIMAL
	, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

internal void
init_main_render_pass(void)
{
    VkAttachmentDescription color_attachment = {};
    color_attachment.format = swapchain.format;
    color_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    color_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    color_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    color_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    color_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    color_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    color_attachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentDescription depth_attachment = {};
    depth_attachment.format = find_depth_format();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // doesn't need to be stored for present
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription attachments[] = { color_attachment, depth_attachment };

    VkAttachmentReference color_ref = {};
    color_ref.attachment = 0;
    color_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depth_ref = {};
    depth_ref.attachment = 1;
    depth_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass_info = {};
    subpass_info.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    
    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo render_pass_info	= {};
    render_pass_info.sType			= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    render_pass_info.attachmentCount		= sizeof(attachments) / sizeof(attachments[0]);
    render_pass_info.pAttachments		= attachments;
    render_pass_info.subpassCount		= 1;
    render_pass_info.pSubpasses			= &subpass_info;
    render_pass_info.dependencyCount		= 1;
    render_pass_info.pDependencies		= &dependency;

    VK_CHECK(vkCreateRenderPass(vk.device, &render_pass_info, nullptr, &graphics.main_render_pass)
	     , "failed to create render pass\n");
}

internal void
init_descriptor_layout(void)
{
    // uniform buffer and texture sample
    VkDescriptorSetLayoutBinding ubo_layout_binding = {};
    ubo_layout_binding.binding = 0;
    ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    ubo_layout_binding.descriptorCount = 1;
    ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    ubo_layout_binding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding sampler_layout_binding = {};
    sampler_layout_binding.binding = 1;
    sampler_layout_binding.descriptorCount = 1;
    sampler_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler_layout_binding.pImmutableSamplers = nullptr;
    sampler_layout_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding bindings[] = { ubo_layout_binding, sampler_layout_binding };

    VkDescriptorSetLayoutCreateInfo layout_info = {};
    layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layout_info.bindingCount = 2;
    layout_info.pBindings = bindings;

    VK_CHECK(vkCreateDescriptorSetLayout(vk.device, &layout_info, nullptr, &graphics.pipeline.set_layout),
	     "failed to create descriptor set layout\n");
}

internal VkShaderModule
create_shader_module(Byte_Array *bytes)
{
    VkShaderModuleCreateInfo module_info = {};

    module_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    module_info.codeSize = bytes->size;
    module_info.pCode = reinterpret_cast<const uint32 *>(bytes->heap_array);

    VkShaderModule shader_module;
    VK_CHECK(vkCreateShaderModule(vk.device, &module_info, nullptr, &shader_module)
	     , "failed to create shader module\n");

    return shader_module;
}

internal VkPipelineShaderStageCreateInfo
create_shader_info(VkShaderModule module
		   , VkShaderStageFlagBits stage_bits
		   , const char *main_f)
{
    VkPipelineShaderStageCreateInfo info = {};
    info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    info.stage = stage_bits;
    info.module = module;
    info.pName = main_f;

    return info;
}

internal void
init_pipeline(void)
{
    auto vsh_bytecode = read_file("../shaders/vert.spv");
    auto fsh_bytecode = read_file("../shaders/frag.spv");

    VkShaderModule vsh_module = create_shader_module(&vsh_bytecode);
    VkShaderModule fsh_module = create_shader_module(&fsh_bytecode);

    VkPipelineShaderStageCreateInfo vsh_shader_stage_info = create_shader_info(vsh_module
									       , VK_SHADER_STAGE_VERTEX_BIT
									       , "main");

    VkPipelineShaderStageCreateInfo fsh_shader_stage_info = create_shader_info(fsh_module
									       , VK_SHADER_STAGE_FRAGMENT_BIT
									       , "main");

    VkPipelineShaderStageCreateInfo stage_infos[] = { vsh_shader_stage_info, fsh_shader_stage_info };

    
}

struct vertex
{
    glm::vec3 position;
    glm::vec3 color;
    glm::vec2 uvs;
};

global std::vector<vertex> vertices
{
    {{-0.5f, -0.5f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, 0.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, 0.0f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, 0.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

    {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
    {{0.5f, -0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
    {{0.5f, 0.5f, -0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
    {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
};

global std::vector<uint32> indices
{
    0, 1, 2, 2, 3, 0,
    4, 5, 6, 6, 7, 4
};

internal void
create_quad(void)
{
    quad_vbo = buffer_manager.release();
    Vk_Buffer *buffer = &buffer_manager.buffers[quad_vbo];

    Vk_Buffer one_use_staging;
    
    // create one binding (1 vbo)
    Model_Binding_Meta vbo_meta_info;
    vbo_meta_info.components_flags = SHIFT(VERTEX_POSITION) | SHIFT(VERTEX_COLOR) | SHIFT(VERTEX_UVS);
    vbo_meta_info.components_count = 3;
    vbo_meta_info.binding = 0;

    quad_model.descriptions_meta.push_back(std::move(vbo_meta_info));
}    

int32
main (int32 argc
      , char *argv[])
{
    printf("begin session\n\n");

    // initialize GLFW
    API_CHECK(glfwInit(), "failed to initialize GLFW\n");

    // initialize window
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    window = glfwCreateWindow(WINDOW_WIDTH
					 , WINDOW_HEIGHT
					 , "Vulkan"
					 , nullptr
					 , nullptr);

    API_CHECK(window, "failed to create window\n");


    init_vk();
    init_swapchain();
    init_main_render_pass();
    init_descriptor_layout();
    
    return(0);
}

