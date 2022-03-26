#include <GLFW/glfw3.h>

#define GLFW_EXPOSE_NATIVE_WIN32 1 //暴露出win32相关的原生接口
#include <GLFW/glfw3native.h>

#include <d3d11.h>
//#include <D3DX11.h>


//DX11的对象：
ID3D11Device* g_pd3dDevice = NULL;
ID3D11DeviceContext* g_pImmediateContext = NULL;
IDXGISwapChain* g_pSwapChain = NULL;
ID3D11RenderTargetView* g_pRenderTargetView = NULL;

//初始化D3D，参数是给交换链用的窗口句柄
HRESULT InitD3D(HWND OutputWindow)
{
  HRESULT hr = S_OK;

  //列出所有考虑的FeatureLevel：
  D3D_FEATURE_LEVEL featureLevels[] =
      {
          D3D_FEATURE_LEVEL_11_0,
          D3D_FEATURE_LEVEL_10_1,
          D3D_FEATURE_LEVEL_10_0,
      };

  //列出所有考虑的驱动类型（越靠前越优先考虑）
  D3D_DRIVER_TYPE driverTypes[] =
      {
          D3D_DRIVER_TYPE_HARDWARE,
          D3D_DRIVER_TYPE_WARP,
          D3D_DRIVER_TYPE_REFERENCE,
      };

  //创建SwapChain的描述结构体
  DXGI_SWAP_CHAIN_DESC swapchainDescription;
  ZeroMemory(&swapchainDescription, sizeof(swapchainDescription));
  swapchainDescription.BufferCount = 1;
  swapchainDescription.BufferDesc.Width = 640;
  swapchainDescription.BufferDesc.Height = 480;
  swapchainDescription.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  swapchainDescription.BufferDesc.RefreshRate.Numerator = 60;
  swapchainDescription.BufferDesc.RefreshRate.Denominator = 1;
  swapchainDescription.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  swapchainDescription.OutputWindow = OutputWindow;
  swapchainDescription.SampleDesc.Count = 1;
  swapchainDescription.SampleDesc.Quality = 0;
  swapchainDescription.Windowed = TRUE;

  D3D_DRIVER_TYPE         outDriverType;	//最终决定的DriverType
  D3D_FEATURE_LEVEL       outFeatureLevel;//最终决定的FeatureLevel
  //按照驱动类型依次尝试创建Device和SwapChain
  for (UINT driverTypeIndex = 0; driverTypeIndex < ARRAYSIZE(driverTypes); driverTypeIndex++)
  {
    outDriverType = driverTypes[driverTypeIndex];
    hr = D3D11CreateDeviceAndSwapChain(NULL, outDriverType, NULL, 0, featureLevels, ARRAYSIZE(featureLevels),
                                       D3D11_SDK_VERSION, &swapchainDescription, &g_pSwapChain, &g_pd3dDevice, &outFeatureLevel, &g_pImmediateContext);
    if (SUCCEEDED(hr))
      break;
  }
  if (FAILED(hr))
    return hr;

  //从SwapChain那里得到BackBuffer
  ID3D11Texture2D* pBackBuffer = NULL;
  // hr = g_pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);
  // or don't use __uuidof
  hr = g_pSwapChain->GetBuffer(0, IID_ID3D11Texture2D, (LPVOID*)&pBackBuffer);
  if (FAILED(hr))
    return hr;

  //创建一个 render target view
  hr = g_pd3dDevice->CreateRenderTargetView(pBackBuffer, NULL, &g_pRenderTargetView);
  pBackBuffer->Release();
  if (FAILED(hr))
    return hr;

  //输出合并阶段（Output-Merger Stage）设置RenderTarget
  g_pImmediateContext->OMSetRenderTargets(1, &g_pRenderTargetView, NULL);

  return hr;
}

void Render()
{
  // 清理 back buffer
  float ClearColor[4] = { 0.0f, 0.0f, 0.75f, 1.0f };
  g_pImmediateContext->ClearRenderTargetView(g_pRenderTargetView, ClearColor);

  // Present the information rendered to the back buffer to the front buffer (the screen)
  g_pSwapChain->Present(0, 0);
}

int main(void)
{
  //必须先初始化该库，然后才能使用大多数GLFW函数。成功初始化后，GLFW_TRUE将返回。如果发生错误，GLFW_FALSE则返回。
  if (!glfwInit())
    return -1;

  //创建窗口
  GLFWwindow* window = glfwCreateWindow(640, 480, "Hello World", NULL, NULL);
  if (!window)
  {
    glfwTerminate();
    return -1;
  }

  //初始化D3D
  InitD3D(glfwGetWin32Window(window));

  //循环直到用户关闭窗口
  while (!glfwWindowShouldClose(window))
  {
    //渲染
    Render();

    //轮询并处理事件
    glfwPollEvents();
  }

  //使用GLFW完成操作后，通常是在应用程序退出之前，需要终止GLFW
  glfwTerminate();

  return 0;
}