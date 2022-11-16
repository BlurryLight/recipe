//
// Created by zhong on 2021/3/27.
//


#pragma once
#if defined(DEBUG) || defined(_DEBUG)
#define _CRTDBG_MAP_ALLOC
#include <crtdbg.h>
#endif

// clang-format off
#include "defines.h"
// clang-format on
#include "d3dUtils.hh"
#include <string>

namespace PD {
    using Microsoft::WRL::ComPtr;
    class D3DApp : public Noncopyable {
        //    protected:
    public:
        explicit D3DApp(HINSTANCE hinstance);
        virtual ~D3DApp();

    public:
        HINSTANCE GetHInstance() const;
        HWND GetHMND() const;
        static D3DApp *GetD3dDApp();
        float GetAspectRatio() const;
        bool GetMSAAState() const;
        bool SetMSAAState(bool val);// return old state

        int MessageLoopRun();

        virtual bool Initialize();
        virtual LRESULT AppMessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    protected:
        virtual void OnResizeCallback();
        virtual void OnMouseDownCallback(WPARAM btnState, int x, int y) {
        }
        virtual void OnMouseUpCallback(WPARAM btnState, int x, int y){};
        virtual void OnMouseMoveCallback(WPARAM btnState, int x, int y) {}

        bool InitMainWindow();
        bool initDirect3D();
        bool CheckMSAASupport(DXGI_FORMAT format, int SampleConut);

        void CreateCommandObjects();
        void CreateSwapChain();


    protected:
        static D3DApp *D3DApp_;
        HINSTANCE hinstance_ = nullptr;
        HWND hMainWindow_ = nullptr;
        bool AppPaused_ = false;
        bool AppMinimized_ = false;
        bool AppMaximized_ = false;
        bool AppResizing_ = false;
        bool AppFullScreen_ = false;

        bool AppMSAA_ = false;
        UINT AppMSAAQuality_ = 0;

        constexpr static int kSwapChainBufferCount = 2;
        std::wstring AppWindowTitle_ = L"d3d test";
        int width_ = 800;
        int height_ = 600;

        // DX12
        ComPtr<IDXGIFactory4> mDxgiFactory;
        ComPtr<IDXGISwapChain> mSwapChain;
        ComPtr<ID3D12Device> mD3dDevice;
        ComPtr<ID3D12Fence> mFence;

        DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        uint64_t mCurrentFence = 0;
        uint32_t mRtvDescriptorSize = 0;
        uint32_t mDsvDescriptorSize = 0;
        uint32_t mCbvSrvUavDescriptorSize = 0;

        ComPtr<ID3D12CommandQueue> mCommandQueue;
        ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
        ComPtr<ID3D12GraphicsCommandList> mCommandList;
    };
}// namespace PD
