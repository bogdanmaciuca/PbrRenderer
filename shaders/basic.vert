#version 450

layout(std140, set = 1, binding = 0) uniform Model {
    mat4 uModel;
};
layout(std140, set = 1, binding = 1) uniform View {
    mat4 uView;
};
layout(std140, set = 1, binding = 2) uniform Projection {
    mat4 uProj;
};

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

layout (location = 0) out vec2 oTexCoord;
layout (location = 1) out vec3 oFragPos;
layout (location = 2) out vec3 oNormal;

layout(location = 0) out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
    oTexCoord = aTexCoord;
    oFragPos = gl_Position.xyz;
    oNormal = aNormal;
}
