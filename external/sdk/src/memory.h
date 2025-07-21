#ifndef memory_h
#define memory_h

#include <cstdint>

namespace sdk {

class fmemory {
public:
  static auto get() -> fmemory& {
    static fmemory instance;
    return instance;
  }

public:
  auto free(void* ptr) -> void;
  auto realloc(void* ptr, size_t count, uint32_t alignment) -> void*;
  auto malloc(size_t count, uint32_t alignment = 0) -> void*;
  auto memcpy(void* dest, const void* src, size_t size) -> void*;
};

}

#endif