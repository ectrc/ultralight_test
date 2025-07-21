#ifndef finder_h
#define finder_h

#include <cstdint>
#include <expected>

#include <libhat.hpp>

namespace sdk {
  enum class find_error {
    none,
    pattern_not_found,
    offset_not_found
  };

  auto find_pattern(const hat::signature_view& signature, size_t offset = 0) -> std::expected<uintptr_t, sdk::find_error>;
  auto find_offset(const char* name) -> std::expected<uintptr_t, sdk::find_error>;
}

#endif