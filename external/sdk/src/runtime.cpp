#include "runtime.h"
#include "classes/misc.h"

#include <iostream>
#include <regex>

namespace sdk {

runtime::runtime() {
  static const auto global_objects = sdk::find_pattern(hat::compile_signature<"48 8B 05 ? ? ? ? 48 8B 0C C8 48 8D 04 D1">());
  _ASSERT(global_objects.has_value() && "Failed to find global objects pattern");

  object_array_ = reinterpret_cast<uobject_array*>(global_objects.value());
  _ASSERT(object_array_ && "Failed to initialize UObjectArray");
}

auto runtime::load_engine_version() -> std::expected<std::tuple<std::string, uint64_t>, sdk::find_error> {
  static const auto get_version_function = sdk::find_pattern(hat::compile_signature<"40 53 48 83 EC 20 48 8B D9 E8 ? ? ? ? 48 8B C8 41 B8 04 ? ? ? 48 8B">());
  _ASSERT(get_version_function.has_value() && "Failed to find EngineVersion offset");
  
  typedef fstring(*get_version_t)();
  const auto get_version = reinterpret_cast<get_version_t>(get_version_function.value());
  const auto string_ = get_version().to_string();

  static std::regex version_pattern{R"((\d+)\.(\d+)\.(\d+))"};
  std::smatch found;
  
  if (std::regex_search(string_, found, version_pattern)) {
    const auto major = std::stoull(found[1].str());
    const auto minor = std::stoull(found[2].str());
    const auto patch = std::stoull(found[3].str());
    
    this->engine_version_ = major * 100 + minor;
    this->engine_version_string_ = string_;

    return std::make_tuple(this->engine_version_string_, this->engine_version_);
  }

  return std::unexpected{sdk::find_error::pattern_not_found};
}

}