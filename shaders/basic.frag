#version 450

layout (location = 0) in vec2 oTexCoord;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D samplerAlbedo;
layout (set = 2, binding = 1) uniform sampler2D samplerNormal;
layout (set = 2, binding = 2) uniform sampler2D samplerARM;

void main() {
    FragColor = vec4(texture(samplerAlbedo, oTexCoord));
}

