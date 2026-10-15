#version 450

// input of the vertex
// locations should match with attributes[]
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

// before rendering the box c++
layout(push_constant) uniform Push {
    // MVP: projection + view + model matrix (transformation)
    mat4 mvp;
    vec4 color;
    vec4 normalCols[3];
} pc;

// output for frag
layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragUV;
layout(location = 2) flat out vec3  fragColor;
layout(location = 3) flat out vec3  fragCheckerColor;
layout(location = 4) flat out float fragCheckerSquares;

void main()
{
    mat3 normalMatrix = mat3(pc.normalCols[0].xyz, pc.normalCols[1].xyz, pc.normalCols[2].xyz);

    gl_Position = pc.mvp * vec4(inPosition, 1.0); // (x,y,z) --> (x,y,z,1); result: position on clip space
    fragNormal  = normalMatrix * inNormal; // position of the vertex changes --> direction also changes
    fragUV      = inUV;

    fragColor          = pc.color.rgb;
    fragCheckerColor   = vec3(pc.normalCols[0].w, pc.normalCols[1].w, pc.normalCols[2].w);
    fragCheckerSquares = pc.color.a; // color pattern
}
