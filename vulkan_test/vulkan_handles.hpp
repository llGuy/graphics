#pragma once

#include "core.hpp"

namespace Vulkan_API
{

    /* all vulkan object handle types : handle types are just indices into an array */
    using Buffer_Handle			= u32;
    using Pipeline_Handle		= u32;
    using Image2D_Handle                = u32;
    using Image_Sampler_Handle          = u32;
    using Render_Pass_Handle		= u32;
    using Descriptor_Set_Layout_Handle	= u32;
    using Descriptor_Set_Handle		= u32;
    using Model_Handle			= u32;
    using Graphics_Pipeline_Handle      = u32;
    using Command_Pool_Handle           = u32;
    using Framebuffer_Handle            = u32;

    template <typename T> struct Array_Handle
    {
	u32 count;
	T first;
    };

    template <typename T> struct Objects_View
    {
	u32 count;
	T *view;
    };
    
    enum { UNINITIALIZED_HANDLE = 0xFFFFFFFF };

}
