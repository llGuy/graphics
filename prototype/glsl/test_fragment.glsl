#version 400

layout(location = 0) out vec4 Albedo;

in VS_DATA
{
    vec3 WS_Position;
    vec3 WS_Normal;
    vec3 WS_Tangent;
    vec2 UVs;
} fs_in;

void
main(void)
{
    Albedo = vec4(1.0);
}

