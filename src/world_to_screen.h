#ifndef world_to_screen_h
#define world_to_screen_h

struct vector_3 {
  float x, y, z;
};

struct vector_2 {
  float x, y;
};


struct mat_4 {
  float m[4][4];

  using vector_4 = struct {
    float x, y, z, w;
  };

  mat_4(const vector_4& first, const vector_4& second, const vector_4& third, const vector_4& fourth) {
    m[0][0] = first.x; m[0][1] = first.y; m[0][2] = first.z; m[0][3] = first.w;
    m[1][0] = second.x; m[1][1] = second.y; m[1][2] = second.z; m[1][3] = second.w;
    m[2][0] = third.x; m[2][1] = third.y; m[2][2] = third.z; m[2][3] = third.w;
    m[3][0] = fourth.x; m[3][1] = fourth.y; m[3][2] = fourth.z; m[3][3] = fourth.w;
  }
};

struct vector_4 {
  float x, y, z, w;

  vector_4 operator*(const mat_4& mat) const {
    return {
      x * mat.m[0][0] + y * mat.m[0][1] + z * mat.m[0][2] + w * mat.m[0][3],
      x * mat.m[1][0] + y * mat.m[1][1] + z * mat.m[1][2] + w * mat.m[1][3],
      x * mat.m[2][0] + y * mat.m[2][1] + z * mat.m[2][2] + w * mat.m[2][3],
      x * mat.m[3][0] + y * mat.m[3][1] + z * mat.m[3][2] + w * mat.m[3][3]
    };
  }

  vector_4(float x, float y, float z, float w) : x(x), y(y), z(z), w(w) {}
  vector_4(const vector_3& vec, float w = 1.0f) : x(vec.x), y(vec.y), z(vec.z), w(w) {}
};

namespace ericutil::world_to_screen {
  struct result {
    vector_2 screen;
    bool valid;
  };

  auto point(const vector_3& world, const vector_3& camera_position, const vector_3& camera_rotation, const mat_4& view_matrix, const mat_4& projection_matrix) -> result;
}

#endif