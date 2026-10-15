#version 450

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragUV;
layout(location = 2) flat in vec3  fragColor;
layout(location = 3) flat in vec3  fragCheckerColor;
layout(location = 4) flat in float fragCheckerSquares;

layout(location = 0) out vec4 outColor;

void main()
{
    vec3 base = fragColor;

    if (fragCheckerSquares > 0.0) {
        float u = floor(fragUV.x * fragCheckerSquares);
        float v = floor(fragUV.y * fragCheckerSquares);

        if (mod(u + v, 2.0) >= 1.0) {
            // u + v mod 2 --> base color
            base = fragCheckerColor;
        }
    }

    vec3 n        = normalize(fragNormal);
    vec3 lightDir = normalize(vec3(0.8, 0.8, 0.6));

    float diffuse = max(dot(n, lightDir), 0.0);

    outColor = vec4(base * (0.2 + 0.85 * diffuse), 1.0);
}
