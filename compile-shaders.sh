#!/usr/bin/env zsh

set -ex

glslangValidator -V shaders/gradient.comp -o shaders/gradient.comp.spv
glslangValidator -V shaders/lit.vert -o shaders/lit.vert.spv
glslangValidator -V shaders/lit.frag -o shaders/lit.frag.spv
glslangValidator -V shaders/wireframe.vert -o shaders/wireframe.vert.spv
glslangValidator -V shaders/wireframe.frag -o shaders/wireframe.frag.spv
glslangValidator -V shaders/gizmo-triangle.vert -o shaders/gizmo-triangle.vert.spv
glslangValidator -V shaders/gizmo-triangle.frag -o shaders/gizmo-triangle.frag.spv
glslangValidator -V shaders/gizmo-line.vert -o shaders/gizmo-line.vert.spv
glslangValidator -V shaders/gizmo-line.frag -o shaders/gizmo-line.frag.spv
