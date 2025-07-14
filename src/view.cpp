#include "view.h"
#include "logger.h"

ultralight::RefPtr<ultralight::Renderer> ultraview::get_renderer() {
  if (!renderer) {
    ultraview::renderer = ultralight::Renderer::Create();
    if (!renderer) {
      LOG("Failed to create Ultralight Renderer");
    }
  }
  return renderer;
}

ultralight::RefPtr<ultralight::View> ultraview::get_view(uint32_t width, uint32_t height) {
  if (!view) {
    ultralight::ViewConfig view_config;
    view_config.initial_device_scale = 1.0;
    view_config.is_transparent = true;
    view_config.is_accelerated = false;

    ultraview::view = get_renderer()->CreateView(width, height, view_config, nullptr);
    if (!view) {
      LOG("Failed to create Ultralight View");
    }
  }
  return view;
}