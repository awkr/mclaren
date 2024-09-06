#version 460 core

layout (location = 0) in  vec4 color;
layout (location = 1) in  vec2 tex_coord;
layout (location = 0) out vec4 frag_color;

layout (set = 1, binding = 0) uniform sampler2D tex;

void main() {
    // frag_color = vec4(color, 1.0);
    // frag_color = color;
    frag_color = texture(tex, tex_coord);
}
