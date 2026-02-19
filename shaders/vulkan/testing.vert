#version 450
// File: `shaders/vulkan/fullscreen.vert`
layout(location = 0) out vec2 vUV;
void main() {
    // Fullscreen triangle trick: positions cover the viewport with 3 verts
    vec2 pos = vec2( (gl_VertexIndex == 0) ? -1.0 : 3.0,
    (gl_VertexIndex == 0) ? -1.0 : -1.0 );
    // For the third vertex adjust Y
    if (gl_VertexIndex == 2) pos.y = 3.0;
    gl_Position = vec4(pos, 0.0, 1.0);
    // convert clip-space pos (-1..1) to UV (0..1)
    vUV = pos * 0.5 + 0.5;
}
