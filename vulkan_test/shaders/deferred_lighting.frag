#version 450

layout(location = 0) in vec2 in_uvs;

layout(location = 0) out vec4 out_color;

const uint G_BUFFER_ALBEDO	= 0;
const uint G_BUFFER_POSITION	= 1;
const uint G_BUFFER_NORMAL	= 2;
const uint G_BUFFER_EXTRA	= 3;
const uint G_BUFFER_TOTAL	= 4;

layout(set = 0, binding = 0) uniform sampler2D g_buffer[G_BUFFER_TOTAL];

void
main(void)
{
    out_color = texture(g_buffer[G_BUFFER_ALBEDO], in_uvs);
}
