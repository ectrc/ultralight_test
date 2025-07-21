#include "finder.h"

#include <windows.h>
#include "classes/uobject.h"

auto sdk::find_pattern(const hat::signature_view& signature, size_t offset) -> std::expected<uintptr_t, sdk::find_error> {
  static const hat::process::module process_module = hat::process::get_process_module();
  static const auto dos_header = reinterpret_cast<const IMAGE_DOS_HEADER*>(process_module.address());
  const auto headers = *reinterpret_cast<const IMAGE_NT_HEADERS*>(reinterpret_cast<uintptr_t>(dos_header) + dos_header->e_lfanew);
  const auto scan_result = hat::find_pattern(reinterpret_cast<const std::byte*>(dos_header), reinterpret_cast<const std::byte*>(dos_header) + headers.OptionalHeader.SizeOfCode, signature);
  
  if (!scan_result.has_result()) {
    return std::unexpected{sdk::find_error::pattern_not_found};
  }

  if (offset != 0) {
    return reinterpret_cast<uintptr_t>(scan_result.rel(offset));
  }
  
  return reinterpret_cast<uintptr_t>(scan_result.get());
}

auto sdk::find_pattern_all(const hat::signature_view& signature, size_t offset) -> std::vector<uintptr_t> {
  std::vector<uintptr_t> results{};

  static const hat::process::module process_module = hat::process::get_process_module();
  std::span<const std::byte> module_data = process_module.get_module_data();
  
  const std::byte* begin = module_data.data();
  const std::byte* end = module_data.data() + module_data.size();

  while (begin < end) {
    const auto scan_result = hat::find_pattern(begin, end, signature);
    if (!scan_result.has_result()) {
      break;
    }

    const std::byte* res_ptr = offset == 0 ? &(*scan_result.get()) : &(*scan_result.rel(offset));
    results.push_back(reinterpret_cast<uintptr_t>(res_ptr));
    begin = res_ptr + 1;
  }

  return results;
}

auto sdk::find_pattern_near(uintptr_t start, const hat::signature_view& signature, size_t offset, find_direction direction, size_t search_offset, uint32_t recursive_calls) -> std::expected<uintptr_t, sdk::find_error> {
  if (recursive_calls == 0) {
    return std::unexpected{sdk::find_error::pattern_not_found};
  }

  const std::byte* begin = reinterpret_cast<std::byte*>(start) - search_offset;
  const std::byte* end = reinterpret_cast<std::byte*>(start) + search_offset;

  const auto scan_result = hat::find_pattern(begin, end, signature);
  if (!scan_result.has_result()) {
    return sdk::find_pattern_near(start, signature, offset, direction, search_offset * 2, recursive_calls - 1);
  }

  if (offset != 0) {
    return reinterpret_cast<uintptr_t>(scan_result.rel(offset));
  }

  return reinterpret_cast<uintptr_t>(scan_result.get());
}

auto sdk::find_offset(const char* name) -> std::expected<uintptr_t, sdk::find_error> {
  return 0;
}