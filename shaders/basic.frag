#version 450

#define POINT_LIGHTS_MAX_NUM 16

layout (location = 0) in vec2 oTexCoord;
layout (location = 1) in vec3 oFragPos;
layout (location = 2) in vec3 oNormal;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D samplerAlbedo;
layout (set = 2, binding = 1) uniform sampler2D samplerNormal;
layout (set = 2, binding = 2) uniform sampler2D samplerARM;

struct PointLight {
    vec3 pos;
    vec3 color;
};

layout (set = 3, binding = 0) buffer PointLights {
    PointLight lights[POINT_LIGHTS_MAX_NUM];
};

void main() {
    vec3 albedo = texture(samplerAlbedo, oTexCoord).rgb;
    float diffuse = max(0, dot(lights[0].pos - oFragPos, oNormal));
    FragColor = vec4(oNormal, 1);
    //FragColor = texture(samplerNormal, oTexCoord);
    //FragColor = texture(samplerAlbedo, oTexCoord);
}

