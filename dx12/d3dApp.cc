//
// Created by zhong on 2021/3/27.
//

#include "d3dApp.hh"
#include <cassert>
#include <iostream>
#include <windowsx.h>

using Microsoft::WRL::ComPtr;
using namespace std;
using namespace DirectX;
using namespace PD;

D3DApp *D3DApp::D3DApp_ = nullptr;
LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return D3DApp::GetD3dDApp()->AppMessageProc(hwnd, uMsg, wParam, lParam);
}
int D3DApp::MessageLoopRun() {
    // Run the message loop.
    MSG msg = {};
    while (msg.message != WM_QUIT) {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        } else {
            //do something in our loop
        }
    }
    return (int) msg.wParam;
}
HINSTANCE D3DApp::GetHInstance() const {
    return hinstance_;
}
D3DApp *D3DApp::GetD3dDApp() {
    assert(D3DApp_);
    return D3DApp_;
}
D3DApp::D3DApp(HINSTANCE hinstance) : hinstance_(hinstance) {
    assert(!D3DApp_);
    D3DApp_ = this;
}
D3DApp::~D3DApp() {
}
HWND D3DApp::GetHMND() const {
    return hMainWindow_;
}
float D3DApp::GetAspectRatio() const {
    return (float) width_ / (float) (height_);
}
bool D3DApp::GetMSAAState() const {
    return AppMSAA_;
}
bool D3DApp::SetMSAAState(bool val) {
    auto old = AppMSAA_;
    AppMSAA_ = val;
    return old;
}
bool D3DApp::Initialize() {
    if (!InitMainWindow()) return false;
    if (!initDirect3D()) return false;
    OnResizeCallback();
    return true;
}
void D3DApp::OnResizeCallback() {
}
bool D3DApp::InitMainWindow() {
    // Register the window class.
    const wchar_t CLASS_NAME[] = L"Sample Window Class";

    WNDCLASS wc = {};

    wc.lpfnWndProc = MainWindowProc;
    wc.hInstance = hinstance_;
    wc.lpszClassName = CLASS_NAME;
    wc.lpszMenuName = nullptr;

    RegisterClass(&wc);
    // Compute window rectangle dimensions based on requested client area dimensions.
    RECT R = {0, 0, width_, height_};
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
    int width = R.right - R.left;
    int height = R.bottom - R.top;

    // Create the window.

    HWND hwnd = CreateWindowEx(
            0,                      // Optional window styles.
            CLASS_NAME,             // Window class
            AppWindowTitle_.c_str(),// Window text
            WS_OVERLAPPEDWINDOW,    // Window style

            // Size and position
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

            nullptr,   // Parent window
            nullptr,   // Menu
            hinstance_,// Instance handle
            nullptr    // Additional application data
    );

    if (hwnd == nullptr) {
        MessageBox(nullptr, L"Create Failed!", nullptr, 0);
        return false;
    }

    ShowWindow(hwnd, SW_SHOW);
    UpdateWindow(hwnd);


    return true;
}
LRESULT D3DApp::AppMessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {

    switch (msg) {
        // WM_ACTIVATE is sent when the window is activated or deactivated.
        // We pause the game when the window is deactivated and unpause it
        // when it becomes active.
        case WM_ACTIVATE:
            if (LOWORD(wParam) == WA_INACTIVE) {
                AppPaused_ = true;
                //                mTimer.Stop();
            } else {
                AppPaused_ = false;
                //                mTimer.Start();
            }
            return 0;

        // WM_SIZE is sent when the user resizes the window.
        case WM_SIZE:
            // Save the new client area dimensions.
            width_ = LOWORD(lParam);
            height_ = HIWORD(lParam);
            //            if (md3dDevice) {
            //                if (wParam == SIZE_MINIMIZED) {
            //                    mAppPaused = true;
            //                    mMinimized = true;
            //                    mMaximized = false;
            //                } else if (wParam == SIZE_MAXIMIZED) {
            //                    mAppPaused = false;
            //                    mMinimized = false;
            //                    mMaximized = true;
            //                    OnResize();
            //                } else if (wParam == SIZE_RESTORED) {
            //
            //                    // Restoring from minimized state?
            //                    if (mMinimized) {
            //                        mAppPaused = false;
            //                        mMinimized = false;
            //                        OnResize();
            //                    }
            //
            //                    // Restoring from maximized state?
            //                    else if (mMaximized) {
            //                        mAppPaused = false;
            //                        mMaximized = false;
            //                        OnResize();
            //                    } else if (mResizing) {
            //                        // If user is dragging the resize bars, we do not resize
            //                        // the buffers here because as the user continuously
            //                        // drags the resize bars, a stream of WM_SIZE messages are
            //                        // sent to the window, and it would be pointless (and slow)
            //                        // to resize for each WM_SIZE message received from dragging
            //                        // the resize bars.  So instead, we reset after the user is
            //                        // done resizing the window and releases the resize bars, which
            //                        // sends a WM_EXITSIZEMOVE message.
            //                    } else// API call such as SetWindowPos or mSwapChain->SetFullscreenState.
            //                    {
            //                        OnResize();
            //                    }
            //                }
            //            }
            return 0;

        // WM_EXITSIZEMOVE is sent when the user grabs the resize bars.
        case WM_ENTERSIZEMOVE:
            AppPaused_ = true;
            AppResizing_ = true;
            //            mTimer.Stop();
            return 0;

        // WM_EXITSIZEMOVE is sent when the user releases the resize bars.
        // Here we reset everything based on the new window dimensions.
        case WM_EXITSIZEMOVE:
            AppPaused_ = false;
            AppResizing_ = false;
            //            mTimer.Start();
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
            } else if ((int) wParam == VK_F2)
                SetMSAAState(!GetMSAAState());

            return 0;
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}

bool PD::D3DApp::initDirect3D() {
#if defined(DEBUG) | defined(_DEBUG)
    ComPtr<ID3D12Debug> debugController = nullptr;
    // 通过COM查询最新的接口实现并填充进去
    ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)));
    assert(debugController.Get());
    debugController->EnableDebugLayer();
#endif
    // https://learn.microsoft.com/en-us/windows/win32/direct3d11/overviews-direct3d-11-devices-enum
    // For Direct3D 11.0 and later, we recommend that apps always use IDXGIFactory1 and CreateDXGIFactory1 instead.

    ComPtr<IDXGIAdapter> pAdapter = nullptr;
    HR(CreateDXGIFactory1(IID_PPV_ARGS(&mDxgiFactory)));
    {
        DXGI_ADAPTER_DESC adapterDesc;
        for (UINT i = 0;
             SUCCEEDED(mDxgiFactory->EnumAdapters(i, &pAdapter));
             ++i) {
            pAdapter->GetDesc(&adapterDesc);
            if (adapterDesc.VendorId == 0x1414 && adapterDesc.DeviceId == 0x8c)
                 continue; // Skip Microsoft Basic Render Driver
            std::wcout << "Avaliable Adapter: " << std::wstring(adapterDesc.Description) << std::endl;
            break;
        }
    }

    HR(D3D12CreateDevice(
            nullptr,
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&mD3dDevice)));


    HR(mD3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));

    mRtvDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	mDsvDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	mCbvSrvUavDescriptorSize = mD3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    // 检查是否支持MSAA
    assert(CheckMSAASupport(mBackBufferFormat, 4));

    CreateCommandObjects();
    return true;
}

bool PD::D3DApp::CheckMSAASupport(DXGI_FORMAT format,int SampleConut)
{
    // https://zhuanlan.zhihu.com/p/263101710
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS msQualityLevels;
	msQualityLevels.Format = format;
	msQualityLevels.SampleCount = SampleConut;
	msQualityLevels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	msQualityLevels.NumQualityLevels = 0;
	ThrowIfFailed(mD3dDevice->CheckFeatureSupport(
		D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
		&msQualityLevels,
		sizeof(msQualityLevels)));

    return msQualityLevels.NumQualityLevels > 0;
    // https://learn.microsoft.com/en-us/windows/win32/api/dxgicommon/ns-dxgicommon-dxgi_sample_desc
    // NumQualitiLevels > 0 代表支持在该format和sample count 下 MSAA
    // 对于DX11以上设备，所有格式都支持4x msaa
    // 对于返回的NumQualitiLevels，其代表显卡支持的不同算法。Quality数值越高，其耗费越多
    // 这个数字可以用在 DXGI_SAMPLE_DESC.Quality = NumQualitiLevels - 1;上
    // 但是这个值也许没有用了。。GTX 3080返回NumQualitiLevels = 1

}

void PD::D3DApp::CreateCommandObjects()
{
    D3D12_COMMAND_QUEUE_DESC Desc{};
    Desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    Desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    HR(mD3dDevice->CreateCommandQueue(&Desc, IID_PPV_ARGS(&mCommandQueue)));
    HR(mD3dDevice->CreateCommandAllocator(Desc.Type, IID_PPV_ARGS(&mDirectCmdListAlloc)));
    HR(mD3dDevice->CreateCommandList(0, Desc.Type, mDirectCmdListAlloc.Get(), nullptr, 
    IID_PPV_ARGS(&mCommandList)));
    HR(mCommandList->Close());
}