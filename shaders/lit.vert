#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"
#include "global_state.glsl"

layout (set = 0, binding = 1) readonly buffer LightsBuffer {
    mat4 view_projection_matrix;
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
} lights_data;

layout (location = 0) out vec2 out_tex_coord;
layout (location = 1) out vec3 out_normal;
layout (location = 2) out vec3 out_frag_pos; // in world space
layout (location = 3) out vec4 out_frag_pos_light_space; // in light space
layout (location = 4) flat out uint out_texture_index;
layout (location = 5) flat out uint out_shadow_map_index;

layout (push_constant) uniform LitInstanceState {
    mat4 model_matrix;
    mat4 normal_matrix;
    vec4 color;
    VertexBuffer vertex_buffer; // actually it's a u64 handle
    uint texture_index;
    uint shadow_map_index;
} instance_state;

const mat4 bias = mat4(
	0.5, 0.0, 0.0, 0.0,
	0.0, 0.5, 0.0, 0.0,
	0.0, 0.0, 1.0, 0.0,
	0.5, 0.5, 0.0, 1.0 );

void main() {
    Vertex vertex = instance_state.vertex_buffer.vertices[gl_VertexIndex];
    gl_Position = global_state.projection * global_state.view * instance_state.model_matrix * vec4(vertex.position, 1.0);
    out_tex_coord = vertex.tex_coord;
    out_normal = (instance_state.model_matrix * vec4(vertex.normal, 0.0)).xyz;
    out_frag_pos = (instance_state.model_matrix * vec4(vertex.position, 1.0)).xyz;
    out_frag_pos_light_space = bias * lights_data.view_projection_matrix * vec4(out_frag_pos, 1.0);
    out_texture_index = instance_state.texture_index;
    out_shadow_map_index = instance_state.shadow_map_index;
}
