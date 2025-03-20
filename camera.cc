#include "camera.h"

bool is_zero(float value) { return abs(value) < glm::epsilon<float>(); }

void create_camera(Camera *camera, const glm::vec3 &pos, const glm::vec3 &dir) {
    camera->position = pos;

    // calculate the rotation of the camera
    const glm::vec3 &forward = glm::vec3(0.0f, 0.0f, -1.0f); // -Z
    calc_rotation_of_two_directions(forward, dir, camera->rotation);

    camera->is_dirty = true;
}

void destroy_camera(Camera *camera) {
    memset(camera, 0, sizeof(Camera)); // just wipe out the memory
}

void camera_set_position(Camera *camera, const glm::vec3 &position) {
    camera->position = position;
    camera->is_dirty = true;
}

const glm::vec3 &camera_get_position(const Camera *camera) noexcept { return camera->position; }

void camera_move_forward(Camera *camera, float delta) {
  camera->position += camera_forward_dir(*camera) * delta;
  camera->is_dirty = true;
}

void camera_move_backward(Camera *camera, float delta) {
    camera->position += camera_backward_dir(*camera) * delta;
    camera->is_dirty = true;
}

void camera_move_left(Camera *camera, float delta) {
    const glm::vec3 &left = glm::normalize(glm::vec3(-camera->view_matrix[0][0],
                                                     -camera->view_matrix[1][0],
                                                     -camera->view_matrix[2][0]));
    camera->position += left * delta;
    camera->is_dirty = true;
}

void camera_move_right(Camera *camera, float delta) {
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
    calc_view_matrix(camera->position, camera->rotation, camera->view_matrix);

    camera->is_dirty = false;
}

glm::vec3 camera_forward_dir(const Camera &camera) noexcept {
  return -camera_backward_dir(camera);
}

glm::vec3 camera_backward_dir(const Camera &camera) noexcept {
  /**
   * glm::mat4 为列主序，在内存中的实际布局为：
   * float data[16] = {
   *   m00, m10, m20, m30, // 第0列
   *   m01, m11, m21, m31, // 第1列
   *   m02, m12, m22, m32, // 第2列
   *   m03, m13, m23, m33  // 第3列
   * };
   */
  return glm::normalize(glm::vec3(camera.view_matrix[0][2], // 第 0 列第 2 行
                                  camera.view_matrix[1][2],
                                  camera.view_matrix[2][2]));
}

void calc_rotation_of_two_directions(const glm::vec3 &from, const glm::vec3 &to, glm::quat &rotation) {
  const glm::vec3 &forward = glm::normalize(from);
  const glm::vec3 &d = glm::normalize(to);

  if (float rotation_angle = glm::acos(glm::dot(forward, d)); is_zero(rotation_angle)) { // 计算旋转角度
    rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
  } else {
    const glm::vec3 &rotation_axis = glm::cross(forward, d); // 计算旋转轴
    rotation = glm::angleAxis(rotation_angle, glm::normalize(rotation_axis));
  }
}

void calc_view_matrix(const glm::vec3 &position, const glm::quat &rotation, glm::mat4 &view_matrix) {
  const glm::mat4 &rotation_matrix = glm::mat4_cast(rotation);
  const glm::mat4 &translation_matrix = glm::translate(glm::mat4(1.0f), position);

  view_matrix = glm::inverse(translation_matrix * rotation_matrix);
}
