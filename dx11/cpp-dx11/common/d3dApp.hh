//
// Created by zhong on 3/23/2022.
//

#ifndef DIDX11LAB_D3DAPP_HH
#define DIDX11LAB_D3DAPP_HH

#include <DirectXMath.h>
#include <d3d11_1.h>
#include <wrl/client.h>
#include <string>
#include <comdef.h>      // for _com_error
#include <iostream>

struct GLFWwindow;
namespace PD {

inline void DxTrace(const wchar_t *file, unsigned long line, HRESULT hr,
                    const wchar_t *proc) {
  _com_error err(hr);
  std::cerr << "file:" << file << "line:" << line << ", " << proc
            << " Error: " << (const char *)err.Description() << std::endl;
}

#define HR_RETURN(op)                                                           \
  if (FAILED(hr = (op))) {                                                     \
    assert(0);                                                                 \
    DxTrace(__FILEW__, __LINE__, hr, L#op);                                    \
    return hr;                                                                 \
  }

#define HR(op)                                                                 \
  if (FAILED(hr = (op))) {                                                     \
    assert(0);                                                                 \
    DxTrace(__FILEW__, __LINE__, hr, L#op); }

class D3DApp {
public:
  D3DApp();
  virtual ~D3DApp();
  HWND MainWnd() const;
  float AspectRatio() const;
  int Run();

  virtual bool Init();
  virtual void OnResize();
  virtual void UpdateScene(float dt) = 0;
  virtual void DrawScene() = 0;

protected:
  bool InitMainWindow();
  bool InitDirect3D();
  void CalcFrameStats();

  HWND mainWnd_;
  bool EnableMSAA_;
  uint32_t  MSAAQuality_;

  template <class T>
  using ComPtr = Microsoft::WRL::ComPtr<T>;

  ComPtr<ID3D11Device> pd3dDevice_;
  ComPtr<ID3D11DeviceContext> pd3dDeviceIMContext_;
  ComPtr<IDXGISwapChain> pSwapChain_;

  ComPtr<ID3D11Device1> pd3dDevice1_;
  ComPtr<ID3D11DeviceContext1> pd3dDeviceIMContext1_;
  ComPtr<IDXGISwapChain1> pSwapChain1_;

  ComPtr<ID3D11Texture2D> pDepthStencilBuffer_;
  ComPtr<ID3D11RenderTargetView> pRenderTargetView_;
  ComPtr<ID3D11DepthStencilView> pDepthStencilView_;
  D3D11_VIEWPORT ScreenViewport_;

  std::string WinTitle_ = "D3D11 Example";
public:
  int ClientWidth_,ClientHeight_;
  GLFWwindow* window_;

};
}

#endif // DIDX11LAB_D3DAPP_HH
