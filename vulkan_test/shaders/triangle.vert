#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(binding = 0) uniform uniform_buffer_object
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) in vec3 vertex_position;
layout(location = 1) in vec3 vertex_color;
layout(location = 2) in vec2 uvs;

layout(location = 0) out vec3 frag_color;
layout(location = 1) out vec2 frag_uvs;

void main(void)
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vertex_position, 1.0);
    frag_color = vertex_color;
    frag_uvs = uvs;
}
