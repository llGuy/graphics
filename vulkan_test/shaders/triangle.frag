#version 450

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uvs;

layout(location = 0) out vec4 out_color;

layout(set = 0, binding = 1) uniform sampler2D texture_sampler;

void main(void)
{
    out_color = vec4(frag_color, 1.0);
}
