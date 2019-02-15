#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 out_color;

layout(location = 0) in vec3 frag_color;
layout(location = 1) in vec2 frag_uvs;

layout(binding = 1) uniform sampler2D texture_sampler;

void main(void)
{
    //    out_color = vec4(frag_uvs, 0.0, 1.0);
    out_color = vec4(texture(texture_sampler, frag_uvs).rgb, 1.0);
}
