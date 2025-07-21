#include "memory.h"

#include "finder.h"

namespace sdk {

auto fmemory::free(void* ptr) -> void {
  static const auto free_function = sdk::find_pattern(hat::compile_signature<"48 85 C9 74 2E 53 48 83 EC 20 48 8B D9 ??">());
  _ASSERT(free_function.has_value() && "Failed to find free function");

  typedef void (*free_t)(void*);
  const auto free = reinterpret_cast<free_t>(free_function.value());

  return free(ptr);
}

auto fmemory::realloc(void* ptr, size_t count, uint32_t alignment) -> void* {
  static const auto realloc_function = sdk::find_pattern(hat::compile_signature<"48 89 5C 24 ?? 48 89 74 24 ?? 57 48 83 EC 20 48 8B F1 41 8B D8 48 8B 0D ?? ?? ?? ??">());
  _ASSERT(realloc_function.has_value() && "Failed to find realloc function");

  typedef void* (*realloc_t)(void*, size_t, uint32_t);
  const auto realloc = reinterpret_cast<realloc_t>(realloc_function.value());

  return realloc(ptr, count, alignment);
}

auto fmemory::malloc(size_t count, uint32_t alignment) -> void* {
  return this->realloc(nullptr, count, alignment);
}

auto fmemory::memcpy(void* dest, const void* src, size_t size) -> void* {
  return std::memcpy(dest, src, size);
}

}