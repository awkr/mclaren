#!/usr/bin/env zsh

set -ex

glslangValidator -V shaders/gradient.comp -o shaders/gradient.comp.spv
glslangValidator -V shaders/lit.vert -o shaders/lit.vert.spv
glslangValidator -V shaders/lit.frag -o shaders/lit.frag.spv
glslangValidator -V shaders/wireframe.vert -o shaders/wireframe.vert.spv
glslangValidator -V shaders/wireframe.frag -o shaders/wireframe.frag.spv
glslangValidator -V shaders/gizmo.vert -o shaders/gizmo.vert.spv
glslangValidator -V shaders/gizmo.frag -o shaders/gizmo.frag.spv
glslangValidator -V shaders/line.vert -o shaders/line.vert.spv
glslangValidator -V shaders/line.frag -o shaders/line.frag.spv
glslangValidator -V shaders/object-picking.vert -o shaders/object-picking.vert.spv
glslangValidator -V shaders/object-picking.frag -o shaders/object-picking.frag.spv
