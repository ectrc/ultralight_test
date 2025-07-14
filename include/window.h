#ifndef window_h
#define window_h

#include <windows.h>
#include <processthreadsapi.h>

namespace windows {
  struct handle_data {
    unsigned long process_id;
    HWND window_handle;
  };
  inline handle_data data{};

  auto main_window() -> HWND {
    if (data.window_handle != nullptr) {
      return data.window_handle;
    }

    EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL {
      GetWindowThreadProcessId(hwnd, &data.process_id);
      if (data.process_id == GetCurrentProcessId()) {
        data.window_handle = hwnd;
        return FALSE;
      }
      return TRUE;
    }, 0);

    if (data.window_handle == nullptr) {
      LOG("Failed to find main window handle");
    }

    return data.window_handle;
  }
}

#endif