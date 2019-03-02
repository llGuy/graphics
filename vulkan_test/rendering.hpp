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
    };
    
    void
    init_rendering_state(Vulkan_API::State *vulkan_state
			 , Rendering_Objects_Handle_Cache *cache);

}
