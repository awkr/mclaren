#include "geometry.h"
#include "logging.h"
#include "mesh_system.h"
#include "vk_command_buffer.h"
#include <glm/gtc/epsilon.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>
#include <microprofile.h>

glm::mat4 model_matrix_from_transform(const Transform &transform) noexcept { // 先缩放，再旋转，最后平移
  const glm::mat4 translation = glm::translate(glm::mat4(1.0f), transform.position);
  const glm::mat4 rotation = glm::mat4_cast(transform.rotation);
  const glm::mat4 scale = glm::scale(glm::mat4(1.0f), transform.scale);
  glm::mat4 model_matrix(1.0f);
  model_matrix = translation * rotation * scale * model_matrix;
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

void create_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, const void *vertices, uint32_t vertex_count, uint32_t vertex_stride, const uint32_t *indices, uint32_t index_count, uint32_t index_stride, const AABB *aabb, Geometry *geometry) {
    Mesh mesh{};
    create_mesh(mesh_system_state, vk_context, vertices, vertex_count, vertex_stride, indices, index_count, index_stride, &mesh);

    Primitive primitive{};
    primitive.vertex_offset = 0;
    primitive.vertex_count = vertex_count;
    primitive.index_offset = 0;
    primitive.index_count = index_count;

    mesh.primitives.push_back(primitive);

    geometry->meshes.push_back(mesh);

    if (aabb && is_aabb_valid(*aabb)) {
      geometry->aabb = *aabb;
      create_mesh_from_aabb(mesh_system_state, vk_context, *aabb, geometry->aabb_mesh);
    }

    geometry->transform = transform_identity();

    log_debug("created geometry %p", geometry);
}

void create_geometry_from_config(MeshSystemState *mesh_system_state, VkContext *vk_context, const GeometryConfig *config, Geometry *geometry) {
  create_geometry(mesh_system_state, vk_context, config->vertices, config->vertex_count, config->vertex_stride, config->indices, config->index_count, config->index_stride, &config->aabb, geometry);
}

void destroy_geometry(VkContext *vk_context, Geometry *geometry) {
  log_debug("destroy geometry %p", geometry);
  if (is_aabb_valid(geometry->aabb)) {
    destroy_mesh(vk_context, &geometry->aabb_mesh);
  }
  for (Mesh &mesh : geometry->meshes) {
    destroy_mesh(vk_context, &mesh);
  }
  *geometry = Geometry(); // todo 改为memset
}

void create_plane_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, float x, float y, Geometry *geometry) {
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

    create_geometry(mesh_system_state, vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), indices, index_count, sizeof(uint32_t), &aabb, geometry);
}

void create_cube_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, float length, Geometry *geometry) {
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

    create_geometry(mesh_system_state, vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), indices, index_count, sizeof(uint32_t), &aabb, geometry);
}

void create_uv_sphere_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, float radius, uint16_t sectors, uint16_t stacks, Geometry *geometry) {
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

    create_geometry(mesh_system_state, vk_context, vertices.data(), vertices.size(), sizeof(Vertex), indices.data(), indices.size(), sizeof(uint32_t), &aabb, geometry);
}

void create_ico_sphere_geometry(VkContext *vk_context, Geometry *geometry) {}

void create_line_geometry(MeshSystemState *mesh_system_state, VkContext *vk_context, const glm::vec3 &a, const glm::vec3 &b, Geometry *geometry) {
  Vertex vertices[2]{};
  memcpy(vertices[0].position, glm::value_ptr(a), sizeof(float) * 3);
  memcpy(vertices[1].position, glm::value_ptr(b), sizeof(float) * 3);
  create_geometry(mesh_system_state, vk_context, vertices, sizeof(vertices) / sizeof(Vertex), sizeof(Vertex), nullptr, 0, 0, nullptr, geometry);
}

void generate_cone_geometry_config_lit(float radius, float height, uint16_t sector, GeometryConfig *config) {
  const uint32_t vertex_count = 1 + (sector + 1) + sector * 3; // center point + points of base circle + points of side
  Vertex *vertices = (Vertex *) malloc(sizeof(Vertex) * vertex_count);
  memset(vertices, 0, sizeof(Vertex) * vertex_count);

  const uint32_t index_count = sector * 3 * 2;
  uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * index_count);
  memset(indices, 0, sizeof(uint32_t) * index_count);

  const float sector_step = 2 * glm::pi<float>() / (float) sector;

  // base circle

  { // center vertex
    Vertex *vertex = &vertices[0];
    vertex->position[0] = 0.0f;
    vertex->position[1] = 0.0f;
    vertex->position[2] = 0.0f;
    vertex->tex_coord[0] = 0.5f;
    vertex->tex_coord[1] = 0.5f;
    vertex->normal[0] = 0.0f;
    vertex->normal[1] = -1.0f;
    vertex->normal[2] = 0.0f;
  }

  for (size_t i = 0; i <= sector; ++i) {
    float sector_angle = i * sector_step;
    float a = cos(sector_angle);
    float b = sin(sector_angle);
    Vertex *vertex = &vertices[1 + i];
    vertex->position[0] = a * radius; // x
    vertex->position[1] = 0.0f; // y
    vertex->position[2] = b * radius; // z
    vertex->tex_coord[0] = a * 0.5f + 0.5f;
    vertex->tex_coord[1] = b * 0.5f + 0.5f;
    vertex->normal[0] = 0.0f;
    vertex->normal[1] = -1.0f;
    vertex->normal[2] = 0.0f;
  }

  for (size_t i = 0; i < sector; ++i) {
    indices[i * 3] = 0;
    indices[i * 3 + 1] = i + 1;
    indices[i * 3 + 2] = i + 2;
  }

  // side

  uint32_t base_index = 1 + (sector + 1);
  for (size_t i = 0; i < sector; ++i) {
    /*  2
     * /  \
     * 0 - 1
     */
    float sector_angle = i * sector_step;

    float face_angle = sector_angle + sector_step * 0.5f;
    float alpha = atan(radius / height); // 斜面与 Y 轴的夹角
    glm::vec3 normal;
    normal.x = sin(alpha) * height * cos(alpha) * cos(face_angle);
    normal.y = sin(alpha) * height * sin(alpha);
    normal.z = -sin(alpha) * height * cos(alpha) * sin(face_angle);
    normal = glm::normalize(normal);

    { // 0
      Vertex *vertex = &vertices[base_index + i * 3];
      vertex->position[0] = cos(sector_angle) * radius; // x
      vertex->position[1] = 0.0f; // y
      vertex->position[2] = -sin(sector_angle) * radius; // z
      vertex->tex_coord[0] = (float) i / (float) sector;
      vertex->tex_coord[1] = 0.0f;
      vertex->normal[0] = normal.x;
      vertex->normal[1] = normal.y;
      vertex->normal[2] = normal.z;
    }
    { // 1
      Vertex *vertex = &vertices[base_index + i * 3 + 1];
      vertex->position[0] = cos(sector_angle + sector_step) * radius; // x
      vertex->position[1] = 0.0f; // y
      vertex->position[2] = -sin(sector_angle + sector_step) * radius; // z
      vertex->tex_coord[0] = (float) (i + 1) / (float) sector;
      vertex->tex_coord[1] = 0.0f;
      vertex->normal[0] = normal.x;
      vertex->normal[1] = normal.y;
      vertex->normal[2] = normal.z;
    }
    { // 2
      Vertex *vertex = &vertices[base_index + i * 3 + 2];
      vertex->position[0] = 0.0f; // x
      vertex->position[1] = height; // y
      vertex->position[2] = 0.0f; // z
      vertex->tex_coord[0] = ((float) i / (float) sector + (float) (i + 1) / (float) sector) * 0.5f;
      vertex->tex_coord[1] = 1.0f;
      vertex->normal[0] = normal.x;
      vertex->normal[1] = normal.y;
      vertex->normal[2] = normal.z;
    }
  }

  for (size_t i = 0; i < sector; ++i) {
    indices[3 * sector + i * 3] = base_index + i * 3;
    indices[3 * sector + i * 3 + 1] = base_index + i * 3 + 1;
    indices[3 * sector + i * 3 + 2] = base_index + i * 3 + 2;
  }

  AABB aabb{};
  generate_aabb_from_vertices(vertices, vertex_count, &aabb);

  config->vertex_count = vertex_count;
  config->vertex_stride = sizeof(Vertex);
  config->vertices = vertices;
  config->index_count = index_count;
  config->index_stride = sizeof(uint32_t);
  config->indices = indices;
  config->aabb = aabb;
}

void generate_cone_geometry_config_vertex_lit(float radius, float height, uint16_t sector, GeometryConfig *config) {
  const uint32_t vertex_count = 1 + (sector + 1) + sector * 3; // center point + points of base circle + points of side
  ColoredVertex *vertices = (ColoredVertex *) malloc(sizeof(ColoredVertex) * vertex_count);
  memset(vertices, 0, sizeof(ColoredVertex) * vertex_count);

  const uint32_t index_count = sector * 3 * 2;
  uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * index_count);
  memset(indices, 0, sizeof(uint32_t) * index_count);

  const float sector_step = 2 * glm::pi<float>() / (float) sector;

  // base circle

  { // center vertex
    ColoredVertex *vertex = &vertices[0];
    vertex->position[0] = 0.0f;
    vertex->position[1] = 0.0f;
    vertex->position[2] = 0.0f;
    vertex->normal[0] = 0.0f;
    vertex->normal[1] = -1.0f;
    vertex->normal[2] = 0.0f;
  }

  for (size_t i = 0; i <= sector; ++i) {
    float sector_angle = i * sector_step;
    float a = cos(sector_angle);
    float b = sin(sector_angle);
    ColoredVertex *vertex = &vertices[1 + i];
    vertex->position[0] = a * radius; // x
    vertex->position[1] = 0.0f; // y
    vertex->position[2] = b * radius; // z
    vertex->normal[0] = 0.0f;
    vertex->normal[1] = -1.0f;
    vertex->normal[2] = 0.0f;
  }

  for (size_t i = 0; i < sector; ++i) {
    indices[i * 3] = 0;
    indices[i * 3 + 1] = i + 1;
    indices[i * 3 + 2] = i + 2;
  }

  // side

  uint32_t base_index = 1 + (sector + 1);
  for (size_t i = 0; i < sector; ++i) {
    /*  2
     * /  \
     * 0 - 1
     */
    float sector_angle = i * sector_step;

    float face_angle = sector_angle + sector_step * 0.5f;
    float alpha = atan(radius / height); // 斜面与 Y 轴的夹角
    glm::vec3 normal;
    normal.x = sin(alpha) * height * cos(alpha) * cos(face_angle);
    normal.y = sin(alpha) * height * sin(alpha);
    normal.z = -sin(alpha) * height * cos(alpha) * sin(face_angle);
    normal = glm::normalize(normal);

    { // 0
      ColoredVertex *vertex = &vertices[base_index + i * 3];
      vertex->position[0] = cos(sector_angle) * radius; // x
      vertex->position[1] = 0.0f; // y
      vertex->position[2] = -sin(sector_angle) * radius; // z
      vertex->normal[0] = normal.x;
      vertex->normal[1] = normal.y;
      vertex->normal[2] = normal.z;
    }
    { // 1
      ColoredVertex *vertex = &vertices[base_index + i * 3 + 1];
      vertex->position[0] = cos(sector_angle + sector_step) * radius; // x
      vertex->position[1] = 0.0f; // y
      vertex->position[2] = -sin(sector_angle + sector_step) * radius; // z
      vertex->normal[0] = normal.x;
      vertex->normal[1] = normal.y;
      vertex->normal[2] = normal.z;
    }
    { // 2
      ColoredVertex *vertex = &vertices[base_index + i * 3 + 2];
      vertex->position[0] = 0.0f; // x
      vertex->position[1] = height; // y
      vertex->position[2] = 0.0f; // z
      vertex->normal[0] = normal.x;
      vertex->normal[1] = normal.y;
      vertex->normal[2] = normal.z;
    }
  }

  for (size_t i = 0; i < sector; ++i) {
    indices[3 * sector + i * 3] = base_index + i * 3;
    indices[3 * sector + i * 3 + 1] = base_index + i * 3 + 1;
    indices[3 * sector + i * 3 + 2] = base_index + i * 3 + 2;
  }

  config->vertex_count = vertex_count;
  config->vertex_stride = sizeof(Vertex);
  config->vertices = vertices;
  config->index_count = index_count;
  config->index_stride = sizeof(uint32_t);
  config->indices = indices;
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

  AABB aabb{};
  generate_aabb_from_vertices(vertices, vertex_count, &aabb);

  config->vertex_count = vertex_count;
  config->vertex_stride = sizeof(Vertex);
  config->vertices = vertices;
  config->index_count = index_count;
  config->index_stride = sizeof(uint32_t);
  config->indices = indices;
  config->aabb = aabb;
}

void generate_torus_geometry_config(float major_radius, float minor_radius, uint16_t major_sector, uint16_t minor_sector, uint16_t theta, GeometryConfig *config) noexcept {
  ASSERT(major_sector >= 3 && minor_sector >= 3);
  ASSERT(theta > 0 && theta <= 360);
  // R = major_radius
  // r = minor_radius
  // x = (R + r * cos(u)) * sin(v) = R * sin(v) + r * cos(u) * sin(v)
  // y = r * sin(u)
  // z = (R + r * cos(u)) * cos(v) = R * cos(v) + r * cos(u) * cos(v)
  // where u: minor_sector angle [0, 360]
  //       v: major_sector angle [0, 360]
  // see https://www.songho.ca/opengl/gl_torus.html

  uint32_t vertex_count = (minor_sector + 1) * (major_sector + 1);
  if (theta != 360) {
    vertex_count += 2 * ((minor_sector + 1) + 1);
  }
  Vertex *vertices = (Vertex *) malloc(sizeof(Vertex) * vertex_count);

  uint32_t index_count = major_sector * minor_sector * (3 + 3) /* 每个面包含 2 个三角形 */;
  if (theta != 360) {
    index_count += 2 * (minor_sector * 3);
  }
  uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * index_count);

  const float major_sector_angle_step = glm::radians((float) theta) / (float) major_sector;
  const float minor_sector_angle_step = 2 * glm::pi<float>() / (float) minor_sector;
  for (size_t i = 0; i <= major_sector; ++i) {
    float major_sector_angle = i * major_sector_angle_step;
    for (size_t j = 0; j <= minor_sector; ++j) {
      float minor_sector_angle = j * minor_sector_angle_step;

      float y = minor_radius * sinf(minor_sector_angle);
      float x = minor_radius * cosf(minor_sector_angle) * sinf(major_sector_angle);
      float z = minor_radius * cosf(minor_sector_angle) * cosf(major_sector_angle);
      glm::vec3 n = glm::normalize(glm::vec3(x, y, z));

      x += major_radius * sinf(major_sector_angle);
      z += major_radius * cosf(major_sector_angle);

      float u = (float) i / (float) major_sector;
      float v = (float) j / (float) minor_sector;

      vertices[i * (minor_sector + 1) + j] = {{x, y, z}, {u, v}, {n.x, n.y, n.z}};
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
  for (size_t i = 0; i < major_sector; ++i) {
    for (size_t j = 0; j < minor_sector; ++j) {
      indices[(i * minor_sector + j) * 6 + 0] = i * (minor_sector + 1) + j;
      indices[(i * minor_sector + j) * 6 + 1] = (i + 1) * (minor_sector + 1) + j;
      indices[(i * minor_sector + j) * 6 + 2] = i * (minor_sector + 1) + j + 1;

      indices[(i * minor_sector + j) * 6 + 3] = i * (minor_sector + 1) + j + 1;
      indices[(i * minor_sector + j) * 6 + 4] = (i + 1) * (minor_sector + 1) + j;
      indices[(i * minor_sector + j) * 6 + 5] = (i + 1) * (minor_sector + 1) + j + 1;
    }
  }

  if (theta != 360) { // 在两个截断面各增加一个 solid circle
    // 确定两个圆环的圆心，再确定两个圆环
    {
      uint32_t vertex_count_offset = (minor_sector + 1) * (major_sector + 1);
      uint32_t index_count_offset = major_sector * minor_sector * 6;

      // 圆心
      float major_sector_angle = 0;

      float y = 0.0f;
      float x = 0.0f;
      float z = major_radius;

      glm::vec3 n = glm::normalize(glm::cross(glm::vec3{x, y, z}, glm::vec3{0.0f, 1.0f, 0.0f}));

      float u = 0.5f;
      float v = 0.5f;

      vertices[vertex_count_offset] = {{x, y, z}, {u, v}, {n.x, n.y, n.z}};

      // 圆环
      for (size_t j = 0; j <= minor_sector; ++j) {
        float minor_sector_angle = j * minor_sector_angle_step;

        y = minor_radius * sinf(minor_sector_angle);
        x = minor_radius * cosf(minor_sector_angle) * sinf(major_sector_angle);
        z = minor_radius * cosf(minor_sector_angle) * cosf(major_sector_angle);

        x += major_radius * sinf(major_sector_angle);
        z += major_radius * cosf(major_sector_angle);

        u = cos(minor_sector_angle) * 0.5f + 0.5f;
        v = sin(minor_sector_angle) * 0.5f + 0.5f;

        vertices[vertex_count_offset + 1 + j] = {{x, y, z}, {u, v}, {n.x, n.y, n.z}};
      }

      // 索引
      for (size_t j = 0; j < minor_sector; ++j) {
        indices[index_count_offset + j * 3] = vertex_count_offset;
        indices[index_count_offset + j * 3 + 1] = vertex_count_offset + 1 + j;
        indices[index_count_offset + j * 3 + 2] = vertex_count_offset + 1 + j + 1;
      }
    }
    {
      uint32_t vertex_count_offset = (minor_sector + 1) * (major_sector + 1) + (1 + minor_sector + 1);
      uint32_t index_count_offset = major_sector * minor_sector * 6 + minor_sector * 3;

      // 圆心
      float major_sector_angle = glm::radians((float) theta);

      float y = 0.0f;
      float x = major_radius * sinf(major_sector_angle);
      float z = major_radius * cosf(major_sector_angle);

      glm::vec3 n = glm::normalize(glm::cross(glm::vec3{0.0f, 1.0f, 0.0f}, glm::vec3{x, y, z}));

      float u = 0.5f;
      float v = 0.5f;

      vertices[vertex_count_offset] = {{x, y, z}, {u, v}, {n.x, n.y, n.z}};

      // 圆环
      for (size_t j = 0; j <= minor_sector; ++j) {
        float minor_sector_angle = j * minor_sector_angle_step;

        y = minor_radius * sinf(minor_sector_angle);
        x = minor_radius * cosf(minor_sector_angle) * sinf(major_sector_angle);
        z = minor_radius * cosf(minor_sector_angle) * cosf(major_sector_angle);

        x += major_radius * sinf(major_sector_angle);
        z += major_radius * cosf(major_sector_angle);

        u = 1.0f - (cos(minor_sector_angle) * 0.5f + 0.5f);
        v = sin(minor_sector_angle) * 0.5f + 0.5f;

        vertices[vertex_count_offset + 1 + j] = {{x, y, z}, {u, v}, {n.x, n.y, n.z}};
      }

      // 索引
      for (size_t j = 0; j < minor_sector; ++j) {
        indices[index_count_offset + j * 3] = vertex_count_offset;
        indices[index_count_offset + j * 3 + 1] = vertex_count_offset + 1 + j + 1;
        indices[index_count_offset + j * 3 + 2] = vertex_count_offset + 1 + j;
      }
    }
  }

  AABB aabb{};
  generate_aabb_from_vertices(vertices, vertex_count, &aabb);

  config->vertex_count = vertex_count;
  config->vertex_stride = sizeof(Vertex);
  config->vertices = vertices;
  config->index_count = index_count;
  config->index_stride = sizeof(uint32_t);
  config->indices = indices;
  config->aabb = aabb;
}

void generate_sector_geometry_config(const glm::vec3 &normal, const glm::vec3 &start_pos, const glm::vec3 &end_pos, char clock_dir, uint16_t sector_count, GeometryConfig *config) {
  const uint32_t vertex_count = sector_count + 1 + 1;
  ColoredVertex *vertices = (ColoredVertex *) malloc(sizeof(ColoredVertex) * vertex_count);

  const uint32_t index_count = sector_count * 3;
  uint32_t *indices = (uint32_t *) malloc(sizeof(uint32_t) * index_count);

  const glm::vec3 norm_a = glm::normalize(start_pos);
  const glm::vec3 norm_b = glm::normalize(end_pos);

  // 计算夹角
  float angle = std::acos(glm::clamp(glm::dot(norm_a, norm_b), -1.0f, 1.0f));

  const glm::vec3 cross = glm::cross(norm_a, norm_b);
  const float dot = glm::dot(cross, normal);

  if (clock_dir == 'C') {
    if (dot < 0.0f) {
      angle = 2 * glm::pi<float>() - angle;
    }
  } else if (clock_dir == 'c') {
    if (angle = -angle; dot > 0.0f) {
      angle = -2 * glm::pi<float>() - angle;
    }
  }

  const float sector_step = angle / (float) sector_count;

  { // center vertex
    ColoredVertex *vertex = &vertices[0];
    vertex->position = glm::vec3(0.0f);
    vertex->color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    vertex->normal = normal;
  }

  for (size_t i = 0; i <= sector_count; ++i) {
    const float sector_angle = i * sector_step;
    const glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), sector_angle, normal);
    const glm::vec3 pos = glm::vec3(rotation * glm::vec4(start_pos, 1.0f));
    ColoredVertex *vertex = &vertices[1 + i];
    vertex->position = pos;
    vertex->color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
    vertex->normal = normal;
  }

  for (size_t i = 0; i < sector_count; ++i) {
    indices[i * 3] = 0;
    indices[i * 3 + 1] = angle > 0 ? i + 1 : i + 2;
    indices[i * 3 + 2] = angle > 0 ? i + 2 : i + 1;
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

Plane create_plane(const glm::vec3 &point_on_plane, const glm::vec3 &normal) noexcept {
  Plane plane{};
  plane.normal = normal;
  plane.distance = -glm::dot(normal, point_on_plane);
  return plane;
}

bool raycast_obb(const Ray &ray, const AABB &aabb, const glm::mat4 &model_matrix, float &distance) {
  Ray ray_in_model_space{};
  transform_ray_to_model_space(&ray, model_matrix, &ray_in_model_space);
  if (glm::vec3 hit_point; raycast_aabb(ray_in_model_space, aabb, hit_point)) {
    distance = glm::length(hit_point - ray_in_model_space.origin);
    return true;
  }
  return false;
}

bool raycast_aabb(const Ray &ray, const AABB &aabb, glm::vec3 &hit_point) {
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
        hit_point = ray.origin;
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
            hit_point[i] = ray.origin[i] + ts[which_plane] * ray.direction[i];
            if (hit_point[i] < aabb.min[i] || hit_point[i] > aabb.max[i]) {
                return false;
            }
        } else {
            hit_point[i] = candidate_planes[i];
        }
    }

    return true; // ray hits box
}

bool raycast_plane(const Ray &ray, const Plane &plane, glm::vec3 &hit_point) noexcept {
  const float denom = glm::dot(plane.normal, ray.direction);
  if (glm::abs(denom) < glm::epsilon<float>()) {
    return false; // 射线与平面平行或在平面内
  }

  const float t = -(glm::dot(plane.normal, ray.origin) + plane.distance) / denom;
  if (t < 0) {
    return false; // 平面在射线起点的反方向
  }

  hit_point = ray.origin + t * ray.direction;
  return true;
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

float raycast_ring(const Ray &ray, const glm::vec3 &circle_center, const glm::vec3 &circle_normal, float radius_inner, float radius_outer) {
  // Step 1: 投影射线到圆环平面
  const glm::vec3 oc = ray.origin - circle_center; // 从圆心指向射线起点的向量
  const float denom = glm::dot(ray.direction, circle_normal); // 射线方向与圆环法向量点积

  glm::vec3 point_on_plane;
  if (glm::abs(denom) > 1e-6) { // 即射线不垂直于平面法向量
    // 射线与平面不平行，计算交点
    float t = -glm::dot(oc, circle_normal) / denom;
    point_on_plane = ray.origin + t * ray.direction; // 射线方程
  } else {
    // 射线与平面平行，直接投影射线起点
    point_on_plane = ray.origin - glm::dot(oc, circle_normal) * circle_normal;
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
