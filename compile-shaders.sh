#!/usr/bin/env zsh

set -ex

TARGET_ENV="--target-env vulkan1.2"
GEN_DEBUG_INFO="-g"

glslangValidator -V shaders/gradient.comp -o shaders/gradient.comp.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/lit.vert -o shaders/lit.vert.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/lit.frag -o shaders/lit.frag.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/wireframe.vert -o shaders/wireframe.vert.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/wireframe.frag -o shaders/wireframe.frag.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/vertex-lit.vert -o shaders/vertex-lit.vert.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/vertex-lit.frag -o shaders/vertex-lit.frag.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/line.vert -o shaders/line.vert.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/line.frag -o shaders/line.frag.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/entity-picking.vert -o shaders/entity-picking.vert.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/entity-picking.frag -o shaders/entity-picking.frag.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/vertex-shadow.vert -o shaders/vertex-shadow.vert.spv $TARGET_ENV $GEN_DEBUG_INFO
glslangValidator -V shaders/empty.frag -o shaders/empty.frag.spv $TARGET_ENV $GEN_DEBUG_INFO
