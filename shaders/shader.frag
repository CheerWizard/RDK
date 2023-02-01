#version 450

layout(location = 0) in vec3 fragColor;

layout(location = 0) out vec4 pixel;

void main() {
    pixel = vec4(fragColor, 1.0);
}