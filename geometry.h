#pragma once

#include "mesh.h"

struct Geometry {
    std::vector<Mesh> meshes;
    glm::vec3 position;
    glm::vec3 rotation;
    glm::vec3 scale;
};

struct GeometryConfig {
    uint32_t vertex_count;
    uint32_t vertex_stride;
    void *vertices;
    uint32_t index_count;
    uint32_t index_stride;
    uint32_t *indices;
};

void dispose_geometry_config(GeometryConfig *config) noexcept;

void create_geometry(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride, const uint32_t *indices, uint32_t index_count, uint32_t index_stride, Geometry *geometry);
void destroy_geometry(VkContext *vk_context, Geometry *geometry);

void create_plane_geometry(VkContext *vk_context, float x, float y, Geometry *geometry);
void create_cube_geometry(VkContext *vk_context, Geometry *geometry);
void create_uv_sphere_geometry(VkContext *vk_context, float radius, uint16_t sectors, uint16_t stacks, Geometry *geometry);
void create_ico_sphere_geometry(VkContext *vk_context, Geometry *geometry);
void create_cone_geometry(VkContext *vk_context, Geometry *geometry);
void create_axis_geometry(VkContext *vk_context, float length, Geometry *geometry) noexcept;
void generate_cone_geometry_config(float base_radius, float height, uint16_t sector, uint16_t stack, GeometryConfig *config);
void generate_circle_geometry_config(float radius, uint16_t sector, GeometryConfig *config) noexcept;
