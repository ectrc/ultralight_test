#include "misc.h"
#include "uobject.h"

#include "../../../include/logger.h"

namespace sdk {

auto fname::to_string() const -> std::string {
  static const auto string_library = uobject::find<uobject>("/Script/Engine.Default__KismetStringLibrary");
  _ASSERT(string_library && "Failed to find KismetStringLibrary");

  LOG("KismetStringLibrary: {:#x}", (uintptr_t)string_library);

  static const auto to_string_function = uobject::find<ufunction>("/Script/Engine.KismetStringLibrary.Conv_NameToString");
  _ASSERT(to_string_function && "Failed to find Conv_NameToString function");

  LOG("Conv_NameToString: {:#x}", (uintptr_t)to_string_function);

  LOG("ABOYUT TO ALLOC FSTRING");

  struct { fname in; fstring out; } params {*this};

  LOG("ABOUT TO CALL PROCESS_EVENT");
  string_library->process_event(to_string_function, &params);

  LOG("ABOUT TO LOG OUT FSTRING");
  LOG("Converted Name to String: {}", params.out.to_string());

  std::string result = params.out.to_string();

  LOG("ABOUT TO FREE FSTRING");
  params.out.free();
  return result;
}

}