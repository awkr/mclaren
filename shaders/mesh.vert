#version 460 core
#extension GL_EXT_buffer_reference : require

layout (location = 0) out vec3 out_color;

struct Vertex {
    vec3 position;
    vec3 color;
};

layout (buffer_reference, std430) readonly buffer VertexBuffer {
    Vertex vertices[];
};

layout (push_constant) uniform PushConstants {
    mat4 model;
    mat4 view;
    mat4 projection;
    VertexBuffer vertex_buffer; // actually it's a u64 handle
} push_constants;

void main() {
    Vertex vertex = push_constants.vertex_buffer.vertices[gl_VertexIndex];
    gl_Position = push_constants.projection * push_constants.view * push_constants.model * vec4(vertex.position, 1.0);
    out_color = vertex.color;
}
