#include "world_to_screen.h"
#include "view.h"

auto ericutil::world_to_screen::point(const vector_3& world, const vector_3& camera_position, const vector_3& camera_rotation, const mat_4& view_matrix, const mat_4& projection_matrix) -> result {
  const auto view_space = vector_4{world} * view_matrix;
  const auto clip_space = view_space * projection_matrix;

  if (clip_space.w <= 0.0f) {
    LOG("Invalid clip space w value: {}", clip_space.w);
    return {vector_2{0.0f, 0.0f}, false};
  }

  const auto normalised_device_coordinates = vector_3{
    clip_space.x / clip_space.w,
    clip_space.y / clip_space.w,
    clip_space.z / clip_space.w
  };
  if (normalised_device_coordinates.x < -1.0f || normalised_device_coordinates.x > 1.0f ||
      normalised_device_coordinates.y < -1.0f || normalised_device_coordinates.y > 1.0f ||
      normalised_device_coordinates.z < -1.0f || normalised_device_coordinates.z > 1.0f) {
    LOG("Normalised device coordinates out of bounds: ({}, {}, {})", normalised_device_coordinates.x, normalised_device_coordinates.y, normalised_device_coordinates.z);
    return {vector_2{0.0f, 0.0f}, false};
  }

  const auto screen_x = (normalised_device_coordinates.x + 1.0f) * 0.5f * ultraview::view->width();
  const auto screen_y = (1.0f - (normalised_device_coordinates.y + 1.0f) * 0.5f) * ultraview::view->height();

  return {vector_2{screen_x, screen_y}, true};
}