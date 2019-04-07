#pragma once

#include "vulkan.hpp"

namespace Load
{

    enum Vertex_Attribute_Bits {POSITION = 1 << 0
				, NORMAL = 1 << 1
				, UVS = 1 << 2
				, COLOR = 1 << 3};
    
    void
    load_model_from_obj(Vulkan_API::Model *dst
			, const char *filename
			, Vertex_Attribute_Bits attributes);
    
}
