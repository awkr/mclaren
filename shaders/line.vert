#version 460 core

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "global_state.glsl"

struct UnlitColoredVertex {
    vec3 position;
    vec4 color;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
    UnlitColoredVertex vertices[];
};

layout (push_constant) uniform InstanceState {
    mat4 model;
    VertexBuffer vertex_buffer; // actually it's a u64 handle
} instance_state;

layout (location = 0) out vec3 out_color;

void main() {
    UnlitColoredVertex vertex = instance_state.vertex_buffer.vertices[gl_VertexIndex];
    gl_Position = global_state.projection * global_state.view * instance_state.model * vec4(vertex.position, 1.0);
    out_color = vertex.color.rgb;
}
