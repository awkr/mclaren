#version 460 core

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"
#include "global_state.glsl"

layout (push_constant) uniform InstanceState {
    mat4 model;
    vec4 color;
    VertexBuffer vertex_buffer; // actually it's a u64 handle
} instance_state;

layout (location = 0) out flat vec4 out_color;

void main() {
    Vertex vertex = instance_state.vertex_buffer.vertices[gl_VertexIndex];
    gl_Position = global_state.projection * global_state.view * instance_state.model * vec4(vertex.position.xyz, 1.0);
    out_color = instance_state.color;
}
