#version 450

layout(std140, set = 1, binding = 0) uniform Projection {
    mat4 uModel;
    mat4 uView;
    mat4 uProj;
};

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

layout(location = 0) out vec2 oTexCoord;

layout(location = 0) out gl_PerVertex {
    vec4 gl_Position;
};

void main() {
    gl_Position = uProj * uView * uModel * vec4(aPos, 1.0);
    oTexCoord = aTexCoord;
}
