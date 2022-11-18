#include <d3dApp.hh>
#include <array>
#include <spdlog/spdlog.h>
#include "MathHelper.h"
#include "UploadBuffer.hh"

using namespace PD;

using namespace DirectX;
struct Vertex
{
    XMFLOAT3 Pos;
    XMFLOAT4 Color;
};

struct ObjectConstants
{

    XMFLOAT4X4 MVP = MathHelper::Identity4x4();
};


class MiniCube: public D3DApp {

public:
    MiniCube(HINSTANCE hInstance) : D3DApp(hInstance) {}
    virtual bool Initialize() override;
    void Draw(const GameTimer &gt) override;
    private:

    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    ComPtr<ID3D12DescriptorHeap> mCbvHeap;

     std::unique_ptr<UploadBuffer<ObjectConstants>> mObjectCB = nullptr;
};

inline bool MiniCube::Initialize() {
    if(!D3DApp::Initialize())
    {
        return false;
    }
    assert(mCommandList);
    assert(mDirectCmdListAlloc);
    HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    // ready for record
    BuildDescriptorHeaps();
    BuildConstantBuffers();    
    // end of record
    HR(mCommandList->Close());
    std::array<ID3D12CommandList*,1> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
    FlushCommandQueue();
    return true;
}

void MiniCube::Draw(const GameTimer &gt) {
    FlushCommandQueue();
}

inline void MiniCube::BuildDescriptorHeaps() {
    assert(mRtvHeap.Get());
    assert(mDsvHeap.Get());
    spdlog::info("Building Descriptor Heaps");
    // 除了父类建立的 rtv,Dsv以外还需要建立 src/uav/cbv的描述符堆
    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = 1;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));
}

inline void MiniCube::BuildConstantBuffers() {
    spdlog::info("Building Constant Buffers");

}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    MiniCube theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}