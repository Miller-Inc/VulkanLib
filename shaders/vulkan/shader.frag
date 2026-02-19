//#version 450
//layout(set = 0, binding = 0) uniform sampler2D tex;
//
//layout(location = 0) in vec2 fragUV;
//layout(location = 0) out vec4 outColor;
//
//void main() {
//    outColor = texture(tex, fragUV);
//}

#version 450
#extension GL_ARB_shading_language_include : enable
#include "common.glsl"

layout(push_constant) uniform constants {
    float time;
} PushConstants;

layout(location = 0) out vec4 outColor;

in layout(location = 1) vec3 frag_pos;
in layout(location = 2) float time_out;
in layout(location = 3) vec4 color_out;

//uniform samplerBuffer dataBuffer;

void main() {
//    float r = 0.5 + 0.5 * sin(PushConstants.time) * sin(time_out * (5 * frag_pos.x + 1)) * (frag_pos.x + 1);
//    float g = 0.5 + 0.5 * sin(PushConstants.time * 0.5) * cos(time_out * (5 * frag_pos.y + 1)) * (frag_pos.y + 1);
//    float b = 0.5 + 0.5 * cos(PushConstants.time) * sin(6.28 * time_out * (5 * frag_pos.y + 1) / 0.5) * (frag_pos.x + 1);

//    texelFetch(dataBuffer, index);

//    outColor = vec4(r, g, b, 1.0);

    outColor = vec4(color_out.r, color_out.g, color_out.b, 1.0);
//    outColor = vec4(frag_pos.r, frag_pos.g, frag_pos.b, 1.0);
}
