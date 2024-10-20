#version 460 core

layout (location = 0) out vec4 frag_color;
layout (location = 0) in  vec3 color;

void main() {
    // frag_color = vec4(255.0 / 255.0, 151.0 / 255.0, 0.0 / 255.0, 1.0);
    frag_color = vec4(color, 1.0);
}
