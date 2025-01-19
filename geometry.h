#pragma once

#include "mesh.h"
#include <glm/gtc/quaternion.hpp>

/**
 * shading model:
 * - lit
 * - vertex lit
 */

struct Transform {
  glm::vec3 position;
  glm::vec3 euler_angles; // 单位：角度
  glm::vec3 scale;

  glm::quat rotation;
};

inline Transform transform_identity() noexcept {
  Transform transform{};
  transform.position = glm::vec3(0.0f, 0.0f, 0.0f);
  transform.euler_angles = glm::vec3(0.0f, 0.0f, 0.0f);
  transform.scale = glm::vec3(1.0f, 1.0f, 1.0f);
  transform.rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  return transform;
}

glm::mat4 model_matrix_from_transform(const Transform &transform) noexcept;

struct Geometry {
    std::vector<Mesh> meshes;
    // Mesh *meshes;
    // uint32_t mesh_count;
    AABB aabb; // in object space
    Mesh aabb_mesh;
    Transform transform;
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

void create_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride, const uint32_t *indices, uint32_t index_count, uint32_t index_stride, const AABB *aabb, Geometry *geometry);
void create_geometry_from_config(MeshSystemState *mesh_system_state, VkContext *vk_context, const GeometryConfig *config, Geometry *geometry);
void destroy_geometry(VkContext *vk_context, Geometry *geometry);

void create_plane_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, float x, float y, Geometry *geometry);
void create_cube_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, float length, Geometry *geometry);
void create_uv_sphere_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, float radius, uint16_t sectors, uint16_t stacks, Geometry *geometry);
void create_ico_sphere_geometry(VkContext *vk_context, Geometry *geometry);
void create_line_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, const glm::vec3 &a, const glm::vec3 &b, Geometry *geometry);

void generate_cone_geometry_config_lit(float radius, float height, uint16_t sector, GeometryConfig *config);
void generate_cone_geometry_config_vertex_lit(float radius, float height, uint16_t sector, GeometryConfig *config);
void generate_solid_circle_geometry_config(const glm::vec3 &center, bool is_facing_up, float radius, uint16_t sector, GeometryConfig *config) noexcept;
void generate_stroke_circle_geometry_config(float radius, uint16_t sector, GeometryConfig *config) noexcept;
void generate_cylinder_geometry_config(float height, float radius, uint16_t sector, GeometryConfig *config) noexcept;
// sector：major 的圆分成了多少段
void generate_torus_geometry_config(float major_radius, float minor_radius, uint16_t sector, uint16_t side, GeometryConfig *config) noexcept;
void generate_sector_geometry_config(const glm::vec3 &normal, const glm::vec3 &start_pos, const glm::vec3 &end_pos, char clock_dir, uint16_t sector_count, GeometryConfig *config);

struct Ray {
  glm::vec3 origin;
  glm::vec3 direction;
};

void transform_ray_to_model_space(const Ray *ray, const glm::mat4 &model_matrix, Ray *out_ray) noexcept;

struct Plane {
    glm::vec3 normal; // 平面的法向量
    float distance; // 从原点到平面的有向距离
};

struct Sphere {
    glm::vec3 center;
    float radius;
};

Plane create_plane(const glm::vec3 &point_on_plane, const glm::vec3 &normal) noexcept;
inline void reset_plane(Plane &plane) noexcept { memset(&plane, 0, sizeof(plane)); }

struct RaycastHit {
    Geometry *geometry;
    glm::vec3 position;
    float distance;
};

struct RaycastHitResult {
    std::vector<RaycastHit> hits; // sorted by distance
};

bool raycast_obb(const Ray &ray, const AABB &aabb, const glm::mat4 &model_matrix, float &distance);
bool raycast_aabb(const Ray &ray, const AABB &aabb, glm::vec3 &hit_point);
bool raycast_plane(const Ray &ray, const Plane &plane, glm::vec3 &hit_point) noexcept;

struct Axis {
    glm::vec3 origin;
    glm::vec3 direction;
    float length;
};

struct StrokeCircle {
    glm::vec3 center;
    glm::vec3 normal;
    float radius;
};

struct Torus {
  glm::vec3 center; // 圆环中心点
  float R; // 主环半径
  float r; // 管道半径
};

// 计算射线与线段或坐标轴的最近距离（最小二乘法），并返回最近点参数和距离
float ray_axis_shortest_distance(const Ray &ray, const Axis &axis, float &t, float &s) noexcept;

float raycast_ring(
    const Ray &ray,
    const glm::vec3 &circle_center, // 圆环中心
    const glm::vec3 &circle_normal, // 圆环法向量 (单位向量)
    float radius_inner, // 圆环内半径
    float radius_outer // 圆环外半径
);
