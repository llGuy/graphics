#include <cstring> 
#include "vulkan.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>

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
	    
	}
	
	extern_impl void
	allocate_gpu_memory(Allocate_GPU_Memory_Params *params
			    , VkDeviceMemory *dest_memory)
	{
	    VkMemoryAllocateInfo alloc_info = {};
	    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	    alloc_info.allocationSize = params->r_allocation_size;
	    alloc_info.memoryTypeIndex = find_memory_type_according_to_requirements(params->r_gpu
										    , params->r_properties
										    , params->r_memory_requirements);

	    VK_CHECK(vkAllocateMemory(params->r_gpu->logical_device
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
									    , 1
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

    internal void
    get_swapchain_support(VkSurfaceKHR *surface
			  , GPU *gpu)
    {
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu->hardware, *surface, &gpu->swapchain_support.capabilities);
	vkGetPhysicalDeviceSurfaceFormatsKHR(gpu->hardware, *surface, &gpu->swapchain_support.available_formats_count, nullptr);

	if (gpu->swapchain_support.available_formats_count != 0)
	{
	    gpu->swapchain_support.available_formats = (VkSurfaceFormatKHR *)allocate_stack(sizeof(VkSurfaceFormatKHR) * gpu->swapchain_support.available_formats_count
											    , 1
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
												, 1
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
											      , 1
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

    internal GPU
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
								       , 1
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
    
    extern_impl void
    init_state(State *state)
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
									    , 1
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
	pop_stack();

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
	init_device(&gpu_extensions
		    , &validation_params
		    , &state->gpu);
    }

}
