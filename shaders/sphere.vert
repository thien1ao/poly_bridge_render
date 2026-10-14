#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// 64 + 16 + 48 = 128 bytes, the push constant size every GPU supports.
// Spare components are reused (see PushConstants in renderer.cpp):
//   color.a        = number of checker squares (0 = solid color)
//   normalCols[c].w = checker color channel c
layout(push_constant) uniform Push {
    mat4 mvp;
    vec4 color;
    vec4 normalCols[3];
} pc;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) flat out vec3  fragColor;
layout(location = 3) flat out vec3  fragCheckerColor;
layout(location = 4) flat out float fragCheckerSquares;

void main()
{
    mat3 normalMatrix = mat3(pc.normalCols[0].xyz, pc.normalCols[1].xyz, pc.normalCols[2].xyz);

    gl_Position = pc.mvp * vec4(inPosition, 1.0);
    fragNormal  = normalMatrix * inNormal;
    fragUV      = inUV;

    fragColor          = pc.color.rgb;
    fragCheckerColor   = vec3(pc.normalCols[0].w, pc.normalCols[1].w, pc.normalCols[2].w);
    fragCheckerSquares = pc.color.a;
}
