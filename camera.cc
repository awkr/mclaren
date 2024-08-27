#include "camera.h"

bool is_zero(float value) { return abs(value) < glm::epsilon<float>(); }

void create_camera(Camera *camera, const glm::vec3 &pos, const glm::vec3 &dir) {
    camera->position = pos;

    // calculate the rotation of the camera
    const glm::vec3 d = glm::normalize(dir);
    const glm::vec3 &forward = glm::vec3(0.0f, 0.0f, -1.0f); // -Z

    if (float rotation_angle = glm::acos(glm::dot(forward, d)); is_zero(rotation_angle)) {
        camera->rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    } else {
        const glm::vec3 &rotation_axis = glm::cross(forward, d);
        camera->rotation = glm::angleAxis(rotation_angle, glm::normalize(rotation_axis));
    }

    camera->is_dirty = true;
}

void destroy_camera(Camera *camera) {
    memset(camera, 0, sizeof(Camera)); // just wipe out the memory
}

void camera_set_position(Camera *camera, const glm::vec3 &position) {
    camera->position = position;
    camera->is_dirty = true;
}

void camera_forward(Camera *camera, float delta) {
    const glm::vec3 &forward = glm::normalize(glm::vec3(-camera->view_matrix[0][2], // 第 0 行第 2 列
                                                        -camera->view_matrix[1][2],
                                                        -camera->view_matrix[2][2]));
    camera->position += forward * delta;
    camera->is_dirty = true;
}

void camera_backward(Camera *camera, float delta) {
    const glm::vec3 &backward = glm::normalize(glm::vec3(camera->view_matrix[0][2],
                                                         camera->view_matrix[1][2],
                                                         camera->view_matrix[2][2]));
    camera->position += backward * delta;
    camera->is_dirty = true;
}

void camera_left(Camera *camera, float delta) {
    const glm::vec3 &left = glm::normalize(glm::vec3(-camera->view_matrix[0][0],
                                                     -camera->view_matrix[1][0],
                                                     -camera->view_matrix[2][0]));
    camera->position += left * delta;
    camera->is_dirty = true;
}

void camera_right(Camera *camera, float delta) {
    const glm::vec3 &right = glm::normalize(glm::vec3(camera->view_matrix[0][0],
                                                      camera->view_matrix[1][0],
                                                      camera->view_matrix[2][0]));
    camera->position += right * delta;
    camera->is_dirty = true;
}

void camera_up(Camera *camera, float delta) {
    const glm::vec3 &up = glm::normalize(glm::vec3(camera->view_matrix[0][1],
                                                   camera->view_matrix[1][1],
                                                   camera->view_matrix[2][1]));
    camera->position = camera->position + up * delta;
    camera->is_dirty = true;
}

void camera_down(Camera *camera, float delta) {
    const glm::vec3 &down = glm::normalize(glm::vec3(-camera->view_matrix[0][1],
                                                     -camera->view_matrix[1][1],
                                                     -camera->view_matrix[2][1]));
    camera->position += down * delta;
    camera->is_dirty = true;
}

void camera_pitch(Camera *camera, float delta_angles) {
    // rotate around the local x-axis
    camera->rotation = glm::angleAxis(glm::radians(delta_angles), camera->rotation * glm::vec3(1.0f, 0.0f, 0.0f)) *
                       camera->rotation;
    camera->is_dirty = true;
}

void camera_yaw(Camera *camera, float delta_angles) {
    // rotate around the world y-axis
    camera->rotation = glm::angleAxis(glm::radians(delta_angles), glm::vec3(0.0f, 1.0f, 0.0f)) * camera->rotation;
    camera->is_dirty = true;
}

void camera_update(Camera *camera) {
    if (!camera->is_dirty) { return; }

    // calculate the view matrix
    const auto &rotation = glm::mat4_cast(camera->rotation);
    const auto &translation = glm::translate(glm::mat4(1.0), -camera->position);

    auto view_matrix = rotation * translation;
    camera->view_matrix = view_matrix;

    camera->is_dirty = false;
}
