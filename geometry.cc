#include "geometry.h"
#include "vk_command_buffer.h"
#include <core/logging.h>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <microprofile.h>

void dispose_geometry_config(GeometryConfig *config) noexcept {
    if (config->vertices) {
        free(config->vertices);
    }
    if (config->indices) {
        free(config->indices);
    }
}

void create_geometry(VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride, const uint32_t *indices, uint32_t index_count, uint32_t index_stride, const AABB &aabb, Geometry *geometry) {
    Mesh mesh{};
    create_mesh(vk_context, vertices, vertex_count, vertex_stride, indices, index_count, index_stride, &mesh);

    Primitive primitive{};
    primitive.vertex_offset = 0;
    primitive.vertex_count = vertex_count;
    primitive.index_offset = 0;
    primitive.index_count = index_count;

    mesh.primitives.push_back(primitive);

    geometry->meshes.push_back(mesh);

    geometry->aabb = aabb;
    create_mesh_from_aabb(vk_context, aabb, geometry->aabb_mesh);
}

void create_geometry_from_config(VkContext *vk_context, const GeometryConfig *config, Geometry *geometry) {}

void destroy_geometry(VkContext *vk_context, Geometry *geometry) {
    destroy_mesh(vk_context, &geometry->aabb_mesh);
    for (Mesh &mesh : geometry->meshes) {
        destroy_mesh(vk_context, &mesh);
    }
}

void create_plane_geometry(VkContext *vk_context, float x, float y, Geometry *geometry) {
    Vertex vertices[4];
    // 2 -- 3
    // | \  |
    // |  \ |
    // 0 -- 1
    const float half_x = x / 2.0f;
    const float half_y = y / 2.0f;
    // clang-format off
    vertices[0] = {{-half_x, -half_y, 0.0f}, {0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    vertices[1] = {{ half_x, -half_y, 0.0f}, {1.0f, 0.0f}, {0.0f, 0.0f, 1.0f}};
    vertices[2] = {{-half_x,  half_y, 0.0f}, {0.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    vertices[3] = {{ half_x,  half_y, 0.0f}, {1.0f, 1.0f}, {0.0f, 0.0f, 1.0f}};
    // clang-format on
    uint32_t indices[6] = {0, 1, 2, 2, 1, 3};
    uint32_t index_count = sizeof(indices) / sizeof(uint32_t);

    AABB aabb{};
    generate_aabb_from_vertices(vertices, sizeof(vertices) / sizeof(Vertex), &aabb);

    create_geometry(vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), indices, index_count, sizeof(uint32_t), aabb, geometry);
}

void create_cube_geometry(VkContext *vk_context, Geometry *geometry) {
    Vertex vertices[24];
    // clang-format off
    vertices[0] = {{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f,  1.0f}}; // front
    vertices[1] = {{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f,  1.0f}};
    vertices[2] = {{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f,  1.0f}};
    vertices[3] = {{ 0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f,  1.0f}};

    vertices[4] = {{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}; // back
    vertices[5] = {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
    vertices[6] = {{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
    vertices[7] = {{-0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};

    vertices[ 8] = {{-0.5f, -0.5f,  -0.5f}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}; // left
    vertices[ 9] = {{-0.5f, -0.5f,   0.5f}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
    vertices[10] = {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};
    vertices[11] = {{-0.5f,  0.5f,  0.5f}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};

    vertices[12] = {{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}; // right
    vertices[13] = {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    vertices[14] = {{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
    vertices[15] = {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

    vertices[16] = {{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}; // top
    vertices[17] = {{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    vertices[18] = {{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};
    vertices[19] = {{ 0.5f,  0.5f, -0.5f}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};

    vertices[20] = {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}; // bottom
    vertices[21] = {{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
    vertices[22] = {{-0.5f, -0.5f,  0.5f}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
    vertices[23] = {{ 0.5f, -0.5f,  0.5f}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};

    uint32_t indices[36] = { 0,  1,  2,  2,  1,  3, // front
                             4,  5,  6,  6,  5,  7, // back
                             8,  9, 10, 10,  9, 11, // left
                            12, 13, 14, 14, 13, 15, // right
                            16, 17, 18, 18, 17, 19,
                            20, 21, 22, 22, 21, 23,};
    // clang-format on
    uint32_t index_count = sizeof(indices) / sizeof(uint32_t);

    AABB aabb{};
    generate_aabb_from_vertices(vertices, sizeof(vertices) / sizeof(Vertex), &aabb);

    create_geometry(vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), indices, index_count, sizeof(uint32_t), aabb, geometry);
}

void create_uv_sphere_geometry(VkContext *vk_context, float radius, uint16_t sectors, uint16_t stacks, Geometry *geometry) {
    assert(sectors < UINT16_MAX && stacks < UINT16_MAX);

    std::vector<Vertex> vertices;

    const float sector_steps = 2 * glm::pi<float>() / (float) sectors;
    const float stack_steps = glm::pi<float>() / (float) stacks;

    for (uint16_t i = 0; i <= stacks; ++i) {
        const float stack_angle = glm::pi<float>() / 2 - (float) i * stack_steps; // starting from pi/2 to -pi/2
        const float zx = radius * cosf(stack_angle);
        const float y = radius * sinf(stack_angle);

        // add (sectors + 1) vertices per stack,
        // first and last vertices have the same position and normal, but different tex coord
        for (uint16_t j = 0; j <= sectors; ++j) {
            const float sector_angle = (float) j * sector_steps; // starting from 0 to 2pi
            const float x = zx * sinf(sector_angle);
            const float z = zx * cosf(sector_angle);

            const glm::vec3 &pos = glm::vec3(x, y, z);
            const glm::vec3 &n = glm::normalize(pos);

            Vertex vertex = {};
            memcpy(vertex.position, glm::value_ptr(pos), sizeof(float) * 3);
            vertex.tex_coord[0] = (float) j / (float) sectors;
            vertex.tex_coord[1] = (float) i / (float) stacks;
            memcpy(vertex.normal, glm::value_ptr(n), sizeof(float) * 3);

            vertices.push_back(vertex);
        }
    }

    // generate CCW index list of sphere triangles
    // a -- a+1
    // |  / |
    // | /  |
    // b -- b+1
    std::vector<uint32_t> indices;
    for (uint16_t i = 0; i <= stacks; ++i) {
        for (uint16_t j = 0; j <= sectors; ++j) {
            uint32_t a = i * (sectors + 1) + j;
            uint32_t b = a + sectors + 1;

            // 2 triangles per sector excluding first and last stacks
            if (i != 0) {
                indices.push_back(a);
                indices.push_back(b);
                indices.push_back(a + 1);
            }
            if (i != (stacks - 1)) {
                indices.push_back(a + 1);
                indices.push_back(b);
                indices.push_back(b + 1);
            }
        }
    }

    AABB aabb{};
    generate_aabb_from_vertices(vertices.data(), vertices.size(), &aabb);

    create_geometry(vk_context, vertices.data(), vertices.size(), sizeof(Vertex), indices.data(), indices.size(), sizeof(uint32_t), aabb, geometry);
}

void create_ico_sphere_geometry(VkContext *vk_context, Geometry *geometry) {}

void create_cone_geometry(VkContext *vk_context, Geometry *geometry) {}

void generate_cone_geometry_config(float base_radius, float height, uint16_t sector, uint16_t stack, GeometryConfig *config) {
    // // generate circle vertices
    // const float sector_step = 2 * glm::pi<float>() / (float) sector;
    // std::vector<glm::vec3> unit_circle_vertices;
    // unit_circle_vertices.emplace_back(0.0f, 0.0f, 0.0f);
    // for (size_t i = 0; i <= sector; ++i) {
    //     float sector_angle = i * sector_step;
    //     unit_circle_vertices.emplace_back(cos(sector_angle), 0, sin(sector_angle));
    // }
    // std::vector<uint32_t> unit_circle_indices;
    // for (size_t i = 0; i < sector; ++i) {
    //     unit_circle_indices.push_back(0);
    //     unit_circle_indices.push_back(i + 1);
    //     unit_circle_indices.push_back(i + 2);
    // }
}

void generate_circle_geometry_config(float radius, uint16_t sector, GeometryConfig *config) noexcept {
    const uint32_t vertex_count = (sector + 1) + 1 /* center vertex */;
    Vertex *vertices = (Vertex *) malloc(sizeof(Vertex) * vertex_count);
    memset(vertices, 0, sizeof(Vertex) * vertex_count);

    const uint32_t index_count = sector * 3;
    uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * index_count);
    memset(indices, 0, sizeof(uint32_t) * index_count);

    const float sector_step = 2 * glm::pi<float>() / (float) sector;

    Vertex *vertex = &vertices[0]; // center vertex
    vertex->position[0] = 0.0f;
    vertex->position[1] = 0.0f;
    vertex->position[2] = 0.0f;
    vertex->tex_coord[0] = 0.5f;
    vertex->tex_coord[1] = 0.5f;
    vertex->normal[0] = 0.0f;
    vertex->normal[1] = 1.0f;
    vertex->normal[2] = 0.0f;

    for (size_t i = 0; i <= sector; ++i) {
        const float sector_angle = i * sector_step;
        const float a = cos(sector_angle);
        const float b = sin(sector_angle);
        vertex = &vertices[i + 1];
        vertex->position[0] = a * radius; // x
        vertex->position[1] = 0.0f; // y
        vertex->position[2] = -b * radius; // z，因为 x-z 平面 z 是向下的，所以这里取负
        vertex->tex_coord[0] = a * 0.5f + 0.5f;
        vertex->tex_coord[1] = b * 0.5f + 0.5f;
        vertex->normal[0] = 0.0f;
        vertex->normal[1] = 1.0f;
        vertex->normal[2] = 0.0f;
    }
    
    for (size_t i = 0; i < sector; ++i) {
        indices[0 + i * 3] = 0;
        indices[1 + i * 3] = i + 1;
        indices[2 + i * 3] = i + 2;
    }

    config->vertex_count = vertex_count;
    config->vertex_stride = sizeof(Vertex);
    config->vertices = vertices;
    config->index_count = index_count;
    config->index_stride = sizeof(uint32_t);
    config->indices = indices;

    generate_aabb_from_vertices(vertices, vertex_count, &config->aabb);
}

void generate_cone_geometry_config(float radius, uint16_t sector, GeometryConfig *config) {}

bool raycast_obb(const Ray &ray, const AABB &aabb, const glm::mat4 &model_matrix, float *out_distance) {
    const glm::mat4 inv_model_matrix = glm::inverse(model_matrix); // transform ray to model space
    Ray ray_in_model_space{};
    ray_in_model_space.origin = glm::vec3(inv_model_matrix * glm::vec4(ray.origin, 1.0f));
    ray_in_model_space.direction = glm::vec3(inv_model_matrix * glm::vec4(ray.direction, 0.0f));
    if (glm::vec3 hit_point; raycast_aabb(ray_in_model_space, aabb, hit_point)) {
        *out_distance = glm::length(hit_point - ray_in_model_space.origin);
        return true;
    }
    return false;
}

bool raycast_aabb(const Ray &ray, const AABB &aabb, glm::vec3 &out_hit_point) {
    // based on Graphics Gems I, "Fast Ray-Box Intersection"
    bool inside = true;
    uint8_t quadrants[3];
    float candidate_planes[3];

    for (uint32_t i = 0; i < 3; ++i) {
        if (ray.origin[i] < aabb.min[i]) {
            inside = false;
            quadrants[i] = 1; // left
            candidate_planes[i] = aabb.min[i];
        } else if (ray.origin[i] > aabb.max[i]) {
            inside = false;
            quadrants[i] = 0; // right
            candidate_planes[i] = aabb.max[i];
        } else {
            quadrants[i] = 2; // middle
        }
    }

    if (inside) { // ray origin inside aabb
        out_hit_point = ray.origin;
        log_debug("inside 1");
        return true;
    }

    // calculate distances to candidate planes
    float ts[3];
    for (uint32_t i = 0; i < 3; ++i) {
        if (quadrants[i] != 2 && std::abs(ray.direction[i]) > std::numeric_limits<float>::epsilon()) {
            ts[i] = (candidate_planes[i] - ray.origin[i]) / ray.direction[i]; // 在轴 i 光线从起点沿着方向移动到平面的距离比例
        } else {
            ts[i] = -1.0f;
        }
    }

    // get largest of the distances for final choice of intersection
    uint32_t which_plane = 0;
    for (uint32_t i = 1; i < 3; ++i) {
        if (ts[i] > ts[which_plane]) {
            which_plane = i;
        }
    }

    // check final candidate actually inside box
    if (ts[which_plane] < 0.0f) { // 光线必须朝反方向前进才能到达 aabb
        return false;
    }
    for (uint32_t i = 0; i < 3; ++i) {
        if (which_plane != i) {
            out_hit_point[i] = ray.origin[i] + ts[which_plane] * ray.direction[i];
            if (out_hit_point[i] < aabb.min[i] || out_hit_point[i] > aabb.max[i]) {
                return false;
            }
        } else {
            out_hit_point[i] = candidate_planes[i];
        }
    }

    return true; // ray hits box
}
