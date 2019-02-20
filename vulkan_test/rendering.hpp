#pragma once

#include "core.hpp"
#include "vulkan.hpp"

namespace Rendering
{

    /* all vulkan object handle types : handle types are just indices into an array */
    using Vulkan_Buffer_Handle = uint32;
    using Vulkan_Pipeline_Handle = uint32;
    using Vulkan_Image_Handle = uint32;
    using Vulkan_Image_View_Handle = uint32;
    using Vulkan_Render_Pass_Handle = uint32;
    
    extern Vulkan_Buffer_Handle
    add_buffer(const Constant_String &string);

    extern Vulkan_Buffer_Handle
    get_buffer_handle(const Constant_String &string);
    
    extern Vulkan_API::Buffer *
    get_buffer(Vulkan_Buffer_Handle handle);

    

    extern Vulkan_Render_Pass_Handle
    add_render_pass(const Constant_String &string);

    extern Vulkan_Render_Pass_Handle
    get_render_pass_handle(const Constant_String &string);
	
    extern Vulkan_API::Render_Pass *
    get_render_pass(Vulkan_Buffer_Handle handle);

}
