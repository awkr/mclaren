#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"
#include "global_state.glsl"

layout (location = 0) out vec2 out_tex_coord;
layout (location = 1) out vec3 out_normal;

layout (push_constant) uniform InstanceState {
    mat4 model_matrix;
    vec4 color;
    VertexBuffer vertex_buffer; // actually it's a u64 handle
} instance_state;

void main() {
    Vertex vertex = instance_state.vertex_buffer.vertices[gl_VertexIndex];
    gl_Position = global_state.projection * global_state.view * instance_state.model_matrix * vec4(vertex.position, 1.0);
    out_tex_coord = vertex.tex_coord;
    out_normal = (instance_state.model_matrix * vec4(vertex.normal, 0.0)).xyz;
}
