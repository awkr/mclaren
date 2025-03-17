#version 460 core
#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"

layout (set = 0, binding = 1) readonly buffer LightsBuffer {
    mat4 view_projection_matrix;
    vec3 position;
    vec3 direction;
    vec3 ambient;
    vec3 diffuse;
} lights_data;

layout (push_constant) uniform InstanceState {
    mat4 model_matrix;
    VertexBuffer vertex_buffer;
} instance_state;

void main() {
    Vertex vertex = instance_state.vertex_buffer.vertices[gl_VertexIndex];
    gl_Position = lights_data.view_projection_matrix * instance_state.model_matrix * vec4(vertex.position, 1.0);
}
