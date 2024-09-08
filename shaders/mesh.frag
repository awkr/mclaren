#version 460 core

#extension GL_GOOGLE_include_directive : require

#include "global_state.glsl"

layout (location = 0) in  vec2 tex_coord;
layout (location = 1) in  vec3 normal;
layout (location = 2) in  vec4 color;
layout (location = 0) out vec4 frag_color;

layout (set = 1, binding = 0) uniform sampler2D tex;

void main() {
    // frag_color = color;
    // frag_color = texture(tex, tex_coord);

    const vec3 base_color = vec3(0.9, 0.9, 0.9);
    float diffuse = max(dot(normal, global_state.sunlight_dir), 0.0);
    frag_color = vec4(base_color * diffuse, 1.0);
}
