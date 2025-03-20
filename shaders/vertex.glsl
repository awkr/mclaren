struct Vertex {
    vec3 position;
    vec2 tex_coord;
    vec3 normal;
};

layout (buffer_reference, std140) readonly buffer VertexBuffer {
    Vertex vertices[];
};
