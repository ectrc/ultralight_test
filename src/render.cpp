#include "render.h"
#include "window.h"
#include "logger.h"
#include "view.h"
#include "world_to_screen.h"

#include <windowsx.h>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <Ultralight/Ultralight.h>
#include <AppCore/AppCore.h>
#include <AppCore/Platform.h>
#include <Ultralight/String.h>
#include <Ultralight/KeyEvent.h>
#include <Ultralight/MouseEvent.h>
#include <Ultralight/ScrollEvent.h>

process_t direct_hook::process_original_ = nullptr;
ID3D11Device* direct_hook::device_ = nullptr;
ID3D11DeviceContext* direct_hook::context_ = nullptr;
ID3D11RenderTargetView* direct_hook::render_target_view_ = nullptr;
ID3D11ShaderResourceView* direct_hook::ultralight_srv_ = nullptr;
std::shared_ptr<direct_hook> direct_hook::instance_ = nullptr;
std::once_flag direct_hook::init_flag;

LRESULT CALLBACK direct_hook::process_trampoline(HWND window, UINT message, WPARAM wide_param, LPARAM long_param) {
  if (message == WM_KEYUP && (wide_param == VK_INSERT || wide_param == VK_END)) {
    direct_hook::show_menu.store(!direct_hook::show_menu.load());
  }

  if (direct_hook::show_menu.load() && ultraview::view.get() != nullptr) {
    switch (message) {
      case WM_MOUSEMOVE:
      case WM_LBUTTONDOWN: 
      case WM_RBUTTONDOWN:
      case WM_MBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_RBUTTONUP:
      case WM_MBUTTONUP: {
        ultraview::queued_event event = ultraview::mouse_event{};
        auto& mouse_event = std::get<ultraview::mouse_event>(event);
        mouse_event.x = GET_X_LPARAM(long_param);
        mouse_event.y = GET_Y_LPARAM(long_param);
        
        if (message == WM_MOUSEMOVE) {
          mouse_event.type = ultralight::MouseEvent::kType_MouseMoved;
        } else if (message == WM_LBUTTONDOWN) {
          mouse_event.type = ultralight::MouseEvent::kType_MouseDown;
          mouse_event.button = ultralight::MouseEvent::kButton_Left;
        } else if (message == WM_RBUTTONDOWN) {
          mouse_event.type = ultralight::MouseEvent::kType_MouseDown;
          mouse_event.button = ultralight::MouseEvent::kButton_Right;
        } else if (message == WM_MBUTTONDOWN) {
          mouse_event.type = ultralight::MouseEvent::kType_MouseDown;
          mouse_event.button = ultralight::MouseEvent::kButton_Middle;
        } else if (message == WM_LBUTTONUP) {
          mouse_event.type = ultralight::MouseEvent::kType_MouseUp;
          mouse_event.button = ultralight::MouseEvent::kButton_Left;
        } else if (message == WM_RBUTTONUP) {
          mouse_event.type = ultralight::MouseEvent::kType_MouseUp;
          mouse_event.button = ultralight::MouseEvent::kButton_Right;
        } else if (message == WM_MBUTTONUP) {
          mouse_event.type = ultralight::MouseEvent::kType_MouseUp;
          mouse_event.button = ultralight::MouseEvent::kButton_Middle;
        }
        
        ultraview::QueueEvent(event);
        return true;
      }
      case WM_MOUSEWHEEL: {
        ultraview::queued_event event = ultraview::scroll_event{};

        auto& scroll_event = std::get<ultraview::scroll_event>(event);
        scroll_event.delta_y = GET_WHEEL_DELTA_WPARAM(wide_param) / WHEEL_DELTA * 32;

        ultraview::QueueEvent(scroll_event);
        return true;
      }
      case WM_KEYDOWN:
      case WM_KEYUP:
      case WM_SYSKEYUP:
      case WM_SYSKEYDOWN: {
        ultraview::queued_event event = ultraview::keyboard_event{};
        auto& key_event = std::get<ultraview::keyboard_event>(event);

        key_event.type = (message == WM_SYSKEYDOWN || message == WM_SYSKEYDOWN) 
            ? ultralight::KeyEvent::kType_KeyDown 
            : ultralight::KeyEvent::kType_KeyUp;
        key_event.virtual_key_code = static_cast<int>(wide_param);
        key_event.native_key_code = (static_cast<int>(long_param) >> 16) & 0xFF;
        key_event.modifiers = ultraview::GetKeyMods();
        key_event.is_system_key = (message == WM_SYSKEYDOWN || message == WM_SYSKEYUP);

        ultralight::String key_identifier_result;
        ultralight::GetKeyIdentifierFromVirtualKeyCode(key_event.virtual_key_code, key_identifier_result);
        key_event.key_identifier = key_identifier_result;

        ultraview::QueueEvent(event);
        return true;
      }

      case WM_CHAR: {
        ultraview::queued_event event = ultraview::keyboard_event{};
        auto& key_event = std::get<ultraview::keyboard_event>(event);

        wchar_t ch = static_cast<wchar_t>(wide_param);
        char utf8_char[5] = {0};
        int len = WideCharToMultiByte(CP_UTF8, 0, &ch, 1, utf8_char, sizeof(utf8_char), nullptr, nullptr);
        
        key_event.text = ultralight::String(utf8_char, len);
        key_event.unmodified_text = key_event.text;
        key_event.modifiers = ultraview::GetKeyMods();
        key_event.type = ultralight::KeyEvent::kType_Char;

        ultraview::QueueEvent(event);
        return true;
      }
    }

    return CallWindowProc(direct_hook::process_original_, window, message, wide_param, long_param);
  }

  ImGui_ImplWin32_WndProcHandler(window, message, wide_param, long_param);
  return CallWindowProc(direct_hook::process_original_, window, message, wide_param, long_param);
}

auto direct_hook::instance() -> std::shared_ptr<direct_hook> {
  if (instance_ == nullptr) {
    instance_ = std::make_shared<direct_hook>();
  }

  return instance_;
}

auto CALLBACK direct_hook::present_trampoline(IDXGISwapChain* chain, UINT sync_interval, UINT flags) -> HRESULT {
  if (finished_last_render_.load()) {
    return instance_->present_hook_.original()(chain, sync_interval, flags);
  }

  std::call_once(init_flag, [&]() {
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange | ImGuiConfigFlags_NavEnableKeyboard | ImGuiConfigFlags_NavEnableGamepad;

    chain->GetDevice(__uuidof(ID3D11Device), reinterpret_cast<void**>(&device_));
    if (device_ == nullptr) {
      LOG("Failed to get D3D11 device from swap chain");
      return;
    }

    device_->GetImmediateContext(&context_);
    if (context_ == nullptr) {
      LOG("Failed to get D3D11 device context");
      return;
    }

    ID3D11Texture2D* back_buffer = nullptr;
    if (FAILED(chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)))) {
      LOG("Failed to get back buffer from swap chain");
      return;
    }

    device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_view_);
    if (render_target_view_ == nullptr) {
      LOG("Failed to create render target view from back buffer");
      back_buffer->Release();
      return;
    }
    back_buffer->Release();

    DXGI_SWAP_CHAIN_DESC swap_chain_desc = { };
    chain->GetDesc(&swap_chain_desc);
    ImGui_ImplWin32_Init(swap_chain_desc.OutputWindow);
    ImGui_ImplDX11_Init(device_, context_);

    ultraview::window_ = swap_chain_desc.OutputWindow;
    process_original_ = (WNDPROC)SetWindowLongPtr(swap_chain_desc.OutputWindow, GWLP_WNDPROC, (LONG_PTR)process_trampoline);

    auto& platform = ultralight::Platform::instance();
    platform.set_file_system(ultralight::GetPlatformFileSystem("C:\\Users\\User\\Desktop\\asd\\"));
    platform.set_font_loader(ultralight::GetPlatformFontLoader());
    platform.set_logger(ultralight::GetDefaultLogger("ul.log"));

    ultralight::Config config{};
    config.user_stylesheet = "html, body { background: transparent; }";
    platform.set_config(config);
    
    const auto view = ultraview::get_view(swap_chain_desc.BufferDesc.Width, swap_chain_desc.BufferDesc.Height);
    view->set_display_id(0);
    view->set_view_listener(&ultraview::view_listener::instance());
    // view->LoadURL("https://retrac.site");
    // view->LoadURL("https://google.com");
    view->LoadHTML("<h1 style='display:flex;flex-direction:column;'>HELLO BUKLLLY <a style='color:black;' href='https://retrac.site'>GO TO RETRAC SITE</a><a style='color:black;' href='https://google.com'>GO TO google SITE</a> <img src='https://media.discordapp.net/attachments/1346186915040329789/1394067962444513311/plytix-682443bc51cf6986fd8fad18-png.png?ex=68757676&is=687424f6&hm=9a582e40c0e668a603da949850fe163306845841d345b74a8647951f9378272b&=&format=webp&quality=lossless' /></h1>");
    view->Focus();

    ultraview::ul_thread_id_ = GetCurrentThreadId();
  });
  ultraview::HandleQueuedEvents();

  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  if (show_menu.load()) {
    ultraview::get_renderer()->Update();
    direct_hook::render_ultralight();
  }

  /////////////////////////////////////// FAKE CAMERA TO TEST WORLD TO SCREEN ///////////////////////////////////////

  static auto world_position = vector_3{3.0f, 0.0f, 5.0f};
  world_position.z -= 0.1f; // Adjust to ensure it's in front of the camera

  const auto camera_rotation = vector_3{0.0f, 0.0f, 0.0f};
  const auto camera_position = vector_3{0.0f, 0.0f, 0.0f};

  const auto view_matrix = mat_4{ // identity if no camera transform
    {1, 0, 0, 0},
    {0, 1, 0, 0},
    {0, 0, 1, 0},
    {0, 0, 0, 1}
  };

  const auto projection_matrix = mat_4{
    {1.0f, 0.0f,  0.0f, 0.0f},
    {0.0f, 1.0f,  0.0f, 0.0f},
    {0.0f, 0.0f, -1.0f, -1.0f},
    {0.0f, 0.0f, -2.0f,  0.0f}
  };

  const auto screen_result = ericutil::world_to_screen::point(world_position, camera_position, camera_rotation, view_matrix, projection_matrix);
  if (screen_result.valid) {
    ImGui::GetBackgroundDrawList()->AddRectFilled(
      ImVec2(screen_result.screen.x - 5, screen_result.screen.y - 5),
      ImVec2(screen_result.screen.x + 5, screen_result.screen.y + 5),
      IM_COL32(255, 0, 0, 255)
    );
  }

  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////


  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  if (render_target_view_) {
    direct_hook::context_->OMSetRenderTargets(1, &render_target_view_, nullptr);
  }

  if (wants_end_) {
    if (render_target_view_) {
      render_target_view_->Release();
      render_target_view_ = nullptr;

      LOG("Render target view released");
    }

    if (context_) {
      context_->Release();
      context_ = nullptr;

      LOG("D3D11 context released");
    }

    if (device_) {
      device_->Release();
      device_ = nullptr;

      LOG("D3D11 device released");
    }

    if (ultralight_srv_) {
      ultralight_srv_->Release();
      ultralight_srv_ = nullptr;

      LOG("Ultralight shader resource view released");
    }

    if (ultraview::window_) {
      SetWindowLongPtr(ultraview::window_, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(direct_hook::process_original_));
      LOG("Window procedure unhooked");
    }

    // ultraview::renderer = nullptr;
    // ultraview::view = nullptr;
    // ultraview::view_listener = nullptr;
    
    finished_last_render_ = true;
  }

  return instance_->present_hook_.original()(chain, sync_interval, flags);
}

auto CALLBACK direct_hook::resize_trampoline(IDXGISwapChain* chain, UINT buffer, UINT width, UINT height, DXGI_FORMAT format, UINT flags) -> HRESULT {
  if (render_target_view_) {
    render_target_view_->Release();
    render_target_view_ = nullptr;
  }

  if (context_) {
    context_->OMSetRenderTargets(0, nullptr, nullptr);
  }

  if (HRESULT result = instance_->resize_hook_.original()(chain, buffer, width, height, format, flags); FAILED(result)) {
    LOG("Failed to resize swap chain: {}", result);
    return result;
  }

  ID3D11Texture2D* back_buffer = nullptr;
  if (FAILED(chain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&back_buffer)))) {
    LOG("Failed to get back buffer from swap chain after resize");
    return E_FAIL;
  }

  if (FAILED(device_->CreateRenderTargetView(back_buffer, nullptr, &render_target_view_))) {
    LOG("Failed to create render target view after resize");
    back_buffer->Release();
    return E_FAIL;
  }
  back_buffer->Release();
  
  D3D11_VIEWPORT viewport;
  viewport.Width = static_cast<float>(width);
  viewport.Height = static_cast<float>(height);
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  viewport.TopLeftX = 0.0f;
  viewport.TopLeftY = 0.0f;
  context_->RSSetViewports(1, &viewport);
  
  ultraview::get_view()->Resize(width, height);
  
  return S_OK;
}

void direct_hook::render_ultralight() {
  if (!ultraview::get_renderer() || !ultraview::get_view()) return;

  ultraview::get_renderer()->RefreshDisplay(0);
  ultraview::get_renderer()->Render();

  auto surface = ultraview::get_view()->surface();
  if (surface == nullptr) {
    LOG("Failed to get surface from Ultralight view");
    return;
  }

  auto bitmap_surface = static_cast<ultralight::BitmapSurface*>(surface);
  if (!bitmap_surface) {
    LOG("Failed to cast surface to BitmapSurface");
    return;
  }

  const auto bitmap_image = bitmap_surface->bitmap();
  if (!bitmap_image || bitmap_image->width() == 0 || bitmap_image->height() == 0) {
    LOG("Failed to lock pixels on BitmapSurface");
    return;
  }

  D3D11_TEXTURE2D_DESC desc = {};
  desc.Width = bitmap_image->width();
  desc.Height = bitmap_image->height();
  desc.MipLevels = 1;
  desc.ArraySize = 1;
  desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
  desc.SampleDesc.Count = 1;
  desc.Usage = D3D11_USAGE_DYNAMIC;
  desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  ID3D11Texture2D* texture = nullptr;
  if (FAILED(direct_hook::device_->CreateTexture2D(&desc, nullptr, &texture))) {
    LOG("Failed to create D3D11 texture");
    return;
  }

  D3D11_MAPPED_SUBRESOURCE mapped_resource = {};
  if (FAILED(direct_hook::context_->Map(texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped_resource))) {
    LOG("Failed to map D3D11 texture");
    texture->Release();
    return;
  }

  const auto pixels = bitmap_image->LockPixels();
  if (!pixels) {
    LOG("Failed to lock pixels on BitmapSurface");
    direct_hook::context_->Unmap(texture, 0);
    texture->Release();
    return;
  }

  for (int y = 0; y < desc.Height; ++y) {
    memcpy(static_cast<uint8_t*>(mapped_resource.pData) + mapped_resource.RowPitch * y,
           static_cast<const uint8_t*>(pixels) + bitmap_image->row_bytes() * y,
           bitmap_image->row_bytes());
  }
  bitmap_image->UnlockPixels();
  
  direct_hook::context_->Unmap(texture, 0);

  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Format = desc.Format;
  srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MostDetailedMip = 0;
  srv_desc.Texture2D.MipLevels = 1;

  if (ultralight_srv_) {
    ultralight_srv_->Release();
    ultralight_srv_ = nullptr;
  }

  if (FAILED(direct_hook::device_->CreateShaderResourceView(texture, &srv_desc, &ultralight_srv_))) {
    LOG("Failed to create shader resource view");
    texture->Release();
    return;
  }

  texture->Release();
  direct_hook::context_->OMSetRenderTargets(1, &direct_hook::render_target_view_, nullptr);

  D3D11_VIEWPORT viewport = {};
  viewport.Width = static_cast<float>(bitmap_image->width());
  viewport.Height = static_cast<float>(bitmap_image->height());
  viewport.MinDepth = 0.0f;
  viewport.MaxDepth = 1.0f;
  viewport.TopLeftX = 0.0f;
  viewport.TopLeftY = 0.0f;
  direct_hook::context_->RSSetViewports(1, &viewport);

  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(ultraview::get_view()->width(), ultraview::get_view()->height()));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::Begin("Overlay", nullptr,
    ImGuiWindowFlags_NoDecoration |
    ImGuiWindowFlags_NoInputs |
    ImGuiWindowFlags_NoBackground |
    ImGuiWindowFlags_NoBringToFrontOnFocus |
    ImGuiWindowFlags_NoTitleBar |
    ImGuiWindowFlags_NoResize |
    ImGuiWindowFlags_NoMove);
  ImGui::Image(reinterpret_cast<ImTextureID>(ultralight_srv_), ImVec2(bitmap_image->width(), bitmap_image->height()));
  ImGui::End();
  ImGui::PopStyleVar(3);
}

direct_hook::direct_hook() {
  const auto window_handle = windows::main_window();
  if (window_handle == nullptr) {
    LOG("Failed to get foreground window handle");
    return;
  }

  constexpr D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_10_1, D3D_FEATURE_LEVEL_11_0 };
  D3D_FEATURE_LEVEL obtained_level;

  DXGI_SWAP_CHAIN_DESC swap_chain_description = { };
  ZeroMemory(&swap_chain_description, sizeof(swap_chain_description));
  swap_chain_description.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swap_chain_description.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
  swap_chain_description.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
  swap_chain_description.BufferDesc.Width = 1000;
  swap_chain_description.BufferDesc.Height = 1000;
  swap_chain_description.BufferDesc.RefreshRate.Numerator = 60;
  swap_chain_description.BufferDesc.RefreshRate.Denominator = 1;
  swap_chain_description.BufferCount = 1;
  swap_chain_description.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swap_chain_description.SampleDesc.Count = 1;
  swap_chain_description.SampleDesc.Quality = 0;
  swap_chain_description.OutputWindow = window_handle;
  swap_chain_description.Windowed = true;
  swap_chain_description.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
  swap_chain_description.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

  IDXGISwapChain* swap_chain = nullptr;
  if (HRESULT result = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, sizeof(levels) / sizeof(D3D_FEATURE_LEVEL), D3D11_SDK_VERSION, &swap_chain_description, &swap_chain, &device_, &obtained_level, &context_); FAILED(result)) {
    LOG("Failed to create D3D11 device and swap chain, error code: {}", result);
    return;
  }

  const auto swap_chain_vtable = *reinterpret_cast<uintptr_t**>(swap_chain);

  this->present_hook_ = base_hook<present_t>{
    "directx present_hook",
    reinterpret_cast<std::byte*>(swap_chain_vtable[8]),
    this->present_trampoline
  };

  this->resize_hook_ = base_hook<resize_t>{
    "directx resize_hook",
    reinterpret_cast<std::byte*>(swap_chain_vtable[13]),
    this->resize_trampoline
  };

  device_->Release();
  context_->Release();
  swap_chain->Release();

  if (this->present_hook_.install()) {
    LOG("DirectX present hook installed successfully");
  } else {
    LOG("Failed to install DirectX present hook");
    return;
  }

  if (this->resize_hook_.install()) {
    LOG("DirectX resize hook installed successfully");
  } else {
    LOG("Failed to install DirectX resize hook");
    return;
  }
}

direct_hook::~direct_hook() {
  if (this->present_hook_.disable()) {
    LOG("DirectX present hook disabled successfully");
  } else {
    LOG("Failed to disable DirectX present hook");
  }

  if (this->present_hook_.uninstall()) {
    LOG("DirectX present hook uninstalled successfully");
  } else {
    LOG("Failed to uninstall DirectX present hook");
  }

  if (context_) {
    context_->Release();
    context_ = nullptr;
  }

  if (device_) {
    device_->Release();
    device_ = nullptr;
  }

  LOG("DirectX present hook destroyed");
}