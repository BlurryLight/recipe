//
// Created by zhong on 2021/3/27.
//


#pragma once

#include "GameTimer.h"
#include "d3dUtils.hh"
#include "imgui.h"
#include <string>
#include "EulerCamera.hh"

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
        bool &GetMSAAState() const;
        bool SetMSAAState(bool val);// return old state

        int MessageLoopRun();

        virtual bool Initialize();
        virtual LRESULT AppMessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

        void ImGuiPrepareDraw();
        virtual void Update(const GameTimer& timer);
        virtual void Draw(const GameTimer& timer) = 0;

    protected:
        virtual void OnResizeCallback();
        virtual void OnKeyUpCallback(WPARAM key);
        virtual void OnMouseDownCallback(WPARAM btnState, int x, int y);
        virtual void OnMouseUpCallback(WPARAM btnState, int x, int y);

        bool InitMainWindow();
        bool initDirect3D();
        bool initImGUI();
        bool CheckMSAASupport(DXGI_FORMAT format, int SampleConut);

        void CreateCommandObjects();
        void CreateSwapChain();
        virtual void CreateRtvAndDsvDescriptorHeaps();
        void CreateMSAAObjects();

        void FlushCommandQueue();
        void CalculateFrameStats();

        ID3D12Resource* CurrentBackBuffer() const;
        D3D12_CPU_DESCRIPTOR_HANDLE CurrentBackBufferView()const;
        D3D12_CPU_DESCRIPTOR_HANDLE DepthStencilView()const;

        void ReleaseAllResource();
        virtual void ReleaseResource(){};
        virtual void ProcessInput(const GameTimer& gt);

    private:
        mutable bool mAppMSAA = false;

    protected:
        static D3DApp *mD3dApp;

        RECT mRect;
        class Camera* mCamera = nullptr;
        HINSTANCE hinstance_ = nullptr;
        HWND hMainWindow_ = nullptr;
        bool mAppPaused = false;
        bool mAppMinimized = false;
        bool mAppMaximized = false;
        bool mAppResizing = false;
        bool mAppFullScreen = false;
        bool mAppActive = false;
        UINT mAppMsAAQuality = 0;
        GameTimer mTimer;

        bool mAllowMouseMove = true;


        std::string AppWindowTitle_ = "d3d test";
        int mWidth = 800;
        int mHeight = 600;

        constexpr static int kSwapChainBufferCount = 2;
        // track current buffer
        int mCurrBackBuffer = 0;
        // 这两个指针指向SwapChain内部保存的Resource
        Microsoft::WRL::ComPtr<ID3D12Resource> mSwapChainBuffer[kSwapChainBufferCount];
        Microsoft::WRL::ComPtr<ID3D12Resource> mDepthStencilBuffer;

        // DX12
        ComPtr<IDXGIFactory4> mDxgiFactory;
        ComPtr<IDXGISwapChain> mSwapChain;
        ComPtr<ID3D12Device> mD3dDevice;
        ComPtr<ID3D12Fence> mFence;

        DXGI_FORMAT mBackBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
        DXGI_FORMAT mDepthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
        uint64_t mCurrentFence = 0;
        // 记录一个描述符有多大，方便后面在Heap里计算偏移量
        uint32_t mRtvDescriptorSize = 0;
        uint32_t mDsvDescriptorSize = 0;
        uint32_t mCbvSrvUavDescriptorSize = 0;

        ComPtr<ID3D12CommandQueue> mCommandQueue;
        ComPtr<ID3D12CommandAllocator> mDirectCmdListAlloc;
        ComPtr<ID3D12GraphicsCommandList> mCommandList;

        ComPtr<ID3D12DescriptorHeap> mRtvHeap;
        ComPtr<ID3D12DescriptorHeap> mDsvHeap;


        D3D12_VIEWPORT mScreenViewport;
        D3D12_RECT mScissorRect;

        ImGuiIO *mImGuiIO = nullptr;
        ComPtr<ID3D12DescriptorHeap> mImGuiCbvHeap;

        ComPtr<ID3D12Resource> mMSAART;
        ComPtr<ID3D12Resource> mMSAADepth;
        ComPtr<ID3D12PipelineState> mMSAAOpaquePSO;
        // ComPtr<ID3D12PipelineState> mMSAATransparentPSO;
    };
}// namespace PD
