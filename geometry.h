#pragma once

#include "mesh.h"

struct Geometry {
    std::vector<Mesh> meshes;
    AABB aabb; // in object space
    Mesh aabb_mesh;
    glm::vec3 position;
    glm::vec3 rotation; // 单位为角度
    glm::vec3 scale;
};

struct GeometryConfig {
    uint32_t vertex_count;
    uint32_t vertex_stride;
    void *vertices;
    uint32_t index_count;
    uint32_t index_stride;
    uint32_t *indices;
    AABB aabb;
};

void dispose_geometry_config(GeometryConfig *config) noexcept;

void create_geometry(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride, const uint32_t *indices, uint32_t index_count, uint32_t index_stride, const AABB &aabb, Geometry *geometry);
void create_geometry_from_config(VkContext *vk_context, const GeometryConfig *config, Geometry *geometry);
void destroy_geometry(VkContext *vk_context, Geometry *geometry);

void create_plane_geometry(VkContext *vk_context, float x, float y, Geometry *geometry);
void create_cube_geometry(VkContext *vk_context, float length, Geometry *geometry);
void create_uv_sphere_geometry(VkContext *vk_context, float radius, uint16_t sectors, uint16_t stacks, Geometry *geometry);
void create_ico_sphere_geometry(VkContext *vk_context, Geometry *geometry);
void create_cone_geometry(VkContext *vk_context, Geometry *geometry);
void generate_cone_geometry_config(float base_radius, float height, uint16_t sector, uint16_t stack, GeometryConfig *config);
void generate_solid_circle_geometry_config(const glm::vec3 &center, bool is_facing_up, float radius, uint16_t sector, GeometryConfig *config) noexcept;
void generate_stroke_circle_geometry_config(float radius, uint16_t sector, GeometryConfig *config) noexcept;
void generate_cylinder_geometry_config(float height, float radius, uint16_t sector, GeometryConfig *config) noexcept;
void generate_torus_geometry_config(float major_radius, float minor_radius, uint16_t sector, uint16_t side, GeometryConfig *config) noexcept;

struct Ray {
    glm::vec3 origin;
    glm::vec3 direction;
};

bool raycast_obb(const Ray &ray, const AABB &aabb, const glm::mat4 &model_matrix, float *out_distance);
bool raycast_aabb(const Ray &ray, const AABB &aabb, glm::vec3 &out_hit_point);
