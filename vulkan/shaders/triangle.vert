#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec2 vertex_position;
layout(location = 1) in vec3 vertex_color;
layout(location = 0) out vec3 frag_color;

layout(binding = 0) uniform uniform_buffer_object
{
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

void main(void)
{
    gl_Position = ubo.proj * ubo.view * ubo.model * vec4(vertex_position, 0.0, 1.0);
    frag_color = vertex_color;
}
