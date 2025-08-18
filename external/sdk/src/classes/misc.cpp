#include "misc.h"
#include "uobject.h"

#include "../../../include/logger.h"

namespace sdk {

auto fname::to_string() const -> std::string {
  static const auto to_string_string_ref = sdk::find_string_ref(L"MaterialQualityOverrides");
  _ASSERT(to_string_string_ref != 0 && "Failed to find string reference for fname::to_string");
  LOG("to_string_string_ref: index: {:#x}", (uintptr_t)to_string_string_ref);

  static const auto to_string_function = sdk::find_pattern_near(to_string_string_ref, hat::compile_signature<"E8 ?? ?? ?? ??">(), 1, sdk::find_direction::backward);
  _ASSERT(to_string_function.has_value() && "Failed to find function for fname::to_string");
  
  typedef void(*to_string_t)(fname*, fstring&);
  static const auto to_string = reinterpret_cast<to_string_t>(to_string_function.value());
  
  LOG("APPEND_STRING: index: {:#x}", (uintptr_t)to_string);

  fstring out{L"AAAA_"};
  to_string(const_cast<fname*>(this), out);

  LOG("fname::to_string: index: {}, number: {}", index_, number_);

  return out.to_string();
}

}

// 0x7ff634c483e0

//to_string_string_ref: index: 0x7ff631c95fe0
// APPEND_STRING: index: 0x7ff634c483e0
//