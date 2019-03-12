#pragma once

#include "core.hpp"
#include "vulkan.hpp"

/* all rendering objects (render passes, buffers, images, etc...) are accessed
   through 32 bit handles (with object respective names) however, if the handle is 
   not known in a certain scope, it can be accessed through a "[name]"_hash identifier 
   which will serve as a lookup into a hash table mapping string hashes to object handles
   a handle cache is maintained for extra speed (however if that extra speed is negligible, 
   in the future, all objects will simply be accessed through these strings) */

namespace Rendering
{        

    struct Rendering_Objects_Handle_Cache
    {
	Vulkan_API::Render_Pass_Handle test_render_pass;
	Vulkan_API::Descriptor_Set_Layout_Handle descriptor_set_layout;
	Vulkan_API::Graphics_Pipeline_Handle graphics_pipeline;
	Vulkan_API::Command_Pool_Handle graphics_command_pool;
	Vulkan_API::Image2D_Handle depth_image;
	Vulkan_API::Image2D_Handle texture;

	Memory_Buffer_View<Vulkan_API::Buffer_Handle> uniform_buffers;
	Memory_Buffer_View<Vulkan_API::Descriptor_Set_Handle> descriptor_sets;
    };

    namespace Old
    {

	void
	init_vk(GLFWwindow *window);

	void
	draw_frame(void);

	void
	destroy_vk(void);
	
    }
    
    void
    init_rendering_state(Vulkan_API::State *vulkan_state
			 , Rendering_Objects_Handle_Cache *cache);

}
