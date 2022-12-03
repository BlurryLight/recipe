//
// Created by zhong on 2021/3/27.
//

#include "d3dApp.hh"
#include "cmake_vars.h"
#include <cassert>
#include <filesystem>
#include <iostream>
#include <spdlog/fmt/fmt.h>
#include <spdlog/spdlog.h>
#include <vector>
#include <windowsx.h>

#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;
using namespace PD;

D3DApp *D3DApp::mD3dApp = nullptr;
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return D3DApp::GetD3dDApp()->AppMessageProc(hwnd, uMsg, wParam, lParam);
}
int D3DApp::MessageLoopRun() {
    // Run the message loop.
    MSG msg = {};

    mTimer.Reset();
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            mTimer.Tick();
            // 当程序最小化时，mAppPaused,但是tick仍然在走
            if (!mAppPaused) {
                CalculateFrameStats();
                //do something in our loop
                Update(mTimer);
                Draw(mTimer);
            } else {
                Sleep(100);
            }
        }
    }

    ReleaseAllResource();
    return (int) msg.wParam;
}
HINSTANCE D3DApp::GetHInstance() const {
    assert(hinstance_);
    return hinstance_;
}
D3DApp *D3DApp::GetD3dDApp() {
    assert(mD3dApp);
    return mD3dApp;
}
D3DApp::D3DApp(HINSTANCE hinstance) : hinstance_(hinstance) {
    assert(!mD3dApp);
    mD3dApp = this;
}
D3DApp::~D3DApp() {}
HWND D3DApp::GetHMND() const {
    assert(hMainWindow_);
    return hMainWindow_;
}
float D3DApp::GetAspectRatio() const { return (float) mWidth / (float) (mHeight); }
bool &D3DApp::GetMSAAState() const { return mAppMSAA; }
bool D3DApp::SetMSAAState(bool val) {
    auto old = mAppMSAA;
    mAppMSAA = val;
    return old;
}
bool D3DApp::Initialize() {
    if (!InitMainWindow()) return false;
    if (!initDirect3D()) return false;
    initImGUI();

    OnResizeCallback();
    return true;
}
void D3DApp::OnResizeCallback() {
    assert(mD3dDevice);
    assert(mSwapChain);
    assert(mDirectCmdListAlloc);
    spdlog::info("Resizing {} x {}", mWidth, mHeight);

    FlushCommandQueue();

    // reset all SwapChain related resource
    HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    for (auto &SWBuffer : mSwapChainBuffer) { SWBuffer.Reset(); }
    mDepthStencilBuffer.Reset();

    // recreate swapchain or first creat
    HR(mSwapChain->ResizeBuffers(kSwapChainBufferCount, mWidth, mHeight, mBackBufferFormat,
                                 DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

    mCurrBackBuffer = 0;
    // SwapChain创建完以后只是单纯的Buffer
    // 要把Buffer创建RTV，并绑定到Device上

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHeapHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart());
    for (int i = 0; i < kSwapChainBufferCount; i++) {
        // 使得mSwapChainBuffer[i]的指针指向SwapChain内的Buffer
        HR(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mSwapChainBuffer[i])));
        // 由于rtvHeapHandle不是指针，所以不用传 &rtvHeapHandle
        // 这里实际含义与指针差不多: 修改rtvHeapHandle的指向到新建的RtvView
        mD3dDevice->CreateRenderTargetView(mSwapChainBuffer[i].Get(), nullptr, rtvHeapHandle);
        rtvHeapHandle.Offset(1, mRtvDescriptorSize);
    }

    // depth buffer 没有在交换链里，所以在这里手动创建纹理资源

    D3D12_RESOURCE_DESC depthStencilDesc;
    depthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    depthStencilDesc.Alignment = 0;
    depthStencilDesc.Width = mWidth;
    depthStencilDesc.Height = mHeight;
    depthStencilDesc.DepthOrArraySize = 1;
    depthStencilDesc.MipLevels = 1;
    // depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // TODO: 研究不同的
    depthStencilDesc.Format = mDepthStencilFormat;
    depthStencilDesc.SampleDesc.Count = 1;
    depthStencilDesc.SampleDesc.Quality = 0;
    depthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    depthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE optClear;
    optClear.Format = mDepthStencilFormat;
    optClear.DepthStencil.Depth = 1.0f;
    optClear.DepthStencil.Stencil = 0;
    // HeapProperty 指明了我们需要在GPU的哪个Heap上创建资源。 Default代表只在GPU上使用，UpLoad/ReadBack顾名思义
    CD3DX12_HEAP_PROPERTIES heapPp(D3D12_HEAP_TYPE_DEFAULT);
    HR(mD3dDevice->CreateCommittedResource(&heapPp, D3D12_HEAP_FLAG_NONE, &depthStencilDesc,
                                           D3D12_RESOURCE_STATE_COMMON, &optClear, IID_PPV_ARGS(&mDepthStencilBuffer)));
    //  创建了纹理后还需要创建对应的View以绑定到pipeline上
    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
    dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsvDesc.Format = mDepthStencilFormat;
    dsvDesc.Texture2D.MipSlice = 0;// only one mip
    mD3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, DepthStencilView());

    // 用Common创建然后转为可写状态
    auto DepthBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mDepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON,
                                                             D3D12_RESOURCE_STATE_DEPTH_WRITE);
    mCommandList->ResourceBarrier(1, &DepthBarrier);

    //cmdlist停止记录
    HR(mCommandList->Close());
    std::vector<ID3D12CommandList *> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists((uint32_t) cmdLists.size(), cmdLists.data());

    // 提交cmdlist后，CPU等待GPU
    FlushCommandQueue();

    mScreenViewport.Height = (float) mHeight;
    mScreenViewport.Width = (float) mWidth;
    mScreenViewport.TopLeftX = 0;
    mScreenViewport.TopLeftY = 0;
    // 通常是[0.0,1.0]，不要动
    // 用于调整Zbuffer的range
    mScreenViewport.MinDepth = 0.0f;
    mScreenViewport.MaxDepth = 1.0f;

    // will discard any pixel outside the scissor
    // https://www.khronos.org/opengl/wiki/Scissor_Test
    // per sample operation
    mScissorRect = D3D12_RECT{0, 0, mWidth, mHeight};

    CreateMSAAObjects();
}
static float gHighDPIScaleFactor = 1.0;
bool D3DApp::InitMainWindow() {

    ImGui_ImplWin32_EnableDpiAwareness();
    gHighDPIScaleFactor = ImGui_ImplWin32_GetDpiScaleForHwnd(GetDesktopWindow());
    // Register the window class.
    const char CLASS_NAME[] = "Sample Window Class";

    WNDCLASS wc = {};

    wc.lpfnWndProc = MainWindowProc;
    // wc.style = CS_VREDRAW|CS_HREDRAW;
    wc.hInstance = hinstance_;
    wc.lpszClassName = CLASS_NAME;
    wc.lpszMenuName = nullptr;


    mWidth = int(mWidth * gHighDPIScaleFactor);
    mHeight = int(mHeight * gHighDPIScaleFactor);
    RegisterClassA(&wc);
    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = {0, 0, mWidth, mHeight};
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    // Create the window.

    HWND hwnd = CreateWindowExA(0,                      // Optional window styles.
                                CLASS_NAME,             // Window class
                                AppWindowTitle_.c_str(),// Window text
                                WS_OVERLAPPEDWINDOW,    // Window style

                                // Size and position
                                CW_USEDEFAULT, CW_USEDEFAULT, width, height,

                                nullptr,   // Parent window
                                nullptr,   // Menu
                                hinstance_,// Instance handle
                                nullptr    // Additional application data
    );


    if (hwnd == nullptr) {
        auto error = GetLastError();
        std::string message = std::system_category().message(error);
        std::cout << message << std::endl;
        MessageBoxW(nullptr, L"Create Failed!", nullptr, 0);
        return false;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);

    hMainWindow_ = hwnd;

    return true;
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT D3DApp::AppMessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    if (ImGui_ImplWin32_WndProcHandler(hwnd, msg, wParam, lParam)) return true;

    switch (msg) {
        // WM_ACTIVATE is sent when the window is activated or deactivated.
        // We pause the game when the window is deactivated and unpause it
        // when it becomes active.
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                mAppPaused = true;
                mTimer.Stop();
            } else {
                mAppPaused = false;
                mTimer.Start();
            }
            return 0;

        // WM_SIZE is sent when the user resizes the window.
        case WM_SIZE:
            // Save the new client area dimensions.
            mWidth = LOWORD(lParam);
            mHeight = HIWORD(lParam);
            if (mD3dDevice) {
                if (wParam == SIZE_MINIMIZED) {
                    mAppPaused = true;
                    mAppMinimized = true;
                    mAppMaximized = false;
                } else if (wParam == SIZE_MAXIMIZED) {
                    mAppPaused = false;
                    mAppMinimized = false;
                    mAppMaximized = true;
                    OnResizeCallback();
                } else if (wParam == SIZE_RESTORED) {

                    // Restoring from minimized state?
                    if (mAppMinimized) {
                        mAppPaused = false;
                        mAppMinimized = false;
                        OnResizeCallback();
                    }

                    // Restoring from maximized state?
                    else if (mAppMaximized) {
                        mAppPaused = false;
                        mAppMaximized = false;
                        OnResizeCallback();
                    } else if (mAppResizing) {
                        // If user is dragging the resize bars, we do not resize
                        // the buffers here because as the user continuously
                        // drags the resize bars, a stream of WM_SIZE messages are
                        // sent to the window, and it would be pointless (and slow)
                        // to resize for each WM_SIZE message received from dragging
                        // the resize bars.  So instead, we reset after the user is
                        // done resizing the window and releases the resize bars, which
                        // sends a WM_EXITSIZEMOVE message.
                    } else// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
                    {
                        OnResizeCallback();
                    }
                }
            }

            return 0;

        // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
        case WM_ENTERSIZEMOVE:
            mAppPaused = true;
            mAppResizing = true;
            mTimer.Stop();
            return 0;

        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
        // Here we reset everything based on the new window dimensions.
        case WM_EXITSIZEMOVE:
            mAppPaused = false;
            mAppResizing = false;
            mTimer.Start();
            OnResizeCallback();
            return 0;

        // WM_DESTROY is sent when the window is being destroyed.
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        // The WM_MENUCHAR message is sent when a menu is active and the user presses
        // a key that does not correspond to any mnemonic or accelerator key.
        case WM_MENUCHAR:
            // Don't beep when we alt-enter.
            return MAKELRESULT(0, MNC_CLOSE);

        // Catch this message so to prevent the window from becoming too small.
        case WM_GETMINMAXINFO:
            ((MINMAXINFO *) lParam)->ptMinTrackSize.x = 200;
            ((MINMAXINFO *) lParam)->ptMinTrackSize.y = 200;
            return 0;

        case WM_LBUTTONDOWN:
        case WM_MBUTTONDOWN:
        case WM_RBUTTONDOWN:
            OnMouseDownCallback(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_LBUTTONUP:
        case WM_MBUTTONUP:
        case WM_RBUTTONUP:
            OnMouseUpCallback(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_MOUSEMOVE:
            OnMouseMoveCallback(wParam, GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
            return 0;
        case WM_KEYUP:
            if (wParam == VK_ESCAPE) {
                PostQuitMessage(0);
            } else if ((int) wParam == VK_F2) {
                SetMSAAState(!GetMSAAState());
            }
            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void PD::D3DApp::ImGuiPrepareDraw() {
    ImGui_ImplDX12_NewFrame();
    ImGui_ImplWin32_NewFrame();
    ImGui::NewFrame();
}

bool PD::D3DApp::initDirect3D() {
#if defined(_DEBUG)
    ComPtr<ID3D12Debug1> debugController = nullptr;
    // 通过COM查询最新的接口实现并填充进去
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    assert(debugController.Get());
    debugController->EnableDebugLayer();
    debugController->SetEnableGPUBasedValidation(true);
#endif
    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-enum
    // For Direct3D 11.0 and later, we recommend that apps always use IDXGIFactory1 and CreateDXGIFactory1 instead.

    // https://walbourn.github.io/anatomy-of-direct3d-12-create-device/
    ComPtr<IDXGIAdapter1> pAdapter = nullptr;
    HR(CreateDXGIFactory2(DXGI_CREATE_FACTORY_DEBUG, IID_PPV_ARGS(&mDxgiFactory)));
    {
        DXGI_ADAPTER_DESC1 adapterDesc;
        for (UINT i = 0; SUCCEEDED(mDxgiFactory->EnumAdapters1(i, &pAdapter)); ++i) {
            pAdapter->GetDesc1(&adapterDesc);
            // old method
            // if (adapterDesc.VendorId == 0x1414 && adapterDesc.DeviceId == 0x8c)
            if (adapterDesc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) continue;// Skip Microsoft Basic Render Driver
            spdlog::info("Available Adapter: {}", utf16_to_utf8_windows(adapterDesc.Description));
            break;
        }
    }

    HR(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mD3dDevice)));


    HR(mD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

    mRtvDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    mDsvDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    mCbvSrvUavDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 检查是否支持MSAA
    assert(CheckMSAASupport(mBackBufferFormat, 4));

    CreateCommandObjects();
    CreateSwapChain();
    CreateRtvAndDsvDescriptorHeaps();
    return true;
}

bool PD::D3DApp::initImGUI() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    mImGuiIO = &ImGui::GetIO();
    std::filesystem::path RootPath(ROOT_DIR);
    auto FontPath = RootPath / "resource" / "JetBrainsMono-Regular.ttf";
    auto Font =
            mImGuiIO->Fonts->AddFontFromFileTTF(FontPath.u8string().c_str(), 16.0f * gHighDPIScaleFactor, NULL, NULL);
    assert(Font);

    spdlog::info("Building ImGUI Descriptor Heaps");
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mImGuiCbvHeap)));

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(GetHMND());
    ImGui_ImplDX12_Init(mD3dDevice.Get(), kSwapChainBufferCount, mBackBufferFormat, mImGuiCbvHeap.Get(),
                        mImGuiCbvHeap->GetCPUDescriptorHandleForHeapStart(),
                        mImGuiCbvHeap->GetGPUDescriptorHandleForHeapStart());

    return true;
}

bool PD::D3DApp::CheckMSAASupport(DXGI_FORMAT format, int SampleConut) {
    // https://zhuanlan.zhihu.com/p/263101710
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
    msQualityLevels.Format = format;
    msQualityLevels.SampleCount = SampleConut;
    msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    msQualityLevels.NumQualityLevels = 0;
    ThrowIfFailed(mD3dDevice->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &msQualityLevels,
                                                  sizeof(msQualityLevels)));

    return msQualityLevels.NumQualityLevels > 0;
    // https://learn.microsoft.com/en-us/windows/win32/api/dxgicommon/ns-dxgicommon-dxgi_sample_desc
    // NumQualitiLevels > 0 代表支持在该format和sample count 下 MSAA
    // 对于DX11以上设备，所有格式都支持4x msaa
    // 对于返回的NumQualitiLevels，其代表显卡支持的不同算法。Quality数值越高，其耗费越多
    // 这个数字可以用在 DXGI_SAMPLE_DESC.Quality = NumQualitiLevels - 1;上
    // 但是这个值也许没有用了。。GTX 3080返回NumQualitiLevels = 1
}

void PD::D3DApp::CreateCommandObjects() {
    D3D12_COMMAND_QUEUE_DESC Desc{};
    Desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    HR(mD3dDevice->CreateCommandQueue(&Desc, IID_PPV_ARGS(&mCommandQueue)));
    HR(mD3dDevice->CreateCommandAllocator(Desc.Type, IID_PPV_ARGS(&mDirectCmdListAlloc)));
    HR(mD3dDevice->CreateCommandList(0, Desc.Type, mDirectCmdListAlloc.Get(), nullptr, IID_PPV_ARGS(&mCommandList)));
    HR(mCommandList->Close());
}

void PD::D3DApp::CreateSwapChain() {
    mSwapChain.Reset();
    DXGI_SWAP_CHAIN_DESC Desc;
    Desc.BufferDesc.Width = mWidth;
    Desc.BufferDesc.Height = mHeight;
    Desc.BufferDesc.RefreshRate.Numerator = 60;
    Desc.BufferDesc.RefreshRate.Denominator = 1;
    Desc.BufferDesc.Format = mBackBufferFormat;
    Desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    Desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
    Desc.SampleDesc.Count = 1;
    Desc.SampleDesc.Quality = 0;
    Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    Desc.BufferCount = kSwapChainBufferCount;
    Desc.OutputWindow = GetHMND();
    Desc.Windowed = true;
    Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

    HR(mDxgiFactory->CreateSwapChain(mCommandQueue.Get(), &Desc, mSwapChain.GetAddressOf()));
}
void PD::D3DApp::CreateRtvAndDsvDescriptorHeaps() {
    D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc{};
    rtvHeapDesc.NumDescriptors = kSwapChainBufferCount + 1;//  + 1 for msaa
    rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    rtvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&mRtvHeap)));

    D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc{};
    dsvHeapDesc.NumDescriptors = 1 + 1;// +1 for msaa
    dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
    dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    dsvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&mDsvHeap)));
}

void PD::D3DApp::CreateMSAAObjects() {

    assert(mDsvHeap);
    assert(mRtvHeap);
    spdlog::info("Create MSAA Objects");
    mMSAART.Reset();
    mMSAADepth.Reset();
    CD3DX12_HEAP_PROPERTIES heapPp(D3D12_HEAP_TYPE_DEFAULT);
    {

        // MSAA RT
        D3D12_RESOURCE_DESC MsaaRTDesc = CD3DX12_RESOURCE_DESC::Tex2D(mBackBufferFormat, mWidth, mHeight,
                                                                      1, // ArraySize
                                                                      1, // MipLevels
                                                                      4);// msaa samples

        MsaaRTDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
        D3D12_CLEAR_VALUE MsaaClearValue;
        MsaaClearValue.Format = mBackBufferFormat;
        MsaaClearValue.Color[0] = Colors::LightBlue[0];
        MsaaClearValue.Color[1] = Colors::LightBlue[1];
        MsaaClearValue.Color[2] = Colors::LightBlue[2];
        MsaaClearValue.Color[3] = Colors::LightBlue[3];

        HR(mD3dDevice->CreateCommittedResource(&heapPp, D3D12_HEAP_FLAG_NONE, &MsaaRTDesc,
                                               D3D12_RESOURCE_STATE_RESOLVE_SOURCE, &MsaaClearValue,
                                               IID_PPV_ARGS(&mMSAART)));
        D3D12_RENDER_TARGET_VIEW_DESC msaaRTVDesc = {};
        msaaRTVDesc.Format = mBackBufferFormat;
        msaaRTVDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
        // skip 2 backbuffer
        CD3DX12_CPU_DESCRIPTOR_HANDLE msaaRTVHandle(mRtvHeap->GetCPUDescriptorHandleForHeapStart(),
                                                    kSwapChainBufferCount, mRtvDescriptorSize);
        mD3dDevice->CreateRenderTargetView(mMSAART.Get(), &msaaRTVDesc, msaaRTVHandle);
    }

    // create DSV
    {

        D3D12_RESOURCE_DESC MsaaDepthDesc = CD3DX12_RESOURCE_DESC::Tex2D(mDepthStencilFormat, mWidth, mHeight,
                                                                         1, // ArraySize
                                                                         1, // MipLevels
                                                                         4);// msaa samples

        MsaaDepthDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
        D3D12_CLEAR_VALUE optClear;
        optClear.Format = mDepthStencilFormat;
        optClear.DepthStencil.Depth = 1.0f;
        optClear.DepthStencil.Stencil = 0;
        HR(mD3dDevice->CreateCommittedResource(&heapPp, D3D12_HEAP_FLAG_NONE, &MsaaDepthDesc,
                                               D3D12_RESOURCE_STATE_DEPTH_WRITE, &optClear, IID_PPV_ARGS(&mMSAADepth)));
        D3D12_DEPTH_STENCIL_VIEW_DESC MsaaDsvDesc;
        MsaaDsvDesc.Flags = D3D12_DSV_FLAG_NONE;
        MsaaDsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
        MsaaDsvDesc.Format = mDepthStencilFormat;
        MsaaDsvDesc.Texture2D.MipSlice = 0;// only one mip

        //DSV
        CD3DX12_CPU_DESCRIPTOR_HANDLE msaaDSVHandle(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1,
                                                    mDsvDescriptorSize);
        mD3dDevice->CreateDepthStencilView(mMSAADepth.Get(), &MsaaDsvDesc, msaaDSVHandle);
    }
}

void PD::D3DApp::FlushCommandQueue() {

    mCurrentFence++;
    HR(mCommandQueue->Signal(mFence.Get(), mCurrentFence));
    if (mFence->GetCompletedValue() < mCurrentFence) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        HR(mFence->SetEventOnCompletion(mCurrentFence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }
}

void PD::D3DApp::CalculateFrameStats() {
    static uint64_t frameCnt = 0;
    static float timeElapsed = 0.0f;// in seconds
    frameCnt++;
    if (mTimer.TotalTime() - timeElapsed > 1.0) {
        float fps = frameCnt / 1.0f;
        float ms_per_frame = 1000.0f / frameCnt;

        // std::string Title =
        //         AppWindowTitle_ + " fps: " + std::to_string(fps) + " ms/frame: " + std::to_string(ms_per_frame);
        std::string Title = fmt::format("{} FPS:{:03.2f}, Ms/Frame: {:03.2f}", AppWindowTitle_, fps, ms_per_frame);
        SetWindowTextA(GetHMND(), Title.c_str());
        frameCnt = 0;
        timeElapsed += 1.0f;
    }
}

ID3D12Resource *PD::D3DApp::CurrentBackBuffer() const {
    auto Ptr = mSwapChainBuffer[mCurrBackBuffer].Get();
    assert(Ptr);
    return Ptr;
}

D3D12_CPU_DESCRIPTOR_HANDLE PD::D3DApp::CurrentBackBufferView() const {
    assert(mRtvHeap);
    assert(mRtvDescriptorSize > 0);
    // 简单的数组偏移，第一个为首地址，第二个为index，第三个为 sizeof(struct)
    return CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), mCurrBackBuffer,
                                         mRtvDescriptorSize);
}

D3D12_CPU_DESCRIPTOR_HANDLE PD::D3DApp::DepthStencilView() const {
    return mDsvHeap->GetCPUDescriptorHandleForHeapStart();
}

void PD::D3DApp::ReleaseAllResource() {
    FlushCommandQueue();

    ReleaseResource();

    FlushCommandQueue();

    // Cleanup
    ImGui_ImplDX12_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();
    mImGuiIO = nullptr;

    // clean up rt
    mSwapChainBuffer[0].Reset();
    mSwapChainBuffer[1].Reset();
    mDepthStencilBuffer.Reset();
    mSwapChain.Reset();

    mCommandQueue.Reset();
    mCommandList.Reset();
    mDirectCmdListAlloc.Reset();
    mRtvHeap.Reset();
    mDsvHeap.Reset();
    mImGuiCbvHeap.Reset();

    mFence.Reset();
    mDxgiFactory.Reset();
    mD3dDevice.Reset();
}
