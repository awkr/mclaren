#include "geometry.h"
#include "vk_command_buffer.h"
#include <core/logging.h>
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <microprofile.h>

glm::mat4 model_matrix_from_transform(const Transform &transform) noexcept { // 先缩放，再旋转，最后平移
  glm::mat4 model_matrix(1.0f);
  model_matrix = glm::translate(model_matrix, transform.position);
  {
    glm::mat4 rotation_x = glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    glm::mat4 rotation_y = glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 rotation_z = glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    model_matrix = rotation_z * rotation_y * rotation_x * model_matrix;
  }
  model_matrix = glm::scale(model_matrix, transform.scale);
  return model_matrix;
}

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

void create_geometry_from_config(VkContext *vk_context, const GeometryConfig *config, Geometry *geometry) {
  create_geometry(vk_context, config->vertices, config->vertex_count, config->vertex_stride, config->indices, config->index_count, config->index_stride, config->aabb, geometry);
}

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

void create_cube_geometry(VkContext *vk_context, float length, Geometry *geometry) {
    Vertex vertices[24];
    // clang-format off
    float half_length = length * 0.5f;
    vertices[0] = {{-half_length, -half_length,  half_length}, {0.0f, 0.0f}, {0.0f, 0.0f,  1.0f}}; // front
    vertices[1] = {{ half_length, -half_length,  half_length}, {1.0f, 0.0f}, {0.0f, 0.0f,  1.0f}};
    vertices[2] = {{-half_length,  half_length,  half_length}, {0.0f, 1.0f}, {0.0f, 0.0f,  1.0f}};
    vertices[3] = {{ half_length,  half_length,  half_length}, {1.0f, 1.0f}, {0.0f, 0.0f,  1.0f}};

    vertices[4] = {{ half_length, -half_length, -half_length}, {0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}}; // back
    vertices[5] = {{-half_length, -half_length, -half_length}, {1.0f, 0.0f}, {0.0f, 0.0f, -1.0f}};
    vertices[6] = {{ half_length,  half_length, -half_length}, {0.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};
    vertices[7] = {{-half_length,  half_length, -half_length}, {1.0f, 1.0f}, {0.0f, 0.0f, -1.0f}};

    vertices[ 8] = {{-half_length, -half_length,  -half_length}, {0.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}}; // left
    vertices[ 9] = {{-half_length, -half_length,   half_length}, {1.0f, 0.0f}, {-1.0f, 0.0f, 0.0f}};
    vertices[10] = {{-half_length,  half_length, -half_length}, {0.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};
    vertices[11] = {{-half_length,  half_length,  half_length}, {1.0f, 1.0f}, {-1.0f, 0.0f, 0.0f}};

    vertices[12] = {{ half_length, -half_length,  half_length}, {0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}}; // right
    vertices[13] = {{ half_length, -half_length, -half_length}, {1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}};
    vertices[14] = {{ half_length,  half_length,  half_length}, {0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};
    vertices[15] = {{ half_length,  half_length, -half_length}, {1.0f, 1.0f}, {1.0f, 0.0f, 0.0f}};

    vertices[16] = {{-half_length,  half_length,  half_length}, {0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}}; // top
    vertices[17] = {{ half_length,  half_length,  half_length}, {1.0f, 0.0f}, {0.0f, 1.0f, 0.0f}};
    vertices[18] = {{-half_length,  half_length, -half_length}, {0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};
    vertices[19] = {{ half_length,  half_length, -half_length}, {1.0f, 1.0f}, {0.0f, 1.0f, 0.0f}};

    vertices[20] = {{-half_length, -half_length, -half_length}, {0.0f, 0.0f}, {0.0f, -1.0f, 0.0f}}; // bottom
    vertices[21] = {{ half_length, -half_length, -half_length}, {1.0f, 0.0f}, {0.0f, -1.0f, 0.0f}};
    vertices[22] = {{-half_length, -half_length,  half_length}, {0.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};
    vertices[23] = {{ half_length, -half_length,  half_length}, {1.0f, 1.0f}, {0.0f, -1.0f, 0.0f}};

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
    //     unit_circle_vertices.emplace_back(cosf(sector_angle), 0, sinf(sector_angle));
    // }
    // std::vector<uint32_t> unit_circle_indices;
    // for (size_t i = 0; i < sector; ++i) {
    //     unit_circle_indices.push_back(0);
    //     unit_circle_indices.push_back(i + 1);
    //     unit_circle_indices.push_back(i + 2);
    // }
}

void generate_solid_circle_geometry_config(const glm::vec3 &center, bool is_facing_up, float radius, uint16_t sector, GeometryConfig *config) noexcept {
    const uint32_t vertex_count = (sector + 1) + 1 /* center vertex */;
    Vertex *vertices = (Vertex *) malloc(sizeof(Vertex) * vertex_count);
    memset(vertices, 0, sizeof(Vertex) * vertex_count);

    const uint32_t index_count = sector * 3;
    uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * index_count);
    memset(indices, 0, sizeof(uint32_t) * index_count);

    const float sector_step = 2 * glm::pi<float>() / (float) sector;

    Vertex *vertex = &vertices[0]; // center vertex
    vertex->position[0] = center.x;
    vertex->position[1] = center.y;
    vertex->position[2] = center.z;
    vertex->tex_coord[0] = 0.5f;
    vertex->tex_coord[1] = 0.5f;
    vertex->normal[0] = 0.0f;
    vertex->normal[1] = is_facing_up ? 1.0f : -1.0f;
    vertex->normal[2] = 0.0f;

    for (size_t i = 0; i <= sector; ++i) {
        const float sector_angle = i * sector_step;
        const float a = cosf(sector_angle);
        const float b = sinf(sector_angle);
        vertex = &vertices[i + 1];
        vertex->position[0] = center.x + a * radius;
        vertex->position[1] = center.y + 0.0f;
        vertex->position[2] = center.z + -b * radius; // 因为 x-z 平面 z 轴是向下的，所以这里取负
        vertex->tex_coord[0] = a * 0.5f + 0.5f;
        vertex->tex_coord[1] = b * 0.5f + 0.5f;
        vertex->normal[0] = 0.0f;
        vertex->normal[1] = is_facing_up ? 1.0f : -1.0f;
        vertex->normal[2] = 0.0f;
    }

    for (size_t i = 0; i < sector; ++i) {
        indices[0 + i * 3] = 0;
        // counter-clockwise
        indices[1 + i * 3] = is_facing_up ? i + 1 : i + 2;
        indices[2 + i * 3] = is_facing_up ? i + 2 : i + 1;
    }

    config->vertex_count = vertex_count;
    config->vertex_stride = sizeof(Vertex);
    config->vertices = vertices;
    config->index_count = index_count;
    config->index_stride = sizeof(uint32_t);
    config->indices = indices;

    generate_aabb_from_vertices(vertices, vertex_count, &config->aabb);
}

void generate_stroke_circle_geometry_config(float radius, uint16_t sector, GeometryConfig *config) noexcept {
    const uint32_t vertex_count = sector + 1;
    const uint32_t vertex_stride = sizeof(Vertex);
    Vertex *vertices = (Vertex *) malloc(vertex_stride * vertex_count);
    memset(vertices, 0, vertex_stride * vertex_count);

    const float sector_step = 2 * glm::pi<float>() / (float) sector;

    for (size_t i = 0; i <= sector; ++i) {
        const float sector_angle = i * sector_step;
        const float a = cosf(sector_angle);
        const float b = sinf(sector_angle);
        Vertex *vertex = &vertices[i];
        vertex->position[0] = a * radius;
        vertex->position[1] = 0.0f;
        vertex->position[2] = -b * radius;
    }

    config->vertex_count = vertex_count;
    config->vertex_stride = vertex_stride;
    config->vertices = vertices;

    generate_aabb_from_vertices(vertices, vertex_count, &config->aabb);
}

void generate_cylinder_geometry_config(float height, float radius, uint16_t sector, GeometryConfig *config) noexcept {
  uint32_t vertex_count_of_solid_circle = (sector + 1) + 1;
  uint32_t index_count_of_solid_circle = sector * 3;

  uint32_t vertex_count = vertex_count_of_solid_circle * 2 + sector * 4;
  Vertex *vertices = (Vertex *) malloc(sizeof(Vertex) * vertex_count);

  uint32_t index_count = index_count_of_solid_circle * 2 + sector * 6;
  uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * index_count);

  float sector_step = 2 * glm::pi<float>() / (float) sector;

  { // generate bottom circle mesh
    Vertex *vertex = &vertices[0]; // center vertex
    vertex->position[0] = 0;
    vertex->position[1] = 0;
    vertex->position[2] = 0;
    vertex->tex_coord[0] = 0.5f;
    vertex->tex_coord[1] = 0.5f;
    vertex->normal[0] = 0.0f;
    vertex->normal[1] = -1.0f;
    vertex->normal[2] = 0.0f;

    for (size_t i = 0; i <= sector; ++i) {
      const float sector_angle = i * sector_step;
      const float a = cosf(sector_angle);
      const float b = sinf(sector_angle);
      vertex = &vertices[i + 1];
      vertex->position[0] = a * radius;
      vertex->position[1] = 0.0f;
      vertex->position[2] = -b * radius;
      vertex->tex_coord[0] = a * 0.5f + 0.5f;
      vertex->tex_coord[1] = b * 0.5f + 0.5f;
      vertex->normal[0] = 0.0f;
      vertex->normal[1] = -1.0f;
      vertex->normal[2] = 0.0f;
    }

    for (size_t i = 0; i < sector; ++i) {
      indices[0 + i * 3] = 0;
      indices[1 + i * 3] = i + 2;
      indices[2 + i * 3] = i + 1;
    }
  }

  { // generate top circle mesh
    Vertex *vertex = &vertices[vertex_count_of_solid_circle]; // center vertex
    vertex->position[0] = 0;
    vertex->position[1] = height;
    vertex->position[2] = 0;
    vertex->tex_coord[0] = 0.5f;
    vertex->tex_coord[1] = 0.5f;
    vertex->normal[0] = 0.0f;
    vertex->normal[1] = 1.0f;
    vertex->normal[2] = 0.0f;

    for (size_t i = 0; i <= sector; ++i) {
      const float sector_angle = i * sector_step;
      const float a = cosf(sector_angle);
      const float b = sinf(sector_angle);
      vertex = &vertices[vertex_count_of_solid_circle + i + 1];
      vertex->position[0] = a * radius;
      vertex->position[1] = height;
      vertex->position[2] = -b * radius;
      vertex->tex_coord[0] = a * 0.5f + 0.5f;
      vertex->tex_coord[1] = b * 0.5f + 0.5f;
      vertex->normal[0] = 0.0f;
      vertex->normal[1] = 1.0f;
      vertex->normal[2] = 0.0f;
    }

    for (size_t i = 0; i < sector; ++i) {
      indices[index_count_of_solid_circle + i * 3 + 0] = vertex_count_of_solid_circle + 0;
      indices[index_count_of_solid_circle + i * 3 + 1] = vertex_count_of_solid_circle + i + 1;
      indices[index_count_of_solid_circle + i * 3 + 2] = vertex_count_of_solid_circle + i + 2;
    }
  }

  { // generate side mesh
    /*
     *  2----3
     *  | \  |
     *  |  \ |
     *  0----1
     *
     *  indices: 0, 1, 2, 2, 1, 3
     */
    for (size_t i = 0; i < sector; ++i) {
      float angle = i * sector_step;
      const glm::vec3 a = glm::normalize(glm::vec3(cosf(angle), 0, -sinf(angle)));
      angle = (i + 1) * sector_step;
      const glm::vec3 b = glm::normalize(glm::vec3(cosf(angle), 0, -sinf(angle)));

      Vertex *vertex = &vertices[vertex_count_of_solid_circle * 2 + i * 4];
      memcpy(vertex->position, vertices[1 + i].position, sizeof(float) * 3);
      vertex->tex_coord[0] = i / (float) sector;
      vertex->tex_coord[1] = 0;
      vertex->normal[0] = a.x;
      vertex->normal[1] = a.y;
      vertex->normal[2] = a.z;

      vertex = &vertices[vertex_count_of_solid_circle * 2 + i * 4 + 1];
      memcpy(vertex->position, vertices[1 + i + 1].position, sizeof(float) * 3);
      vertex->tex_coord[0] = (i + 1) / (float) sector;
      vertex->tex_coord[1] = 0;
      vertex->normal[0] = b.x;
      vertex->normal[1] = b.y;
      vertex->normal[2] = b.z;

      vertex = &vertices[vertex_count_of_solid_circle * 2 + i * 4 + 2];
      memcpy(vertex->position, vertices[vertex_count_of_solid_circle + 1 + i].position, sizeof(float) * 3);
      vertex->tex_coord[0] = i / (float) sector;
      vertex->tex_coord[1] = 1;
      vertex->normal[0] = a.x;
      vertex->normal[1] = a.y;
      vertex->normal[2] = a.z;

      vertex = &vertices[vertex_count_of_solid_circle * 2 + i * 4 + 3];
      memcpy(vertex->position, vertices[vertex_count_of_solid_circle + 1 + i + 1].position, sizeof(float) * 3);
      vertex->tex_coord[0] = (i + 1) / (float) sector;
      vertex->tex_coord[1] = 1;
      vertex->normal[0] = b.x;
      vertex->normal[1] = b.y;
      vertex->normal[2] = b.z;
    }

    for (size_t i = 0; i < sector; ++i) {
      indices[index_count_of_solid_circle * 2 + i * 6 + 0] = vertex_count_of_solid_circle * 2 + i * 4 + 0;
      indices[index_count_of_solid_circle * 2 + i * 6 + 1] = vertex_count_of_solid_circle * 2 + i * 4 + 1;
      indices[index_count_of_solid_circle * 2 + i * 6 + 2] = vertex_count_of_solid_circle * 2 + i * 4 + 2;

      indices[index_count_of_solid_circle * 2 + i * 6 + 3] = vertex_count_of_solid_circle * 2 + i * 4 + 2;
      indices[index_count_of_solid_circle * 2 + i * 6 + 4] = vertex_count_of_solid_circle * 2 + i * 4 + 1;
      indices[index_count_of_solid_circle * 2 + i * 6 + 5] = vertex_count_of_solid_circle * 2 + i * 4 + 3;
    }
  }

  config->vertex_count = vertex_count;
  config->vertex_stride = sizeof(Vertex);
  config->vertices = vertices;
  config->index_count = index_count;
  config->index_stride = sizeof(uint32_t);
  config->indices = indices;
}

void generate_torus_geometry_config(float major_radius, float minor_radius, uint16_t sector, uint16_t side, GeometryConfig *config) noexcept {
  ASSERT(sector >= 3);
  // R = major_radius
  // r = minor_radius
  // x = (R + r * cos(u)) * sin(v) = R * sin(v) + r * cos(u) * sin(v)
  // y = r * sin(u)
  // z = (R + r * cos(u)) * cos(v) = R * cos(v) + r * cos(u) * cos(v)
  // where u: side angle [0, 360]
  //       v: sector angle [0, 360]

  uint32_t vertex_count = (side + 1) * (sector + 1);
  Vertex *vertices = (Vertex *) malloc(sizeof(Vertex) * vertex_count);

  uint32_t index_count = sector * side * 6;
  uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * index_count);

  float sector_angle_step = 2 * glm::pi<float>() / (float) sector;
  float side_angle_step = 2 * glm::pi<float>() / (float) side;
  for (size_t i = 0; i <= sector; ++i) {
    float sector_angle = i * sector_angle_step;
    for (size_t j = 0; j <= side; ++j) {
      float side_angle = j * side_angle_step;

      float y = minor_radius * sinf(side_angle);
      float x = minor_radius * cosf(side_angle) * sinf(sector_angle);
      float z = minor_radius * cosf(side_angle) * cosf(sector_angle);
      glm::vec3 n = glm::normalize(glm::vec3(x, y, z));

      x += major_radius * sinf(sector_angle);
      z += major_radius * cosf(sector_angle);

      float u = (float) i / (float) sector;
      float v = (float) j / (float) side;

      vertices[i * (side + 1) + j] = {{x, y, z}, {u, v}, {n.x, n.y, n.z}};
    }
  }

  /*
   *  2----3
   *  | \  |
   *  |  \ |
   *  0----1
   *
   *  indices: 0, 1, 2, 2, 1, 3
   */
  for (size_t i = 0; i < sector; ++i) {
    for (size_t j = 0; j < side; ++j) {
      indices[(i * side + j) * 6 + 0] = i * (side + 1) + j;
      indices[(i * side + j) * 6 + 1] = (i + 1) * (side + 1) + j;
      indices[(i * side + j) * 6 + 2] = i * (side + 1) + j + 1;

      indices[(i * side + j) * 6 + 3] = i * (side + 1) + j + 1;
      indices[(i * side + j) * 6 + 4] = (i + 1) * (side + 1) + j;
      indices[(i * side + j) * 6 + 5] = (i + 1) * (side + 1) + j + 1;
    }
  }

  config->vertex_count = vertex_count;
  config->vertex_stride = sizeof(Vertex);
  config->vertices = vertices;
  config->index_count = index_count;
  config->index_stride = sizeof(uint32_t);
  config->indices = indices;
}

void generate_cone_geometry_config(float radius, uint16_t sector, GeometryConfig *config) {}

void transform_ray_to_model_space(const Ray *ray, const glm::mat4 &model_matrix, Ray *out_ray) noexcept {
  const glm::mat4 inv_model_matrix = glm::inverse(model_matrix);
  out_ray->origin = glm::vec3(inv_model_matrix * glm::vec4(ray->origin, 1.0f));
  out_ray->direction = glm::normalize(glm::vec3(inv_model_matrix * glm::vec4(ray->direction, 0.0f)));
}

bool raycast_obb(const Ray &ray, const AABB &aabb, const glm::mat4 &model_matrix, float *out_distance) {
  Ray ray_in_model_space{};
  transform_ray_to_model_space(&ray, model_matrix, &ray_in_model_space);
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

float ray_axis_shortest_distance(const Ray &ray, const Axis &axis, float &t, float &s) noexcept {
  const glm::vec3 w0 = ray.origin - axis.origin;

  const float a = glm::dot(ray.direction, ray.direction); // 射线方向的模平方
  const float b = glm::dot(ray.direction, axis.direction); // 射线和轴方向的点积
  const float c = glm::dot(axis.direction, axis.direction); // 轴方向的模平方
  const float d = glm::dot(ray.direction, w0); // 射线方向和 w0 的点积
  const float e = glm::dot(axis.direction, w0); // 轴方向和 w0 的点积

  // 解方程得到 t 和 s
  const float denominator = a * c - b * b;
  if (std::abs(denominator) < 1e-6f) {
    return std::numeric_limits<float>::infinity(); // 射线与轴平行，无最近距离
  }

  t = (b * e - c * d) / denominator; // 射线在最近点的参数
  s = (a * e - b * d) / denominator; // 轴在线段内的最近点参数

  // 如果 s 不在 [0, L] 范围内，则射线不与轴的有效部分相交
  if (s < 0.0f || s > axis.length) {
    return std::numeric_limits<float>::infinity();
  }

  // 计算最近点之间的距离
  const glm::vec3 closest_point_on_ray = ray.origin + t * ray.direction;
  const glm::vec3 closest_point_on_axis = axis.origin + s * axis.direction;
  return glm::length(closest_point_on_ray - closest_point_on_axis);
}

float ray_ring_shortest_distance(const glm::vec3 &ray_origin, const glm::vec3 &ray_dir, const glm::vec3 &circle_center, const glm::vec3 &circle_normal, float radius_inner, float radius_outer) {
  // Step 1: 投影射线到圆环平面
  glm::vec3 oc = ray_origin - circle_center; // 从圆心指向射线起点的向量
  float denom = glm::dot(ray_dir, circle_normal); // 射线方向与圆环法向量点积

  glm::vec3 point_on_plane;
  if (glm::abs(denom) > 1e-6) {
    // 射线与平面不平行，计算交点
    float t = -glm::dot(oc, circle_normal) / denom;
    point_on_plane = ray_origin + t * ray_dir;
  } else {
    // 射线与平面平行，直接投影射线起点
    point_on_plane = ray_origin - glm::dot(oc, circle_normal) * circle_normal;
  }

  // Step 2: 计算点到圆心的距离
  glm::vec3 vec_to_center = point_on_plane - circle_center; // 平面上点到圆心的向量
  float distance_to_center = glm::length(vec_to_center);

  // Step 3: 判断距离与圆环半径的关系
  if (distance_to_center >= radius_inner && distance_to_center <= radius_outer) {
    // 投影点在圆环内或边界上
    return 0.0f;
  } else if (distance_to_center > radius_outer) {
    // 投影点在圆环外，返回到外圆的距离
    return distance_to_center - radius_outer;
  } else {
    // 投影点在圆环内，返回到内圆的距离
    return radius_inner - distance_to_center;
  }
}
