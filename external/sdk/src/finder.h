#ifndef finder_h
#define finder_h

#include <cstdint>

namespace sdk {
  auto find_offset(const char* name) -> uintptr_t;
}

#endif