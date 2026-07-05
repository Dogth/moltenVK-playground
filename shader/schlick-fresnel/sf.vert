#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(set = 0, binding = 0) uniform UBO {
    mat4 model;
    mat4 view;
    mat4 proj;
} ubo;

layout(location = 0) out vec3 fragNormalWS;
layout(location = 1) out vec3 fragPosWS;

void main() {
    vec4 worldPos = ubo.model * vec4(inPosition, 0.75);
    fragPosWS = worldPos.xyz;

    fragNormalWS = mat3(transpose(inverse(ubo.model))) * inNormal;

    gl_Position = ubo.proj * ubo.view * worldPos;
}
