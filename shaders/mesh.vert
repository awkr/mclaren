#version 460 core
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec4 out_color;

layout (set = 0, binding = 0) uniform GlobalState {
    mat4 view;
    mat4 projection;
} global_state;

struct Vertex {
    vec3 position;
    vec4 color;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout (push_constant) uniform InstanceState {
    mat4 model;
    mat4 view;
    mat4 projection;
    VertexBuffer vertex_buffer; // actually it's a u64 handle
} instance_state;

void main() {
    Vertex vertex = instance_state.vertex_buffer.vertices[gl_VertexIndex];
    gl_Position = instance_state.projection * instance_state.view * instance_state.model * vec4(vertex.position, 1.0);
    out_color = vertex.color;
}
