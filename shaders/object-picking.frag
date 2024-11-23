#version 460 core

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"

layout (push_constant) uniform InstanceState {
    mat4 model_matrix;
    uint entity_id;
    VertexBuffer vertex_buffer;
} instance_state;

layout (location = 0) out uint entity_id;

void main() {
    entity_id = instance_state.entity_id;
}
