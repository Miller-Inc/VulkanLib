
struct VertexInfo {
    float x, y, z;
    float u, v;
};

vec4 to_pos(VertexInfo v) {
    return vec4(v.x, v.y, v.z, 1.0);
}

vec4 to_pos(vec3 v) {
    return vec4(v.x, v.y, v.z, 1.0);
}