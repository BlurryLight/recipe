#include "CommonMesh.hh"
#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "UploadBuffer.hh"
#include "resource_path_searcher.h"
#include <DDSTextureLoader12.h>
#include <MeshGeometry.hh>
#include <array>
#include <d3dApp.hh>
#include <spdlog/spdlog.h>
#include <stb_image/stb_image.h>


#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"
#include <map>


using namespace PD;

using namespace DirectX;

using DirectX::PackedVector::XMCOLOR;
const static int kNumFrameResources = 3;

// RenderItem is related to per object
struct RenderItem {
    XMFLOAT4X4 World = MathHelper::Identity4x4();
    XMFLOAT4X4 TexTransform = MathHelper::Identity4x4();
    // init: every resource needs to be updated
    int NumFramesDirty = kNumFrameResources;

    UINT ObjectCBIndex = -1;

    int TextureHandleIndex = 0;

    MeshGeometry *Geo = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // draw instance params
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};


class BlendApp : public D3DApp {
public:
    BlendApp(HINSTANCE hInstance);
    virtual void ReleaseResource() override;
    virtual bool Initialize() override;
    void Update(const GameTimer &timer) override;
    void Draw(const GameTimer &gt) override;

private:
    void BuildDescriptorHeaps();
    void BuildConstantBuffers();
    void BuildRootSignature();
    void BuildShaderAndInputLayout();
    void BuildShapeGeometry();
    void BuildPSOs();
    void BuildFrameResources();
    void BuildRenderItems();
    void LoadTextures();

    void UpdateObjectCB(const GameTimer &timer);
    void UpdateMainPassCB(const GameTimer &timer);
    void DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<const RenderItem *> &ritems);
    ComPtr<ID3D12DescriptorHeap> mCbvHeap;
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ResourcePathSearcher mResourceManager;

    std::unordered_map<std::string, ComPtr<ID3D10Blob>> mShaders;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;

    std::map<std::string, std::unique_ptr<PD::Texture>> mTextures;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    std::vector<const RenderItem *> mOpaqueRitems;
    std::vector<const RenderItem *> mAlphaCoverageRitems;
    std::vector<const RenderItem *> mAlphaBlendRitems;
    std::vector<const RenderItem *> mAlphaClipRitems;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();
    UINT mPassCbvOffset = 0;
    int mCurrFrameResourceIndex = 0;

    FrameResource *mCurrFrameResource = nullptr;
    PassConstants mMainPassCB;
    bool mbShowWireFrame = false;
    bool mbVsync = true;

    // scale x scale y transx trans y
    SimpleMath::Vector4 mCrateTextureTrans = SimpleMath::Vector4(1, 1, 0, 0);
};

inline BlendApp::BlendApp(HINSTANCE hInstance) : D3DApp(hInstance) {
    auto path = PD::ResourcePathSearcher::Path(PROJECT_DIR);
    if (!path.is_absolute()) { path = fs::absolute(path); }
    mResourceManager.add_path(path);
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "models");
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "textures");
    mResourceManager.add_path(path / L"Shaders");
}

inline void BlendApp::ReleaseResource() {
    mRootSignature.Reset();
    mCbvHeap.Reset();
}

inline bool BlendApp::Initialize() {
    if (!D3DApp::Initialize()) { return false; }
    mCamera = new PD::Camera(SimpleMath::Vector3(0, 2, -5), SimpleMath::Vector3(0, 0, 1), SimpleMath::Vector3(0, 1, 0));
    assert(mCommandList);
    assert(mDirectCmdListAlloc);
    HR(mCommandList->Reset(mDirectCmdListAlloc.Get(), nullptr));
    // ready for record
    LoadTextures();
    BuildRootSignature();
    BuildShaderAndInputLayout();
    BuildShapeGeometry();
    BuildRenderItems();
    BuildFrameResources();
    BuildDescriptorHeaps();
    BuildConstantBuffers();
    BuildPSOs();

    // end of record
    HR(mCommandList->Close());
    std::array<ID3D12CommandList *, 1> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());
    FlushCommandQueue();

    for (auto &it : mGeometries) { it.second->DisposeUploaders(); }

    return true;
}

inline void BlendApp::Update(const GameTimer &timer) {

    D3DApp::Update(timer);
    XMStoreFloat4x4(&mView, mCamera->GetViewMatrix());

    // The window resized, so update the aspect ratio and recompute the projection matrix.
    float aspect = GetAspectRatio();
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspect, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);

    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % kNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // 此时GPU已经落后CPU kNumFrameResources帧
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        HR(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectCB(timer);
    UpdateMainPassCB(timer);

    ImGuiPrepareDraw();
    static bool bShowDemoWindow = false;
    if (bShowDemoWindow) ImGui::ShowDemoWindow(&bShowDemoWindow);

    ImGui::Begin("Hello, world!");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::Checkbox("MSAA State", &GetMSAAState());
    ImGui::Checkbox("WireFrame", &mbShowWireFrame);
    ImGui::Checkbox("VSync", &mbVsync);
    ImGui::Checkbox("DemoWindow", &bShowDemoWindow);
    if (ImGui::SliderFloat4("CrateTextureTrans", (float *) &mCrateTextureTrans, 0.0f, 2.0f)) {
        // 刷新所有缓冲区的属性
        mAllRitems[0]->NumFramesDirty = kNumFrameResources;
    }

    ImGui::End();
    ImGui::Render();
}

void BlendApp::UpdateObjectCB(const GameTimer &timer) {
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto &element : mAllRitems) {
        // upload data when needed
        if (element->NumFramesDirty > 0) {
            XMMATRIX world = XMLoadFloat4x4(&element->World);
            ObjectConstants objCB;
            XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objCB.InvTransWorld, XMMatrixTranspose(MathHelper::InverseTranspose(world)));

            auto TexTrans = XMMatrixScaling(mCrateTextureTrans.x, mCrateTextureTrans.y, 0) *
                            XMMatrixTranslation(mCrateTextureTrans.z, mCrateTextureTrans.w, 0);

            XMStoreFloat4x4(&objCB.TexTransform, XMMatrixTranspose(TexTrans));
            currObjectCB->CopyData(element->ObjectCBIndex, objCB);
            element->NumFramesDirty--;
        }
    }
}
inline void BlendApp::UpdateMainPassCB(const GameTimer &gt) {
    XMMATRIX view = XMLoadFloat4x4(&mView);
    XMMATRIX proj = XMLoadFloat4x4(&mProj);

    XMMATRIX viewProj = XMMatrixMultiply(view, proj);
    XMMATRIX invView = XMMatrixInverse(nullptr, view);
    XMMATRIX invProj = XMMatrixInverse(nullptr, proj);
    XMMATRIX invViewProj = XMMatrixInverse(nullptr, viewProj);

    XMStoreFloat4x4(&mMainPassCB.View, XMMatrixTranspose(view));
    XMStoreFloat4x4(&mMainPassCB.InvView, XMMatrixTranspose(invView));
    XMStoreFloat4x4(&mMainPassCB.Proj, XMMatrixTranspose(proj));
    XMStoreFloat4x4(&mMainPassCB.InvProj, XMMatrixTranspose(invProj));
    XMStoreFloat4x4(&mMainPassCB.ViewProj, XMMatrixTranspose(viewProj));
    XMStoreFloat4x4(&mMainPassCB.InvViewProj, XMMatrixTranspose(invViewProj));
    mMainPassCB.EyePosW = mCamera->Position;
    mMainPassCB.RenderTargetSize = XMFLOAT2((float) mWidth, (float) mHeight);
    mMainPassCB.InvRenderTargetSize = XMFLOAT2(1.0f / mWidth, 1.0f / mHeight);
    mMainPassCB.NearZ = 1.0f;
    mMainPassCB.FarZ = 1000.0f;
    mMainPassCB.TotalTime = gt.TotalTime();
    mMainPassCB.DeltaTime = gt.DeltaTime();

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

inline void BlendApp::DrawRenderItems(ID3D12GraphicsCommandList *cmdList,
                                      const std::vector<const RenderItem *> &ritems) {

    uint32_t objCBByteSize = CalcConstantBufferBytesSize(sizeof(ObjectConstants));
    auto objectCB = mCurrFrameResource->ObjectCB->Resource();
    for (size_t i = 0; i < ritems.size(); i++) {
        // 要绘制一个物体，需要物体本身的信息，和它有关的model
        // 先设置顶点信息
        auto ri = ritems[i];
        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS ObjCBStartAddress = objectCB->GetGPUVirtualAddress();
        auto ObjCBAddress = ObjCBStartAddress + ri->ObjectCBIndex * objCBByteSize;

        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        tex.Offset(ri->TextureHandleIndex, mCbvSrvUavDescriptorSize);
        cmdList->SetGraphicsRootConstantBufferView(0, ObjCBAddress);
        cmdList->SetGraphicsRootDescriptorTable(2, tex);
        assert(ri->IndexCount > 0);
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}


void BlendApp::Draw(const GameTimer &gt) {


    // 取出当前帧的Alloc,然后把cmdlist关联到这个alloc
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    HR(cmdListAlloc->Reset());

    // stage 0: msaa + opaque
    auto SwitchPSO = [this]() {
        if (GetMSAAState()) { return mMSAAOpaquePSO.Get(); }
        return mbShowWireFrame ? mPSOs["opaque_wireframe"].Get() : mPSOs["opaque"].Get();
    };
    HR(mCommandList->Reset(cmdListAlloc.Get(), SwitchPSO()));

    auto MsaaRTVHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(mRtvHeap->GetCPUDescriptorHandleForHeapStart(), 2, mRtvDescriptorSize);
    auto MsaaDSVHandle =
            CD3DX12_CPU_DESCRIPTOR_HANDLE(mDsvHeap->GetCPUDescriptorHandleForHeapStart(), 1, mDsvDescriptorSize);

    if (GetMSAAState()) {
        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMSAART.Get(),
                                                                               D3D12_RESOURCE_STATE_RESOLVE_SOURCE,
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET));
        mCommandList->ClearRenderTargetView(MsaaRTVHandle, DirectX::Colors::LightBlue, 0, nullptr);
        mCommandList->ClearDepthStencilView(MsaaDSVHandle, D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0,
                                            0, nullptr);
    }

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);


    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0,
                                        0, nullptr);

    if (GetMSAAState()) {
        mCommandList->OMSetRenderTargets(1, &MsaaRTVHandle, true, &MsaaDSVHandle);
    } else {
        mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    }

    std::vector<ID3D12DescriptorHeap *> descriptorHeaps{mSrvDescriptorHeap.Get()};
    mCommandList->SetDescriptorHeaps(descriptorHeaps.size(), descriptorHeaps.data());
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetGraphicsRootConstantBufferView(1, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());

    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);


    if (GetMSAAState()) {

        // stage 2: alpha_to_coverage
        mCommandList->SetPipelineState(mPSOs["transparent_alpha_to_coverage"].Get());
        DrawRenderItems(mCommandList.Get(), mAlphaCoverageRitems);

        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(mMSAART.Get(),
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                               D3D12_RESOURCE_STATE_RESOLVE_SOURCE));


        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                               D3D12_RESOURCE_STATE_RESOLVE_DEST));
        mCommandList->ResolveSubresource(CurrentBackBuffer(), 0, mMSAART.Get(), 0, mBackBufferFormat);


        mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                               D3D12_RESOURCE_STATE_RESOLVE_DEST,
                                                                               D3D12_RESOURCE_STATE_RENDER_TARGET));
    }


    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    mCommandList->SetPipelineState(mPSOs["transparent_blend"].Get());
    // 按距离从远到近排序
    sort(mAlphaBlendRitems.begin(), mAlphaBlendRitems.end(), [this](const RenderItem *l, const RenderItem *r) {
        SimpleMath::Vector3 lpos{l->World._41, l->World._42, l->World._43};
        SimpleMath::Vector3 rpos{r->World._41, r->World._42, r->World._43};
        auto ldis = (lpos - mCamera->Position).LengthSquared();
        auto rdis = (rpos - mCamera->Position).LengthSquared();
        return ldis > rdis;
    });
    DrawRenderItems(mCommandList.Get(), mAlphaBlendRitems);

    mCommandList->SetPipelineState(mPSOs["alphaClip"].Get());
    DrawRenderItems(mCommandList.Get(), mAlphaClipRitems);

    // stage 3: imgui
    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    mCommandList->SetPipelineState(mPSOs["opaque"].Get());
    mCommandList->SetDescriptorHeaps(1, mImGuiCbvHeap.GetAddressOf());
    ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), mCommandList.Get());


    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET,
                                                                           D3D12_RESOURCE_STATE_PRESENT));
    HR(mCommandList->Close());
    std::vector<ID3D12CommandList *> cmdLists{mCommandList.Get()};
    mCommandQueue->ExecuteCommandLists(cmdLists.size(), cmdLists.data());

    // turn vsync on
    HR(mSwapChain->Present(mbVsync, 0));
    mCurrBackBuffer = (mCurrBackBuffer + 1) % kSwapChainBufferCount;

    // CPU往FrameResource里记录一下新的Fence点，当这一帧完成的时候，Fence应该为 ++mCurrentFence
    // 同时往GPU插一条命令，当GPU任务执行完时，应该把mFence的值设置为mCurrentFence
    mCurrFrameResource->Fence = ++mCurrentFence;
    mCommandQueue->Signal(mFence.Get(), mCurrentFence);
}

inline void BlendApp::BuildDescriptorHeaps() {
    assert(mRtvHeap.Get());
    assert(mDsvHeap.Get());
    spdlog::info("Building Descriptor Heaps");

    UINT ObjCount = (UINT) mOpaqueRitems.size();
    // ObjCount 是per obj的cb
    // +1 是整个Pass共通的cb
    UINT numDescriptor = (ObjCount + 1) * kNumFrameResources;

    mPassCbvOffset = ObjCount * kNumFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = numDescriptor;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));

    // srv

    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.NumDescriptors = mTextures.size();
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvDescriptorHeap)));


    CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
    for (const auto &it : mTextures) {

        auto resource = it.second->Resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        mD3dDevice->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
        srvHandle.Offset(1, mCbvSrvUavDescriptorSize);
    }
}

inline void BlendApp::BuildConstantBuffers() {
    spdlog::info("Building Constant Buffers");

    spdlog::info("Building Objects Constant Buffers");
    uint32_t ObjCBBytesSize = CalcConstantBufferBytesSize(sizeof(ObjectConstants));
    UINT ObjCount = (UINT) mOpaqueRitems.size();
    for (int frameIndex = 0; frameIndex < kNumFrameResources; ++frameIndex) {
        auto ObjectCB = mFrameResources[frameIndex]->ObjectCB->Resource();
        for (uint32_t i = 0; i < ObjCount; i++) {
            D3D12_GPU_VIRTUAL_ADDRESS cbAddress = ObjectCB->GetGPUVirtualAddress();
            // 第几个物体的ConstBuffer
            cbAddress += i * ObjCBBytesSize;
            int heapIndex = frameIndex * ObjCount + i;
            auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
            handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);
            D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
            cbvDesc.BufferLocation = cbAddress;
            cbvDesc.SizeInBytes = ObjCBBytesSize;
            mD3dDevice->CreateConstantBufferView(&cbvDesc, handle);
        }
    }

    spdlog::info("Building Passes Constant Buffers");
    UINT passCBByteSize = CalcConstantBufferBytesSize(sizeof(PassConstants));

    // Last three descriptors are the pass CBVs for each frame resource.
    for (int frameIndex = 0; frameIndex < kNumFrameResources; ++frameIndex) {
        auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

        // Offset to the pass cbv in the descriptor heap.
        assert(mPassCbvOffset > 0);
        int heapIndex = mPassCbvOffset + frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = passCBByteSize;
        mD3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }
}

inline void BlendApp::BuildRootSignature() {
    CD3DX12_ROOT_PARAMETER slotRootParameter[3];
    // 一个放Object，一个放PassCB
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);

    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParameter[2].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    auto staticSamplers = PD::GetStaticSamplers();

    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter, staticSamplers.size(),
                                            staticSamplers.data(),
                                            D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
    ComPtr<ID3DBlob> serializedRootSig = nullptr;
    ComPtr<ID3DBlob> errorBlob = nullptr;// 如果创建root sig失败了，里面会存放错误信息

    HRESULT hr = D3D12SerializeRootSignature(&rootSigDesc, D3D_ROOT_SIGNATURE_VERSION_1,
                                             serializedRootSig.GetAddressOf(), errorBlob.GetAddressOf());
    if (errorBlob) { spdlog::warn("Root Sig error: {}", (const char *) (errorBlob->GetBufferPointer())); }
    HR(hr);

    HR(mD3dDevice->CreateRootSignature(0, serializedRootSig->GetBufferPointer(), serializedRootSig->GetBufferSize(),
                                       IID_PPV_ARGS(&mRootSignature)));
}

inline void BlendApp::BuildShaderAndInputLayout() {
    spdlog::info("Bulding Shaders");
    auto ShaderPath = mResourceManager.find_path("color.hlsl");

    auto VSBlob = CompileShader(ShaderPath, nullptr, "VSMain", "vs_5_0");
    auto PSBlob = CompileShader(ShaderPath, nullptr, "PSMain", "ps_5_0");
    assert(VSBlob);
    assert(PSBlob);
    mShaders["standardVS"] = VSBlob;
    mShaders["opaquePS"] = PSBlob;

    const D3D_SHADER_MACRO alphaTestDefines[] = {{"ALPHA_TEST", "1"}, {NULL, NULL}};
    PSBlob = CompileShader(ShaderPath, alphaTestDefines, "PSMain", "ps_5_0");
    mShaders["alphaClip"] = PSBlob;

    spdlog::info("Bulding Input Layout");
    mInputLayout = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
             0},
            {
                    "TEXCOORD",
                    0,
                    DXGI_FORMAT_R32G32_FLOAT,
                    0,
                    sizeof(XMFLOAT3) + sizeof(XMFLOAT3),
                    D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
                    0,
            },
    };
}

inline void BlendApp::BuildShapeGeometry() {
    spdlog::info("Bulding Shape Geometries");

    auto BoxGeo = PD::CreateBoxMesh(mD3dDevice.Get(), mCommandList.Get());
    mGeometries[BoxGeo->name] = std::move(BoxGeo);

    auto PlaneGeo = PD::CreatePlaneMesh(mD3dDevice.Get(), mCommandList.Get());
    mGeometries[PlaneGeo->name] = std::move(PlaneGeo);

    auto SphereGeo = PD::CreateSphereMesh(mD3dDevice.Get(), mCommandList.Get());
    mGeometries[SphereGeo->name] = std::move(SphereGeo);
}

inline void BlendApp::BuildPSOs() {

    spdlog::info("Building PSO");
    spdlog::info("Building Opaque PSO");
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc{0};
    psoDesc.InputLayout = {mInputLayout.data(), (UINT) mInputLayout.size()};
    psoDesc.pRootSignature = mRootSignature.Get();

    psoDesc.VS = {(void *) (mShaders["standardVS"]->GetBufferPointer()), mShaders["standardVS"]->GetBufferSize()};
    psoDesc.PS = {(void *) (mShaders["opaquePS"]->GetBufferPointer()), mShaders["opaquePS"]->GetBufferSize()};

    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);

    // PSO这里指定的 Primitive 类型 和 IA那里指定的 TriangleStrip，需要是同一大类，但是不需完全匹配上
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = mBackBufferFormat;
    psoDesc.DSVFormat = mDepthStencilFormat;

    psoDesc.SampleMask = UINT_MAX;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.SampleDesc.Quality = 0;

    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque"])));

    {
        spdlog::info("Building WireFrame PSO");
        auto tmp = psoDesc;
        tmp.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));
    }

    {
        spdlog::info("Building MSAA PSO");
        auto tmp = psoDesc;
        tmp.SampleDesc.Count = 4;
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mMSAAOpaquePSO)));
    }

    {
        spdlog::info("Building alpha_coverage PSO");
        // rgb  c_src * a_src + dst * (1 - a_src)
        // a   a_src * 1 + a_dst * 0
        auto tmp = psoDesc;
        tmp.BlendState.AlphaToCoverageEnable = true;
        tmp.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mPSOs["transparent_alpha_to_coverage"])));
    }

    {
        spdlog::info("Building blend PSO");
        // rgb  c_src * a_src + dst * (1 - a_src)
        // a   a_src * 1 + a_dst * 0
        auto tmp = psoDesc;
        D3D12_RENDER_TARGET_BLEND_DESC transparencyBlendDesc;
        transparencyBlendDesc.BlendEnable = true;
        transparencyBlendDesc.LogicOpEnable = false;
        transparencyBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        transparencyBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        transparencyBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        transparencyBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
        transparencyBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
        transparencyBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        transparencyBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
        transparencyBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
        tmp.BlendState.RenderTarget[0] = transparencyBlendDesc;
        tmp.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mPSOs["transparent_blend"])));
    }

    {
        spdlog::info("Building alpha clip PSO");
        auto tmp = psoDesc;
        tmp.PS = {(void *) (mShaders["alphaClip"]->GetBufferPointer()), mShaders["alphaClip"]->GetBufferSize()};
        tmp.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mPSOs["alphaClip"])));
    }
}

inline void BlendApp::BuildFrameResources() {
    for (int i = 0; i < kNumFrameResources; i++) {
        mFrameResources.push_back(std::make_unique<FrameResource>(mD3dDevice.Get(), 1, (UINT) mAllRitems.size()));
    }
}

inline void BlendApp::BuildRenderItems() {
    spdlog::info("Build Render Items");
    UINT objCBIndex = 0;

    {

        auto BoxItem = std::make_unique<RenderItem>();
        XMStoreFloat4x4(&BoxItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 1.5f, 5.0f));
        BoxItem->ObjectCBIndex = objCBIndex++;
        BoxItem->Geo = mGeometries["box"].get();
        BoxItem->TextureHandleIndex = 0;
        BoxItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = (BoxItem->Geo->DrawArgs.begin())->second;
        BoxItem->IndexCount = SubMeshInfo.IndexCount;
        BoxItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        BoxItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;
        mAllRitems.push_back(std::move(BoxItem));
    }

    {
        auto PlaneItem = std::make_unique<RenderItem>();
        XMStoreFloat4x4(&PlaneItem->World, XMMatrixScaling(20.0f, 1.0f, 20.0f) * XMMatrixTranslation(0.0f, -2.f, 0.0f));
        PlaneItem->ObjectCBIndex = objCBIndex++;
        PlaneItem->Geo = mGeometries["plane"].get();
        PlaneItem->TextureHandleIndex = 2;
        PlaneItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = (PlaneItem->Geo->DrawArgs.begin())->second;
        PlaneItem->IndexCount = SubMeshInfo.IndexCount;
        PlaneItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        PlaneItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;
        mAllRitems.push_back(std::move(PlaneItem));
    }
    spdlog::info("Build Opaque Render Items");
    // All the render items are opaque.
    for (auto &e : mAllRitems) mOpaqueRitems.push_back(e.get());

    spdlog::info("Build Transparent Render Items");
    std::vector<SimpleMath::Vector3> windows{
            SimpleMath::Vector3(-1.5f, 0.0f, -0.48f), SimpleMath::Vector3(1.5f, 0.0f, 0.51f),
            SimpleMath::Vector3(0.0f, 0.0f, 0.7f), SimpleMath::Vector3(-0.3f, 0.0f, -2.3f),
            SimpleMath::Vector3(0.5f, 0.0f, -0.6f)};

    for (int i = 0; i < 5; i++) {
        auto windowItem = std::make_unique<RenderItem>();
        XMStoreFloat4x4(&windowItem->World, XMMatrixRotationX(XMConvertToRadians(-90.0)) *
                                                    XMMatrixTranslation(windows[i].x, windows[i].y, windows[i].z));
        windowItem->ObjectCBIndex = objCBIndex++;
        windowItem->Geo = mGeometries["plane"].get();
        windowItem->TextureHandleIndex = 3;// window.png
        windowItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = (windowItem->Geo->DrawArgs.begin())->second;
        windowItem->IndexCount = SubMeshInfo.IndexCount;
        windowItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        windowItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;
        mAlphaBlendRitems.push_back(windowItem.get());
        mAllRitems.push_back(std::move(windowItem));
    }

    for (int i = 0; i < 5; i++) {
        auto windowItem = std::make_unique<RenderItem>();
        XMStoreFloat4x4(&windowItem->World, XMMatrixRotationX(XMConvertToRadians(-90.0)) *
                                                    XMMatrixTranslation(windows[i].x, windows[i].y, windows[i].z + 5));
        windowItem->ObjectCBIndex = objCBIndex++;
        windowItem->Geo = mGeometries["plane"].get();
        windowItem->TextureHandleIndex = 3;// window.png
        windowItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = (windowItem->Geo->DrawArgs.begin())->second;
        windowItem->IndexCount = SubMeshInfo.IndexCount;
        windowItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        windowItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;
        mAlphaCoverageRitems.push_back(windowItem.get());
        mAllRitems.push_back(std::move(windowItem));
    }

    for (int i = 0; i < 5; i++) {
        auto grassItem = std::make_unique<RenderItem>();
        XMStoreFloat4x4(&grassItem->World, XMMatrixRotationX(XMConvertToRadians(-90.0)) *
                                                   XMMatrixTranslation(windows[i].x, windows[i].y, windows[i].z - 5));
        grassItem->ObjectCBIndex = objCBIndex++;
        grassItem->Geo = mGeometries["plane"].get();
        grassItem->TextureHandleIndex = 4;// grass.png
        grassItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = (grassItem->Geo->DrawArgs.begin())->second;
        grassItem->IndexCount = SubMeshInfo.IndexCount;
        grassItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        grassItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;
        mAlphaClipRitems.push_back(grassItem.get());
        mAllRitems.push_back(std::move(grassItem));
    }
}

inline void BlendApp::LoadTextures() {
    auto woodCrateTex = std::make_unique<Texture>();
    woodCrateTex->Name = "0_WoodCrateTexture";
    woodCrateTex->Filename = "WoodCrate01.dds";
    woodCrateTex->Filename = fs::absolute(this->mResourceManager.find_path(woodCrateTex->Filename)).u8string();
    Texture::LoadAndUploadTexture(*woodCrateTex, mD3dDevice.Get(), mCommandList.Get());
    mTextures[woodCrateTex->Name] = std::move(woodCrateTex);

    auto faceTex = std::make_unique<Texture>();
    faceTex->Name = "1_face";
    faceTex->Filename = fs::absolute(this->mResourceManager.find_path(L"awesomeface.png")).u8string();
    Texture::LoadAndUploadTexture(*faceTex, mD3dDevice.Get(), mCommandList.Get(), /*flip*/ false);
    mTextures[faceTex->Name] = std::move(faceTex);

    auto woodTex = std::make_unique<Texture>();
    woodTex->Name = "2_wood";
    woodTex->Filename = fs::absolute(this->mResourceManager.find_path(L"wood.png")).u8string();
    Texture::LoadAndUploadTexture(*woodTex, mD3dDevice.Get(), mCommandList.Get(), /*flip*/ false);
    mTextures[woodTex->Name] = std::move(woodTex);

    auto windowTex = std::make_unique<Texture>();
    windowTex->Name = "3_window";
    windowTex->Filename = fs::absolute(this->mResourceManager.find_path(L"window.png")).u8string();
    Texture::LoadAndUploadTexture(*windowTex, mD3dDevice.Get(), mCommandList.Get(), /*flip*/ false);
    mTextures[windowTex->Name] = std::move(windowTex);

    auto grassTex = std::make_unique<Texture>();
    grassTex->Name = "4_grass";
    grassTex->Filename = fs::absolute(this->mResourceManager.find_path(L"grass.png")).u8string();
    Texture::LoadAndUploadTexture(*grassTex, mD3dDevice.Get(), mCommandList.Get(), /*flip*/ false);
    mTextures[grassTex->Name] = std::move(grassTex);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    BlendApp theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}