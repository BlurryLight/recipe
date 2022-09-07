//
// Created by zhong on 3/23/2022.
//

#ifndef DIDX11LAB_D3DAPP_HH
#define DIDX11LAB_D3DAPP_HH

#include <DirectXMath.h>
#include <d3d11_1.h>
#include <string>
#include <wrl/client.h>

struct GLFWwindow;
struct ImGuiIO;
template <class T> using ComPtr = Microsoft::WRL::ComPtr<T>;
namespace PD {

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
  virtual void glfw_keycallback(int key, int scancode, int action, int mods);
  virtual void ProcessInput(GLFWwindow *); // default do nothing
  /**
   * @brief 有关IMGUI的**内容**
   *
   */
  virtual void DrawImGUI();

protected:
  bool InitMainWindow();
  bool InitDirect3D();
  void CalcFrameStats();

  HWND mainWnd_;
  bool EnableMSAA_;
  uint32_t MSAAQuality_;

  ComPtr<ID3D11Device1> pd3dDevice_ = nullptr;
  ComPtr<ID3D11DeviceContext1> pd3dDeviceIMContext_ = nullptr;
  ComPtr<IDXGISwapChain1> pSwapChain_ = nullptr;

  ComPtr<ID3D11Texture2D> pDepthStencilBuffer_ = nullptr;
  ComPtr<ID3D11RenderTargetView> pRenderTargetView_ = nullptr;
  ComPtr<ID3D11DepthStencilView> pDepthStencilView_ = nullptr;
  D3D11_VIEWPORT ScreenViewport_;
  D3D_FEATURE_LEVEL FeatureLevel_;

  std::string WinTitle_ = "D3D11 Example";
  bool vsync_ = true;
  bool reverse_z_ = false;
  bool AllowMouseMove_ = true;
  ImGuiIO *imgui_io_ = nullptr;

public:
  int ClientWidth_ = -1, ClientHeight_ = -1;
  GLFWwindow *window_ = nullptr;
};
} // namespace PD

#endif // DIDX11LAB_D3DAPP_HH
