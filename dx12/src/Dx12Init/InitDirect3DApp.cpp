#include <d3dApp.hh>

using namespace PD;
class InitDirect3DApp : public D3DApp {

public:
    InitDirect3DApp(HINSTANCE hInstance) : D3DApp(hInstance) {}
    void Draw(const GameTimer &gt) override;
};

void InitDirect3DApp::Draw(const GameTimer &gt) {

    HR(mDirectCmdListAlloc->Reset());
    HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);

    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0,
                                        0, nullptr);
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                           D3D12_RESOURCE_STATE_PRESENT));
    HR(mCommandList->Close());
    std::vector<ID3D12CommandList *> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());

    // turn vsync on
    HR(mSwapChain->Present(1, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % kSwapChainBufferCount;
    FlushCommandQueue();
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {
    // Enable run-time memory check for debug builds.
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    InitDirect3DApp theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}