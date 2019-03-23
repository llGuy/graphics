#version 450

layout(location = 0) in vec2 in_position;
layout(location = 1) in vec2 in_uvs;

layout(location = 0) out vec2 out_uvs;

void
main(void)
{
    gl_Position = vec4(in_position, 0.0f, 1.0f);

    out_uvs = in_uvs;
}
