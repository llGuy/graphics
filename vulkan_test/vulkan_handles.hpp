#pragma once

#include "core.hpp"

namespace Vulkan_API
{

    /* all vulkan object handle types : handle types are just indices into an array */
    using Buffer_Handle			= uint32;
    using Pipeline_Handle		= uint32;
    using Image_Handle			= uint32;
    using Image_View_Handle		= uint32;
    using Render_Pass_Handle		= uint32;
    using Descriptor_Set_Layout_Handle	= uint32;
    using Descriptor_Set_Handle		= uint32;
    using Model_Handle			= uint32;

}
