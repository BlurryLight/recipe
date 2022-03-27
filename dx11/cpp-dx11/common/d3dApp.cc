//
// Created by zhong on 3/23/2022.
//
#include "d3dApp.hh"
#include "d3dUtils.hh"
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32 1 //暴露出win32相关的原生接口
// glfw should be after windows.h
// it will undefine foreirn APIENTRY to avoid redefine it
#include <GLFW/glfw3native.h> //https://github.com/glfw/glfw/issues/1062
#include <assert.h>
#include <imgui/imgui.h>
#include <imgui/imgui_impl_dx11.h>
#include <imgui/imgui_impl_glfw.h>
// #include <imgui/imgui_impl_win32.h>

using namespace PD;
static D3DApp *g_app = nullptr;
static void OnResizeFrame(GLFWwindow *window, int width, int height) {
  assert(g_app);
  g_app->ClientHeight_ = height;
  g_app->ClientWidth_ = width;
  g_app->OnResize();
};

D3DApp::D3DApp()
    : mainWnd_(nullptr), EnableMSAA_(true), MSAAQuality_(0), ClientWidth_(1280),
      ClientHeight_(720) {
  memset(&ScreenViewport_, 0, sizeof ScreenViewport_);
  g_app = this;
}
D3DApp::~D3DApp() {
  if (pd3dDeviceIMContext_) {
    pd3dDeviceIMContext_->ClearState();
  }
}
HWND D3DApp::MainWnd() const {
  assert(mainWnd_);
  return mainWnd_;
}
float D3DApp::AspectRatio() const {
  assert(ClientHeight_ > 0);
  return (float)(ClientWidth_) / (float)(ClientHeight_);
}

void D3DApp::DrawImGUI() {
  {
    ImGui::Begin("Hello, world!");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)",
                1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
    ImGui::End();
  }
}
int D3DApp::Run() {
  while (!glfwWindowShouldClose(window_)) {
    // Deal with ImGUI
    ImGui_ImplDX11_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    DrawImGUI();
    ImGui::Render();
    DrawScene();
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    HRESULT hr;
    HR(pSwapChain_->Present(0, 0));

    //轮询并处理事件
    glfwPollEvents();
  }

  //使用GLFW完成操作后，通常是在应用程序退出之前，需要终止GLFW
  ImGui_ImplDX11_Shutdown();
  // ImGui_ImplWin32_Shutdown();
  ImGui::DestroyContext();
  glfwTerminate();
  return 0;
}
bool D3DApp::Init() {
  auto res = InitMainWindow() && InitDirect3D();
  return res;
}
void D3DApp::OnResize() {
  //  std::cout << "On Resize Height: " << ClientHeight_ << " Width:  " <<
  //  ClientWidth_ << std::endl;
  HRESULT hr;
  assert(pd3dDeviceIMContext_);
  assert(pd3dDevice_);
  assert(pSwapChain_);

  pRenderTargetView_.Reset();
  pDepthStencilView_.Reset();
  pDepthStencilBuffer_.Reset();
  ComPtr<ID3D11Texture2D> backBuffer;
  HR(pSwapChain_->ResizeBuffers(1, ClientWidth_, ClientHeight_,
                                DXGI_FORMAT_R8G8B8A8_UNORM, 0));
  HR(pSwapChain_->GetBuffer(
      0, __uuidof(ID3D11Texture2D),
      reinterpret_cast<void **>(backBuffer.GetAddressOf())));
  HR(pd3dDevice_->CreateRenderTargetView(backBuffer.Get(), nullptr,
                                         pRenderTargetView_.GetAddressOf()));

  backBuffer.Reset();
  D3D11_TEXTURE2D_DESC depthStencilDesc;
  depthStencilDesc.Width = ClientWidth_;
  depthStencilDesc.Height = ClientHeight_;
  depthStencilDesc.MipLevels = 1;
  depthStencilDesc.ArraySize = 1;
  depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
  depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
  depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
  depthStencilDesc.CPUAccessFlags = 0;
  depthStencilDesc.MiscFlags = 0;
  depthStencilDesc.SampleDesc.Count = EnableMSAA_ ? 4 : 1;
  depthStencilDesc.SampleDesc.Quality = 0;

  // 创建深度缓冲区以及深度模板视图
  HR(pd3dDevice_->CreateTexture2D(&depthStencilDesc, nullptr,
                                  pDepthStencilBuffer_.GetAddressOf()));
  HR(pd3dDevice_->CreateDepthStencilView(pDepthStencilBuffer_.Get(), nullptr,
                                         pDepthStencilView_.GetAddressOf()));

  // 将渲染目标视图和深度/模板缓冲区结合到管线
  pd3dDeviceIMContext_->OMSetRenderTargets(1, pRenderTargetView_.GetAddressOf(),
                                           pDepthStencilView_.Get());

  // 设置视口变换
  ScreenViewport_.TopLeftX = 0;
  ScreenViewport_.TopLeftY = 0;
  ScreenViewport_.Width = static_cast<float>(ClientWidth_);
  ScreenViewport_.Height = static_cast<float>(ClientHeight_);
  ScreenViewport_.MinDepth = 0.0f;
  ScreenViewport_.MaxDepth = 1.0f;

  pd3dDeviceIMContext_->RSSetViewports(1, &ScreenViewport_);
}
bool D3DApp::InitMainWindow() {
  if (!glfwInit())
    return false;

  //创建窗口
  GLFWwindow *window = glfwCreateWindow(ClientWidth_, ClientHeight_,
                                        this->WinTitle_.c_str(), NULL, NULL);
  assert(window);
  if (!window) {
    glfwTerminate();
    return false;
  }
  window_ = window;
  this->mainWnd_ = glfwGetWin32Window(window);
  glfwSetFramebufferSizeCallback(window, OnResizeFrame);
  //  glfwSetWindowSizeCallback(window,OnResizeFrame);

  // IMGUI INIT
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  // ImGui_ImplWin32_Init(mainWnd_);
  ImGui_ImplGlfw_InitForOther(window, true);
  return true;
}
bool D3DApp::InitDirect3D() {
  HRESULT hr = S_OK;

  //列出所有考虑的FeatureLevel：
  D3D_FEATURE_LEVEL featureLevels[] = {
      D3D_FEATURE_LEVEL_11_0,
      D3D_FEATURE_LEVEL_10_1,
      D3D_FEATURE_LEVEL_10_0,
  };

  //列出所有考虑的驱动类型（越靠前越优先考虑）
  D3D_DRIVER_TYPE driverTypes[] = {
      D3D_DRIVER_TYPE_HARDWARE,
      D3D_DRIVER_TYPE_WARP,
      D3D_DRIVER_TYPE_REFERENCE,
  };

  //创建SwapChain的描述结构体
  DXGI_SWAP_CHAIN_DESC swapchainDescription;
  ZeroMemory(&swapchainDescription, sizeof(swapchainDescription));
  swapchainDescription.BufferCount = 1;
  swapchainDescription.BufferDesc.Width = ClientWidth_;
  swapchainDescription.BufferDesc.Height = ClientHeight_;
  swapchainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapchainDescription.BufferDesc.RefreshRate.Numerator = 60;
  swapchainDescription.BufferDesc.RefreshRate.Denominator = 1;
  swapchainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchainDescription.OutputWindow = MainWnd();
  swapchainDescription.SampleDesc.Count = EnableMSAA_ ? 4 : 1;
  swapchainDescription.SampleDesc.Quality = 0;
  swapchainDescription.Windowed = TRUE;

  D3D_DRIVER_TYPE outDriverType;     //最终决定的DriverType
  D3D_FEATURE_LEVEL outFeatureLevel; //最终决定的FeatureLevel

  ComPtr<ID3D11Device> d3dDevice = nullptr;
  ComPtr<ID3D11DeviceContext> d3dDeviceIMContext = nullptr;
  ComPtr<IDXGISwapChain> SwapChain = nullptr;
  //按照驱动类型依次尝试创建Device和SwapChain
  for (UINT driverTypeIndex = 0; driverTypeIndex < ARRAYSIZE(driverTypes);
       driverTypeIndex++) {
    outDriverType = driverTypes[driverTypeIndex];
    hr = D3D11CreateDeviceAndSwapChain(
        NULL, outDriverType, NULL, 0, featureLevels, ARRAYSIZE(featureLevels),
        D3D11_SDK_VERSION, &swapchainDescription, &SwapChain, &d3dDevice,
        &outFeatureLevel, &d3dDeviceIMContext);
    if (SUCCEEDED(hr))
      break;
  }
  if (FAILED(hr))
    return false;
  FeatureLevel_ = outFeatureLevel;
  SwapChain.As(&pSwapChain_);
  d3dDevice.As(&pd3dDevice_);
  d3dDeviceIMContext.As(&pd3dDeviceIMContext_);

  // 设置调试对象名
  D3D11SetDebugObjectName(pd3dDeviceIMContext_.Get(), "ImmediateContext");
  // D3D11SetDebugObjectName(pd3dDevice_.Get(), "Device");
  DXGISetDebugObjectName(pSwapChain_.Get(), "SwapChain");

  // 每当窗口被重新调整大小的时候，都需要调用这个OnResize函数。现在调用
  // 以避免代码重复
  OnResize();

  ImGui_ImplDX11_Init(d3dDevice.Get(), d3dDeviceIMContext.Get());

  return true;
}
