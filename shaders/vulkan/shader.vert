//#version 450
//layout(location = 0) in vec3 inPos;
//layout(location = 1) in vec2 inUV;
//
//layout(location = 0) out vec2 fragUV;
//
//void main() {
//    fragUV = inUV;
//    gl_Position = vec4(inPos, 1.0);
//}


#version 450
#extension GL_ARB_shading_language_include : enable
#include "common.glsl"

layout(push_constant) uniform constants {
    float time;
    float r, g, b;
    vec3[6] pos;
    vec3[6] normals;
} PushConstants;

// Vertex positions are provided via an SSBO bound at set=0, binding=0
//layout(std430, set = 0, binding = 0) buffer VertBuf {
//    vec4 positions[];
//} Vertices;

layout(binding = 0, set = 0) buffer VertBuf {
    VertexInfo verts[]; // x, y, z, u, v
} Vertices;

layout(binding = 1, set = 0) buffer TriangleBuf {
    uint indices[];
} Triangles;

const vec4 verts[3] = vec4[](
    vec4(-0.25, -0.25, 0.0, 1.0),
    vec4(0.25, -0.25, 0.0, 1.0),
    vec4(0.0, 0.25, 0.0, 1.0)
);

const VertexInfo[6] vertex_data = VertexInfo[](
    VertexInfo(-0.25, -0.25, 0.0, 0.0, 0.0),
    VertexInfo(0.25, -0.25, 0.0, 1.0, 0.0),
    VertexInfo(0.0, 0.25, 0.0, 0.5, 1.0),
    VertexInfo(-0.75, -0.75, 0.0, 0.0, 0.0),
    VertexInfo(-0.25, -0.75, 0.0, 1.0, 0.0),
    VertexInfo(0.0, -0.25, 0.0, 0.5, 1.0)
);

layout(location = 1) out vec3 frag_pos;
layout(location = 2) out float time_out;
layout(location = 3) out vec4 color_out;
//layout(std430, set = 0, binding = 0) buffer ColorBuf {
//    vec4 colors[];
//} Colors;

void main() {
    // Read position for this vertex from the SSBO (already in clip-space / NDC)
//    vec4 p = Vertices.positions[gl_VertexIndex];

//    float x = Vertices.verts[gl_VertexIndex].x, y = Vertices.verts[gl_VertexIndex].y, z = Vertices.verts[gl_VertexIndex].z;
//    float x = vertex_data[gl_VertexIndex].x, y = vertex_data[gl_VertexIndex].y, z = vertex_data[gl_VertexIndex].z;

//    vec4 p = to_pos(vertex_data[gl_VertexIndex]);
    vec4 p = to_pos(PushConstants.pos[gl_VertexIndex - 2]);
//    p = to_pos(Vertices.verts[gl_VertexIndex]);
//    p.x = Vertices.verts[gl_VertexIndex].x + 0.1 * gl_VertexIndex;
//    if (gl_VertexIndex <= 6){
//        p.x = PushConstants.pos[gl_VertexIndex].x;
//        p.y = PushConstants.pos[gl_VertexIndex].y;
//    }

//    const int case_num = (gl_VertexIndex % 3);
//    const float time = PushConstants.time;

//    switch (case_num){
//        case 0:
//            p.x += 0.15 * cos(time * 1.50) * 0.75;
//            p.y += 0.9 * sin(time * 3.00) * 0.25;
//            break;
//        case 1:
//            p.x += 0.1 * cos(6.28 * time / 6.0) * 0.50;
//            p.y += 0.8 * sin(6.28 * time / 9.0) * 0.75;
//            break;
//        case 2:
//            p.x += 0.8 * cos(6.28 * time / 16.0) * 0.75;
//            p.y += 0.1 * sin(6.28 * time / 8.0) * 0.50;
//            break;
//    }
//
    gl_Position = p;

    frag_pos = p.xyz;
//    color_out = vec4(1.0, 0.0, 0.5, 1.0);
//    color_out = vec4(PushConstants.r, PushConstants.g, PushConstants.b, 1.0);
    color_out = to_pos(PushConstants.normals[gl_VertexIndex - 2]);
//    color_out = vec4(colors[gl_VertexIndex]);
//    float time = mod(PushConstants.time, 2 * 3.14159);

}