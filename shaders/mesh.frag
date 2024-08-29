#version 460 core

layout (location = 0) in  vec4 color;
layout (location = 0) out vec4 frag_color;

void main() {
    // frag_color = vec4(color, 1.0);
    frag_color = color;
}
