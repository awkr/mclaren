#version 460 core

#extension GL_EXT_buffer_reference : require
#extension GL_GOOGLE_include_directive : require

#include "vertex.glsl"

layout (early_fragment_tests) in;

layout (set = 1, binding = 0) buffer EntityPickingSSBO {
  uint data[];
};

layout (location = 0) flat in uint id;
layout (location = 0) out uint out_id;

void main() {
    out_id = id;
    data[0] = id;
}
