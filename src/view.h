#ifndef view_h
#define view_h

#include <windows.h>
#include <variant>

#include <Ultralight/Ultralight.h>
#include <AppCore/AppCore.h>

#include "logger.h"

namespace ultraview {
  using mouse_event = ultralight::MouseEvent;
  using keyboard_event = ultralight::KeyEvent;
  using scroll_event = ultralight::ScrollEvent;

  using queued_event = std::variant<mouse_event, keyboard_event, scroll_event>;

  inline std::mutex event_mtx_;
  inline std::vector<queued_event> event_queue_;
  inline DWORD ul_thread_id_ = 0;

  static HCURSOR cursor = LoadCursor(nullptr, IDC_ARROW);
  static HHOOK mouse_hook = nullptr;
  static HHOOK keyboard_hook = nullptr;
  
  inline ultralight::RefPtr<ultralight::Renderer> renderer;
  inline ultralight::RefPtr<ultralight::View> view;
  inline ultralight::ViewListener* view_listener;
  inline HWND window_ = nullptr;

  extern ultralight::RefPtr<ultralight::Renderer> get_renderer();
  extern ultralight::RefPtr<ultralight::View> get_view(uint32_t width = 800, uint32_t height = 600);

  static void Dispatch(const queued_event& ev) {
    if (!ultraview::get_view()) return;

    std::visit([](auto&& e) {
      using T = std::decay_t<decltype(e)>;
      if constexpr (std::is_same_v<T, mouse_event>) {
        // LOG("Mouse event: type={}, x={}, y={}, button={}", (int)e.type, e.x, e.y, (int)e.button);
        ultraview::get_view()->FireMouseEvent(e);
      }
      else if constexpr (std::is_same_v<T, keyboard_event>) {
        // LOG("Key event: type={}, key={}, mods={}", (int)e.type, e.key_identifier.utf8().data(), e.modifiers);
        ultraview::get_view()->FireKeyEvent(e);
      }
      else if constexpr (std::is_same_v<T, scroll_event>) {
        // LOG("Scroll event: delta_x={}, delta_y={}", e.delta_x, e.delta_y);
        ultraview::get_view()->FireScrollEvent(e);
      }
    }, ev);
  }

  static void QueueEvent(const queued_event& ev) {
    std::lock_guard<std::mutex> lock(event_mtx_);
    event_queue_.push_back(ev);
  }

  static void HandleQueuedEvents() {
    std::vector<queued_event> local;
    {
      std::lock_guard lock(event_mtx_);
      local.swap(event_queue_);
    }
    for (auto& ev : local)
      Dispatch(ev);
  }

  static unsigned GetKeyMods() {
    using ultralight::KeyEvent;
    unsigned mods = 0;
    if (GetKeyState(VK_SHIFT) & 0x8000) mods |= KeyEvent::kMod_ShiftKey;
    if (GetKeyState(VK_CONTROL) & 0x8000) mods |= KeyEvent::kMod_CtrlKey;
    if (GetKeyState(VK_MENU) & 0x8000) mods |= KeyEvent::kMod_AltKey;
    if (GetKeyState(VK_LWIN) & 0x8000 || GetKeyState(VK_RWIN) & 0x8000) mods |= KeyEvent::kMod_MetaKey;
    return mods;
  }

  class view_listener : public ultralight::ViewListener {
  public:
    static view_listener& instance() {
      static view_listener instance;
      return instance;
    }
  
  public:
    void OnChangeCursor(ultralight::View*, ultralight::Cursor cur) override {
      cursor = LoadCursor(nullptr, 
        [](ultralight::Cursor c) -> LPCTSTR {
          using ultralight::Cursor;
          switch (c) {
            case Cursor::kCursor_Pointer: return IDC_ARROW;
            case Cursor::kCursor_Hand: return IDC_HAND;
            case Cursor::kCursor_IBeam: return IDC_IBEAM;
            case Cursor::kCursor_Cross: return IDC_CROSS;
            case Cursor::kCursor_Wait: return IDC_WAIT;
            case Cursor::kCursor_Help: return IDC_HELP;
            case Cursor::kCursor_EastResize: return IDC_SIZEWE;
            case Cursor::kCursor_NorthResize: return IDC_SIZENS;
            case Cursor::kCursor_NorthEastResize: return IDC_SIZENESW;
            case Cursor::kCursor_NorthWestResize: return IDC_SIZENWSE;
            case Cursor::kCursor_SouthResize: return IDC_SIZENS;
            case Cursor::kCursor_SouthEastResize: return IDC_SIZENWSE;
            case Cursor::kCursor_SouthWestResize: return IDC_SIZENESW;
            case Cursor::kCursor_WestResize: return IDC_SIZEWE;
            case Cursor::kCursor_NorthSouthResize: return IDC_SIZENS;
            case Cursor::kCursor_EastWestResize: return IDC_SIZEWE;
            case Cursor::kCursor_NorthEastSouthWestResize: return IDC_SIZENWSE;
            case Cursor::kCursor_NorthWestSouthEastResize: return IDC_SIZENESW;
            case Cursor::kCursor_ColumnResize: return IDC_SIZEWE;
            case Cursor::kCursor_RowResize: return IDC_SIZENS;
            case Cursor::kCursor_MiddlePanning: return IDC_SIZEALL;
            case Cursor::kCursor_EastPanning: return IDC_SIZEWE;
            case Cursor::kCursor_NorthPanning: return IDC_SIZENS;
            case Cursor::kCursor_NorthEastPanning: return IDC_SIZENESW;
            case Cursor::kCursor_NorthWestPanning: return IDC_SIZENWSE;
            case Cursor::kCursor_SouthPanning: return IDC_SIZENS;
            case Cursor::kCursor_SouthEastPanning: return IDC_SIZENWSE;
            case Cursor::kCursor_SouthWestPanning: return IDC_SIZENESW;
            case Cursor::kCursor_WestPanning: return IDC_SIZEWE;
            case Cursor::kCursor_Move: return IDC_SIZEALL;
            case Cursor::kCursor_VerticalText: return IDC_IBEAM;
            case Cursor::kCursor_Cell: return IDC_ARROW;
            case Cursor::kCursor_ContextMenu: return IDC_ARROW;
            case Cursor::kCursor_Alias: return IDC_ARROW;
            case Cursor::kCursor_Progress: return IDC_WAIT;
            case Cursor::kCursor_NoDrop: return IDC_NO;
            // case Cursor::kCursor_Copy: return IDC_COPY;
            case Cursor::kCursor_None: return IDC_ARROW;
            case Cursor::kCursor_NotAllowed: return IDC_NO;
            // case Cursor::kCursor_ZoomIn: return IDC_ZOOM;
            // case Cursor::kCursor_ZoomOut: return IDC_ZOOM;
            case Cursor::kCursor_Grab: return IDC_HAND;
            case Cursor::kCursor_Grabbing: return IDC_SIZEALL;
            case Cursor::kCursor_Custom: return IDC_ARROW; // Custom cursors would require additional

            default: return IDC_ARROW;
          }
        }(cur)
      );
      SetCursor(cursor);
    }
  };
}

#endif