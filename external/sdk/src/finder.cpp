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

auto sdk::find_offset(const char* name) -> std::expected<uintptr_t, sdk::find_error> {
  return 0;
}