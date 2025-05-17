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
    vec2 padding;
};

// TODO: Not working rn
layout (set = 3, binding = 0) buffer PointLights {
    PointLight lights[POINT_LIGHTS_MAX_NUM];
};

void main() {
    vec3 lightPos = vec3(1, 1, 1);
    vec3 norm = normalize(oNormal);
    vec3 lightDir = normalize(lightPos - oFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * vec3(1);

    vec3 result = diffuse * texture(samplerAlbedo, oTexCoord).xyz;
    FragColor = vec4(result, 1.0);
}

