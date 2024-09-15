#!/usr/bin/env zsh

set -ex

glslangValidator -V shaders/gradient.comp -o shaders/gradient.comp.spv
glslangValidator -V shaders/colored-triangle.vert -o shaders/colored-triangle.vert.spv
glslangValidator -V shaders/colored-triangle.frag -o shaders/colored-triangle.frag.spv
glslangValidator -V shaders/mesh.vert -o shaders/mesh.vert.spv
glslangValidator -V shaders/mesh.frag -o shaders/mesh.frag.spv
glslangValidator -V shaders/wireframe.vert -o shaders/wireframe.vert.spv
glslangValidator -V shaders/wireframe.frag -o shaders/wireframe.frag.spv
