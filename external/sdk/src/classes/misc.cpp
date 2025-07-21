#include "misc.h"
#include "uobject.h"

namespace sdk {

auto fname::to_string() const -> std::string {
  static const auto string_library = uobject::find<uobject>("/Script/Engine.Default__KismetStringLibrary");
  _ASSERT(string_library && "Failed to find KismetStringLibrary");

  static const auto to_string_function = uobject::find<ufunction>("/Script/Engine.KismetStringLibrary.Conv_NameToString");
  _ASSERT(to_string_function && "Failed to find Conv_NameToString function");

  struct { fname in; fstring out; } params {*this};
  string_library->process_event(to_string_function, &params);

  std::string result = params.out.to_string();
  params.out.free();
  return result;
}

}