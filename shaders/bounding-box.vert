#version 460 core

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "global_state.glsl"

struct ColorVertex {
    vec3 position;
    vec4 color;
    vec3 normal;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
    ColorVertex vertices[];
};

layout (push_constant) uniform InstanceState {
    mat4 model;
    vec3 color;
    VertexBuffer vertex_buffer; // actually it's a u64 handle
} instance_state;

layout (location = 0) out vec4 out_color;

void main() {
    ColorVertex vertex = instance_state.vertex_buffer.vertices[gl_VertexIndex];
    gl_Position = global_state.projection * global_state.view * instance_state.model * vec4(vertex.position.xyz, 1.0);
    out_color = vec4(instance_state.color, 1.0f);
}
