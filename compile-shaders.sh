#!/usr/bin/env zsh

set -ex

glslangValidator -V shaders/gradient.comp -o shaders/gradient.comp.spv
glslangValidator -V shaders/colored-triangle.vert -o shaders/colored-triangle.vert.spv
glslangValidator -V shaders/colored-triangle.frag -o shaders/colored-triangle.frag.spv
glslangValidator -V shaders/lit.vert -o shaders/lit.vert.spv
glslangValidator -V shaders/lit.frag -o shaders/lit.frag.spv
glslangValidator -V shaders/wireframe.vert -o shaders/wireframe.vert.spv
glslangValidator -V shaders/wireframe.frag -o shaders/wireframe.frag.spv
glslangValidator -V shaders/line.vert -o shaders/line.vert.spv
glslangValidator -V shaders/line.frag -o shaders/line.frag.spv
glslangValidator -V shaders/gizmo-triangle.vert -o shaders/gizmo-triangle.vert.spv
glslangValidator -V shaders/gizmo-triangle.frag -o shaders/gizmo-triangle.frag.spv
glslangValidator -V shaders/gizmo.vert -o shaders/gizmo.vert.spv
glslangValidator -V shaders/gizmo.frag -o shaders/gizmo.frag.spv
glslangValidator -V shaders/colored-line.vert -o shaders/colored-line.vert.spv
glslangValidator -V shaders/colored-line.frag -o shaders/colored-line.frag.spv
