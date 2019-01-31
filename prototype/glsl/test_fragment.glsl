#version 400

out vec4 FinalColor;

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
    FinalColor = vec4(fs_in.WS_Normal, 1.0);
}

