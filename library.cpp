#include <windows.h>
#include <minhook.h>

#include <sdk.hpp>
#include "render.h"
#include "view.h"

auto __stdcall thread(void* module) -> void {
  MH_Initialize();
  LOG("Hello World");

  const auto direct = direct_hook::instance();
  direct->present_hook_.enable();
  direct->resize_hook_.enable();

  auto runtime = sdk::runtime::get();

  auto result = runtime.load_engine_version();
  LOG("Engine Version: {}, Version Number: {}", std::get<0>(result.value()), std::get<1>(result.value()));

  auto gobjects = runtime.load_gobjects();
  LOG("GObjects: {:#x}", (gobjects->size()));

  // FreeLibraryAndExitThread(reinterpret_cast<HMODULE>(module), 0);
}

auto __stdcall DllExit(void* module, const unsigned long reason, void*) -> bool {
  LOG("DllExit");
  
  direct_hook::wants_end_.store(true);
  while (!direct_hook::finished_last_render_.load()) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
  }

  LOG("Okay deleting hooks");
  
  auto direct = direct_hook::instance();
  direct->~direct_hook();
  MH_Uninitialize();

  return true;
}

[[maybe_unused]] auto __stdcall DllMain(void* module, const unsigned long reason, void*) -> bool {
  if (reason == DLL_PROCESS_ATTACH) CreateThread(nullptr, 0,  reinterpret_cast<LPTHREAD_START_ROUTINE>(thread), module, 0, nullptr);
  else if (reason == DLL_PROCESS_DETACH) return DllExit(module, reason, nullptr);
  return true;
}