#include "render.h"

#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <Ultralight/Ultralight.h>
#include <AppCore/AppCore.h>
#include <AppCore/Platform.h>
ultralight::RefPtr<ultralight::Renderer> renderer;
ultralight::RefPtr<ultralight::View> view;

ID3D11Device* direct_hook::device_ = nullptr;
ID3D11DeviceContext* direct_hook::context_ = nullptr;
ID3D11RenderTargetView* direct_hook::render_target_view_ = nullptr;
std::shared_ptr<direct_hook> direct_hook::instance_ = nullptr;
std::once_flag direct_hook::init_flag;

ID3D11ShaderResourceView* ultralight_srv = nullptr;

auto direct_hook::instance() -> std::shared_ptr<direct_hook> {
  if (instance_ == nullptr) {
    instance_ = std::make_shared<direct_hook>();
  }

  return instance_;
}

auto CALLBACK direct_hook::present_trampoline(IDXGISwapChain* chain, UINT sync_interval, UINT flags) -> HRESULT {
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

    auto& platform = ultralight::Platform::instance();
    platform.set_file_system(ultralight::GetPlatformFileSystem("C:\\Users\\User\\Desktop\\asd\\"));
    platform.set_font_loader(ultralight::GetPlatformFontLoader());
    platform.set_logger(ultralight::GetDefaultLogger("ul.log"));

    ultralight::Config config{};
    platform.set_config(config);

    ultralight::ViewConfig view_config;
    view_config.initial_device_scale = 1.0;
    view_config.is_transparent = false;
    view_config.is_accelerated = false;

    renderer = ultralight::Renderer::Create();
    if (!renderer) {
      LOG("Failed to create Ultralight App instance");
      return;
    }
    
    view = renderer->CreateView(swap_chain_desc.BufferDesc.Width, swap_chain_desc.BufferDesc.Height, view_config, nullptr);
    if (!view) {
      LOG("Failed to create Ultralight view");
      return;
    }
    // view->LoadHTML("<h1>HELLO HAZEL</h1>");

    view->set_display_id(0);
    view->LoadURL("https://lezah.cloud");
    view->Focus();

    LOG("Created Ultralight context and initialized ImGui");
  });

  ImGui_ImplDX11_NewFrame();
  ImGui_ImplWin32_NewFrame();
  ImGui::NewFrame();

  renderer->Update();
  direct_hook::render_ultralight();

  ImGui::Render();
  ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
  if (render_target_view_) {
    direct_hook::context_->OMSetRenderTargets(1, &render_target_view_, nullptr);
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
  
  view->Resize(width, height);
  
  return S_OK;
}

void direct_hook::render_ultralight() {
  if (!renderer || !view) return;

  renderer->RefreshDisplay(0);
  renderer->Render();

  auto surface = view->surface();
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

  if (ultralight_srv) {
    ultralight_srv->Release();
    ultralight_srv = nullptr;
  }

  if (FAILED(direct_hook::device_->CreateShaderResourceView(texture, &srv_desc, &ultralight_srv))) {
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
  ImGui::SetNextWindowSize(ImVec2(view->width(), view->height()));
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
  ImGui::Image(reinterpret_cast<ImTextureID>(ultralight_srv), ImVec2(bitmap_image->width(), bitmap_image->height()));
  ImGui::End();
  ImGui::PopStyleVar(3);
}

direct_hook::direct_hook() {
  const auto window_handle = FindWindow("ImGui Example", nullptr);
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