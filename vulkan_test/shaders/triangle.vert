#version 450

layout(binding = 0) uniform Uniform_Buffer_Object
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform Push_Constants
{
    mat4 model;
} push_k;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_color;
layout(location = 2) in vec2 uvs;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uvs;

void
main(void)
{
    gl_Position = ubo.proj * ubo.view * push_k.model * vec4(vertex_position, 1.0);
    frag_color = vertex_color;
    frag_uvs = uvs;
}
