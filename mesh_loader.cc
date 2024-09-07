#include "mesh_loader.h"
#include "core/logging.h"

void load_gltf(VkContext *vk_context, const char *filepath, Geometry *geometry) {
    cgltf_options options = {};
    cgltf_data *data = nullptr;
    cgltf_result result = cgltf_parse_file(&options, filepath, &data);
    ASSERT(result == cgltf_result_success);

    result = cgltf_load_buffers(&options, data, filepath);
    ASSERT(result == cgltf_result_success);

    geometry->meshes.resize(data->meshes_count);

    for (size_t mesh_index = 0; mesh_index < data->meshes_count; ++mesh_index) {
        const cgltf_mesh *mesh = &data->meshes[mesh_index];

        log_debug("mesh index: %d, name: %s", mesh_index, mesh->name);

        geometry->meshes[mesh_index].primitives.resize(mesh->primitives_count);

        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;// 暂时只支持 u32 索引类型

        for (size_t primitive_index = 0; primitive_index < mesh->primitives_count; ++primitive_index) {
            const cgltf_primitive *primitive = &mesh->primitives[primitive_index];

            log_debug("primitive index: %zu", primitive_index);

            geometry->meshes[mesh_index].primitives[primitive_index].index_offset = indices.size();
            ASSERT(primitive->indices);
            geometry->meshes[mesh_index].primitives[primitive_index].index_count = primitive->indices->count;

            uint32_t vertex_offset = vertices.size();
            uint32_t index_offset = indices.size();

            uint32_t vertex_count = primitive->attributes[0].data->count; // 假设同一 primitive 的所有 attribute 的 count 相同
            vertices.resize(vertex_offset + vertex_count);
            indices.resize(index_offset + primitive->indices->count);

            for (size_t attribute_index = 0; attribute_index < primitive->attributes_count; ++attribute_index) {
                const cgltf_attribute *attribute = &primitive->attributes[attribute_index];

                log_debug("attribute name: %s", attribute->name);

                if (strcmp(attribute->name, "POSITION") == 0) {
                    for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                        void *pos = (void *) ((uintptr_t) attribute->data->buffer_view->buffer->data +
                                              attribute->data->buffer_view->offset +
                                              vertex_index * attribute->data->stride);
                        memcpy(vertices[vertex_offset + vertex_index].pos, pos, attribute->data->stride);
                    }
                } else if (strcmp(attribute->name, "TEXCOORD_0") == 0) {
                    for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                        void *tex_coord = (void *) ((uintptr_t) attribute->data->buffer_view->buffer->data + attribute->data->buffer_view->offset + vertex_index * attribute->data->stride);
                        memcpy(vertices[vertex_offset + vertex_index].tex_coord, tex_coord, attribute->data->stride);
                    }
                } else if (strcmp(attribute->name, "NORMAL") == 0) {
                    for (uint32_t vertex_index = 0; vertex_index < vertex_count; ++vertex_index) {
                        void *normal = (void *) ((uintptr_t) attribute->data->buffer_view->buffer->data + attribute->data->buffer_view->offset + vertex_index * attribute->data->stride);
                        memcpy(vertices[vertex_offset + vertex_index].normal, normal, attribute->data->stride);
                    }
                }
            }

            for (uint32_t index = 0; index < primitive->indices->count; ++index) {
                void *idx = (void *) ((uintptr_t) primitive->indices->buffer_view->buffer->data +
                                      primitive->indices->buffer_view->offset +
                                      index * primitive->indices->stride);
                if (primitive->indices->component_type == cgltf_component_type_r_8u) {
                    indices[index_offset + index] = *(uint8_t *) idx;
                } else if (primitive->indices->component_type == cgltf_component_type_r_16u) {
                    indices[index_offset + index] = *(uint16_t *) idx;
                } else if (primitive->indices->component_type == cgltf_component_type_r_32u) {
                    indices[index_offset + index] = *(uint32_t *) idx;
                } else {
                    ASSERT_MESSAGE(false, "unsupported index component type - %d", primitive->indices->component_type);
                }
            }
        } // end looping primitives

        create_mesh_buffer(vk_context, vertices.data(), vertices.size(), sizeof(Vertex), indices.data(), indices.size(),
                           sizeof(uint32_t), &geometry->meshes[mesh_index].mesh_buffer);
    } // end looping meshes

    cgltf_free(data);
}

void destroy_geometry(VkContext *vk_context, Geometry *geometry) {
    for (Mesh &mesh: geometry->meshes) { destroy_mesh(vk_context, &mesh); }
}

void destroy_mesh(VkContext *vk_context, Mesh *mesh) { destroy_mesh_buffer(vk_context, &mesh->mesh_buffer); }
