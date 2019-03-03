#pragma once

#include "core.hpp"

namespace Vulkan_API
{

    /* all vulkan object handle types : handle types are just indices into an array */
    using Buffer_Handle			= uint32;
    using Pipeline_Handle		= uint32;
    using Image2D_Handle                = uint32;
    using Image_Sampler_Handle          = uint32;
    using Render_Pass_Handle		= uint32;
    using Descriptor_Set_Layout_Handle	= uint32;
    using Descriptor_Set_Handle		= uint32;
    using Model_Handle			= uint32;
    using Graphics_Pipeline_Handle      = uint32;
    using Command_Pool_Handle           = uint32;
    using Framebuffer_Handle            = uint32;

    enum { UNINITIALIZED_HANDLE = 0xFFFFFFFF };

}
