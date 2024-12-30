#!/usr/bin/env zsh

set -ex

glslangValidator -V shaders/gradient.comp -o shaders/gradient.comp.spv
glslangValidator -V shaders/lit.vert -o shaders/lit.vert.spv
glslangValidator -V shaders/lit.frag -o shaders/lit.frag.spv
glslangValidator -V shaders/wireframe.vert -o shaders/wireframe.vert.spv
glslangValidator -V shaders/wireframe.frag -o shaders/wireframe.frag.spv
glslangValidator -V shaders/vertex-lit.vert -o shaders/vertex-lit.vert.spv
glslangValidator -V shaders/vertex-lit.frag -o shaders/vertex-lit.frag.spv
glslangValidator -V shaders/line.vert -o shaders/line.vert.spv
glslangValidator -V shaders/line.frag -o shaders/line.frag.spv
glslangValidator -V shaders/object-picking.vert -o shaders/object-picking.vert.spv
glslangValidator -V shaders/object-picking.frag -o shaders/object-picking.frag.spv
