#version 450
// File: `shaders/vulkan/procedural.frag`
layout(location = 0) in vec2 vUV;
layout(location = 0) out vec4 outColor;

// Uniform block for resolution and time (bind/update from CPU)
layout(set = 0, binding = 0) uniform Params {
    vec2 iResolution;
    float iTime;
} params;

// simple signed-distance function for circle
float sdCircle(vec2 p, float r) {
    return length(p) - r;
}

// signed half-plane distance to edge (normalized)
float edgeDist(vec2 p, vec2 a, vec2 b) {
    vec2 e = b - a;
    vec2 n = normalize(vec2(e.y, -e.x)); // inward/outward depends on winding
    return dot(p - a, n);
}

// approximate signed distance to triangle using half-plane distances
float sdTriangle(vec2 p, vec2 a, vec2 b, vec2 c) {
    float d0 = edgeDist(p, a, b);
    float d1 = edgeDist(p, b, c);
    float d2 = edgeDist(p, c, a);
    // max of half-plane distances: positive outside, negative inside
    return max(max(d0, d1), d2);
}

// anti-aliased smooth step for SDF
float opSmoothUnion(float d1, float d2, float k) {
    float h = clamp(0.5 + 0.5*(d2 - d1)/k, 0.0, 1.0);
    return mix(d2, d1, h) - k*h*(1.0 - h);
}

vec3 palette(float t) {
    // simple palette function
    return vec3(0.5 + 0.5*cos(6.2831*(t + vec3(0.0,0.33,0.67))));
}

void main() {
    // convert uv -> centered coordinates with aspect-preserving scale
    vec2 res = params.iResolution;
    vec2 uv = vUV;
    vec2 centered = (uv * res - 0.5*res) / min(res.x, res.y);

    // animate parameters
    float time = params.iTime;
    // multiple shapes: circle + triangle
    vec2 cPos = vec2(sin(time*0.7)*0.3, cos(time*0.6)*0.2);
    float circleR = 0.25 + 0.05*sin(time*1.3);

    // triangle vertices (rotating)
    float ang = time*0.4;
    mat2 R = mat2(cos(ang), -sin(ang), sin(ang), cos(ang));
    vec2 a = R * vec2(0.0, 0.35) + vec2(0.5, -0.1);
    vec2 b = R * vec2(-0.3, -0.1) + vec2(0.5, -0.1);
    vec2 c = R * vec2(0.3, -0.1) + vec2(0.5, -0.1);
    // move triangle into centered coords
    a = a - vec2(0.0,0.0); b = b; c = c;
    // Space convert triangle coords from normalized (0..1) to centered coord scale
    vec2 toCentered = vec2(res.x < res.y ? 1.0 : res.y/res.x, res.y < res.x ? 1.0 : res.x/res.y);
    // For simplicity, place triangle in centered space around (-0.4,0.0)
    a = (R * (vec2(-0.5, 0.0)) + vec2(-0.3, 0.0));
    b = (R * (vec2(-0.2, -0.35)) + vec2(-0.3, 0.0));
    c = (R * (vec2(0.2, -0.35)) + vec2(-0.3, 0.0));

    // compute SDFs
    float dCircle = sdCircle(centered - cPos, circleR);
    float dTri = sdTriangle(centered, a, b, c);

    // smooth union of shapes
    float k = 0.03; // smoothing radius
    float d = opSmoothUnion(dCircle, dTri, k);

    // anti-aliased edge factor
    float aa = fwidth(d);
    float inside = 1.0 - smoothstep(-aa, aa, d);

    // color composition
    vec3 bg = palette(uv.y + 0.2*sin(time*0.3)); // vertical gradient palette
    vec3 triColor = vec3(0.9, 0.6, 0.2) * (0.5 + 0.5*sin(time*1.2));
    vec3 circColor = vec3(0.2, 0.6, 0.9) * (0.5 + 0.5*cos(time*0.8));

    // blend circle and triangle contributions by proximity
    float tCirc = smoothstep(0.15, -0.05, dCircle);
    float tTri  = smoothstep(0.08, -0.02, dTri);

    vec3 color = bg;
    color = mix(color, circColor, tCirc * 0.95);
    color = mix(color, triColor, tTri * 0.9);

    // add rim/outline using distance
    float outline = smoothstep(0.01, -0.003, abs(d) - 0.0);
    color = mix(color, vec3(0.0), outline * 0.3);

    // subtle vignette
    float v = length(uv - 0.5);
    color *= smoothstep(0.9, 0.4, 1.0 - v);

    // gamma correction (linearize display)
    color = pow(color, vec3(1.0 / 2.2));

    outColor = vec4(color, 1.0);
}
