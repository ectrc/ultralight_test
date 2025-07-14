#ifndef render_h
#define render_h

#include <memory>
#include <d3d11.h>


#include <hook.h>

typedef HRESULT (CALLBACK *present_t)(IDXGISwapChain* chain, UINT sync_interval, UINT flags);
typedef HRESULT (CALLBACK *resize_t)(IDXGISwapChain* chain, UINT buffer, UINT width, UINT height, DXGI_FORMAT format, UINT flags);

class direct_hook {
public:
  direct_hook();
  ~direct_hook();
  static auto instance() -> std::shared_ptr<direct_hook>;
public:
  base_hook<present_t> present_hook_;
  static auto CALLBACK present_trampoline(IDXGISwapChain* chain, UINT sync_interval, UINT flags) -> HRESULT;

  base_hook<resize_t> resize_hook_;
  static auto CALLBACK resize_trampoline(IDXGISwapChain* chain, UINT buffer, UINT width, UINT height, DXGI_FORMAT format, UINT flags) -> HRESULT;
private:
  static std::shared_ptr<direct_hook> instance_;
public:
  static ID3D11Device* device_;
  static ID3D11DeviceContext* context_;
  static ID3D11RenderTargetView* render_target_view_;

  static void render_ultralight();
  static std::once_flag init_flag;
};

#endif