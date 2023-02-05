#version 450

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragUV;

layout(location = 0) out vec4 fragment;

layout(binding = 1) uniform sampler2D textureSampler;

void main() {
    vec3 baseColor = fragColor * texture(textureSampler, fragUV).rgb;
    fragment = vec4(baseColor, 1.0);
}