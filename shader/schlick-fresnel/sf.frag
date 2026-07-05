#version 450

layout(location = 0) in vec3 fragNormalWS;
layout(location = 1) in vec3 fragPosWS;

layout(location = 0) out vec4 outColor;

layout(push_constant) uniform FresnelPushConstants {
    vec3  cameraPosWS;
    float f0Scalar;   
    vec3  lightDirWS; 
    float pad0;
    vec3  albedo;
    float pad1;
} pc;

float fresnelSchlick(float cosTheta, float F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    vec3 N = normalize(fragNormalWS);
    vec3 V = normalize(pc.cameraPosWS - fragPosWS);
    vec3 L = normalize(-pc.lightDirWS);
    vec3 H = normalize(V + L);

    float NdotL = max(dot(N, L), 0.0);
    float VdotH = max(dot(V, H), 0.0);

    float F = fresnelSchlick(VdotH, pc.f0Scalar);

    vec3 diffuse  = pc.albedo * NdotL * (1.0 - F);
    vec3 specular = vec3(F) * NdotL;

    outColor = vec4(diffuse + specular - V * 2.0 + 0.5, 1.0);
}
