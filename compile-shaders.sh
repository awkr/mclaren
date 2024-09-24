#!/usr/bin/env zsh

set -ex

glslangValidator -V shaders/gradient.comp -o shaders/gradient.comp.spv
glslangValidator -V shaders/colored-triangle.vert -o shaders/colored-triangle.vert.spv
glslangValidator -V shaders/colored-triangle.frag -o shaders/colored-triangle.frag.spv
glslangValidator -V shaders/mesh.vert -o shaders/mesh.vert.spv
glslangValidator -V shaders/mesh.frag -o shaders/mesh.frag.spv
glslangValidator -V shaders/wireframe.vert -o shaders/wireframe.vert.spv
glslangValidator -V shaders/wireframe.frag -o shaders/wireframe.frag.spv
glslangValidator -V shaders/gizmo.vert -o shaders/gizmo.vert.spv
glslangValidator -V shaders/gizmo.frag -o shaders/gizmo.frag.spv
glslangValidator -V shaders/bounding-box.vert -o shaders/bounding-box.vert.spv
glslangValidator -V shaders/bounding-box.frag -o shaders/bounding-box.frag.spv
