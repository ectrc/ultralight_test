#include "uobject.h"

auto sdk::uobject::process_event(class ufunction* function, void* params) -> void {
  static const auto process_event_function = sdk::find_pattern(hat::compile_signature<"40 55 56 57 41 54 41 55 41 56 41 57 48 81 EC ?? ?? ?? ?? 48 8D 6C 24 ?? 48 89 9D ?? ?? ?? ?? 48 8B 05 ?? ?? ?? ?? 48 33 C5 48 89 85 ?? ?? ?? ?? 8B 41 0C">());
  _ASSERT(process_event_function.has_value() && "Failed to find ProcessEvent function");

  typedef void(*process_event_t)(uobject*, ufunction*, void*);
  static const auto process_event = reinterpret_cast<process_event_t>(process_event_function.value());

  _ASSERT(function && "Function is null");
  
  return process_event(this, function, params);
}