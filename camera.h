#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct Camera {
    glm::vec3 position;
    glm::quat rotation;

    glm::mat4 view_matrix; // 必须先调用 camera_update() 以更新 view_matrix
    bool is_dirty;
};

void create_camera(Camera *camera, const glm::vec3 &pos, const glm::vec3 &dir);

void destroy_camera(Camera *camera);

void camera_set_position(Camera *camera, const glm::vec3 &position);
const glm::vec3 &camera_get_position(const Camera *camera) noexcept;

void camera_forward(Camera *camera, float delta);

void camera_backward(Camera *camera, float delta);

void camera_left(Camera *camera, float delta);

void camera_right(Camera *camera, float delta);

void camera_up(Camera *camera, float delta);

void camera_down(Camera *camera, float delta);

void camera_pitch(Camera *camera, float delta_angles);

void camera_yaw(Camera *camera, float delta_angles);

void camera_update(Camera *camera);

glm::vec3 camera_backward_dir(const Camera &camera) noexcept;
