#version 450

#define MAX_POINT_LIGHT_NUM 1024

layout (location = 0) in vec2 oTexCoord;
layout (location = 1) in vec3 oFragPos;
layout (location = 2) in mat3 oTBN;

layout (location = 0) out vec4 FragColor;

layout (set = 2, binding = 0) uniform sampler2D samplerAlbedo;
layout (set = 2, binding = 1) uniform sampler2D samplerNormal;
layout (set = 2, binding = 2) uniform sampler2D samplerARM;

struct PointLight {
    vec4 posRad;
    vec3 color;
};

layout(std140, set = 2, binding = 3) buffer FrameData {
    vec3       uCamPos;
    vec3       uDirLight;
    uint       uPointLightNum;
    PointLight uPointLights[MAX_POINT_LIGHT_NUM];
};

void main() {
    vec3 lightPos = uPointLights[0].posRad.xyz;
    vec3 lightColor = uPointLights[0].color;

    vec3 norm = normalize(texture(samplerNormal, oTexCoord).rgb * 2.0 - 1.0);

    vec3 lightDir = normalize(oTBN * lightPos - oTBN * oFragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor;

    vec3 result = diffuse * texture(samplerAlbedo, oTexCoord).xyz;
    FragColor = vec4(result, 1.0);
}

