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
    choose_gpu(void)
    {
	
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

	// choose gpu
	
    }

}
