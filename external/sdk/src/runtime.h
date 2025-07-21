#ifndef runtime_h
#define runtime_h

#include <string>
#include <tuple>

#include "classes/uobject.h"
#include "finder.h"

namespace sdk {

class runtime {
public:
  static auto get() -> runtime& {
    static runtime instance;
    return instance;
  }

  auto load_engine_version() -> std::expected<std::tuple<std::string, uint64_t>, sdk::find_error>;
  auto load_gobjects() -> uobject_array*;

public:
  uint64_t engine_version_ = 0;
  std::string engine_version_string_;

  uobject_array* object_array_;
};

}

#endif