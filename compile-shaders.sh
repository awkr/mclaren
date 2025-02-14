#!/usr/bin/env zsh

set -ex

TARGET_ENV="--target-env vulkan1.2"

glslangValidator -V shaders/gradient.comp -o shaders/gradient.comp.spv -g $TARGET_ENV
glslangValidator -V shaders/lit.vert -o shaders/lit.vert.spv -g $TARGET_ENV
glslangValidator -V shaders/lit.frag -o shaders/lit.frag.spv -g $TARGET_ENV
glslangValidator -V shaders/wireframe.vert -o shaders/wireframe.vert.spv -g $TARGET_ENV
glslangValidator -V shaders/wireframe.frag -o shaders/wireframe.frag.spv -g $TARGET_ENV
glslangValidator -V shaders/vertex-lit.vert -o shaders/vertex-lit.vert.spv -g $TARGET_ENV
glslangValidator -V shaders/vertex-lit.frag -o shaders/vertex-lit.frag.spv -g $TARGET_ENV
glslangValidator -V shaders/line.vert -o shaders/line.vert.spv -g $TARGET_ENV
glslangValidator -V shaders/line.frag -o shaders/line.frag.spv -g $TARGET_ENV
glslangValidator -V shaders/entity-picking.vert -o shaders/entity-picking.vert.spv -g $TARGET_ENV
glslangValidator -V shaders/entity-picking.frag -o shaders/entity-picking.frag.spv -g $TARGET_ENV
