#include <windows.h>
#include <minhook.h>

#include "render.h"

auto __stdcall thread(void* module) -> void {
  MH_Initialize();
  LOG("Hello World");

  const auto direct = direct_hook::instance();
  direct->present_hook_.enable();
  direct->resize_hook_.enable();
}

[[maybe_unused]] auto __stdcall DllMain(void* module, const unsigned long reason, void*) -> bool {
  if (reason == DLL_PROCESS_ATTACH) CreateThread(nullptr, 0,  reinterpret_cast<LPTHREAD_START_ROUTINE>(thread), module, 0, nullptr);
  return true;
}