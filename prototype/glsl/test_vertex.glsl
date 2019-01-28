#version 400

layout(location = 0) in vec3 VertexPosition;
layout(location = 1) in vec3 VertexNormal;
layout(location = 2) in vec2 VertexUVs;
layout(location = 3) in vec3 VertexTangent;

uniform mat4 ProjectionMatrix;
uniform mat4 ViewMatrix;
uniform mat4 ModelMatrix;

out VS_DATA 
{
    vec3 WS_Position;
    vec3 WS_Normal;
    vec3 WS_Tangent;
    vec2 UVs;
} vs_out;

void
main(void)
{
    vec4 WS_Position = ModelMatrix * vec4(VertexPosition, 1.0);
    gl_Position = ProjectionMatrix * ViewMatrix * WS_Position;

    vs_out.WS_Position = vec3(WS_Position);
    vs_out.WS_Normal = mat3(ModelMatrix) * VertexNormal;
    vs_out.WS_Tangent = mat3(ModelMatrix) * VertexTangent;
    vs_out.UVs = VertexUVs;
}
