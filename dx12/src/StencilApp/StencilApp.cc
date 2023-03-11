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
    Material *Mat = nullptr;
    MeshGeometry *Geo = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    // draw instance params
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    int BaseVertexLocation = 0;
};


class StencilApp : public D3DApp {
public:
    StencilApp(HINSTANCE hInstance);
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
    void BuildMaterials();
    void LoadTextures();

    void UpdateObjectCB(const GameTimer &timer);
    void UpdateMaterialCBs(const GameTimer &timer);
    // 需要两个PassCB，因为Reflected的光照反向也要变化
    void UpdatePassCBs(const GameTimer &timer);
    void DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<const RenderItem *> &ritems);
    ComPtr<ID3D12DescriptorHeap> mCbvHeap;
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ResourcePathSearcher mResourceManager;

    std::unordered_map<std::string, ComPtr<ID3D10Blob>> mShaders;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

    std::unordered_map<std::string, std::unique_ptr<PD::Texture>> mTextures;
    std::unordered_map<std::string, int> mTextureSrvHandleIndices;

    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    std::vector<const RenderItem *> mOpaqueRitems;
    std::vector<const RenderItem *> mMirrorsRitems;
    std::vector<const RenderItem *> mTransparentRitems;
    std::vector<const RenderItem *> mReflectedRitems;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();
    UINT mPassCbvOffset = 0;
    int mCurrFrameResourceIndex = 0;

    FrameResource *mCurrFrameResource = nullptr;
    PassConstants mMainPassCB;
    PassConstants mReflectedPassCB;
    bool mbShowWireFrame = false;
    bool mbVsync = true;
};

inline StencilApp::StencilApp(HINSTANCE hInstance) : D3DApp(hInstance) {
    auto path = PD::ResourcePathSearcher::Path(PROJECT_DIR);
    if (!path.is_absolute()) { path = fs::absolute(path); }
    mResourceManager.add_path(path);
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "models");
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "textures");
    mResourceManager.add_path(path / L"Shaders");
    mResourceManager.add_path(path / ".." / ".." / ".." / "dx11" / "cpp-dx11" / "resources" / "models" / "spot");
}

inline void StencilApp::ReleaseResource() {
    mRootSignature.Reset();
    mCbvHeap.Reset();
}

inline bool StencilApp::Initialize() {
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
    BuildMaterials();
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

    // dispose all uploaders
    // for (auto &it : mGeometries) { it.second->DisposeUploaders(); }

    return true;
}

inline void StencilApp::Update(const GameTimer &timer) {

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
    UpdatePassCBs(timer);
    UpdateMaterialCBs(timer);

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
    ImGui::End();
    ImGui::Render();
}

void StencilApp::UpdateObjectCB(const GameTimer &timer) {
    auto currObjectCB = mCurrFrameResource->ObjectCB.get();
    for (auto &element : mAllRitems) {
        // upload data when needed
        if (element->NumFramesDirty > 0) {
            XMMATRIX world = XMLoadFloat4x4(&element->World);
            ObjectConstants objCB;
            XMStoreFloat4x4(&objCB.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&objCB.InvTransWorld, XMMatrixTranspose(MathHelper::InverseTranspose(world)));

            XMStoreFloat4x4(&objCB.TexTransform, XMLoadFloat4x4(&MathHelper::Identity4x4()));
            currObjectCB->CopyData(element->ObjectCBIndex, objCB);
            element->NumFramesDirty--;
        }
    }
}
inline void StencilApp::UpdateMaterialCBs(const GameTimer &timer) {

    auto currMaterialCB = mCurrFrameResource->MaterialCB.get();
    for (auto &element : mMaterials) {
        // upload data when needed
        Material *mat = element.second.get();
        if (mat->NumFramesDirty > 0) {
            MaterialConstants matConstants;
            matConstants.DiffuseAlbedo = mat->DiffuseAlbedo;
            matConstants.FresnelR0 = mat->FresnelR0;
            matConstants.Roughness = mat->Roughness;
            currMaterialCB->CopyData(mat->MatCBIndex, matConstants);
            mat->NumFramesDirty--;
        }
    }
}
inline void StencilApp::UpdatePassCBs(const GameTimer &gt) {
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
    mMainPassCB.AmbientLight = {0.25, 0.25, 0.35, 1.0};

    mMainPassCB.Lights[0].Direction = {0.57735f, -0.57735f, 0.57735f};
    mMainPassCB.Lights[0].Strength = {0.6f, 0.6f, 0.6f};

    mMainPassCB.Lights[1].Direction = {-0.57735f, -0.57735f, 0.57735f};
    mMainPassCB.Lights[1].Strength = {0.3f, 0.3f, 0.3f};

    mMainPassCB.Lights[2].Direction = {0.707f, 0.f, -0.707f};
    mMainPassCB.Lights[2].Strength = {0.55f, 0.15f, 0.15f};

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
    mReflectedPassCB = mMainPassCB;

    XMVECTOR mirrorPlane = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);
    XMMATRIX R = XMMatrixReflect(mirrorPlane);

    // Reflect the lighting.
    for (int i = 0; i < 3; ++i) {
        XMVECTOR lightDir = XMLoadFloat3(&mMainPassCB.Lights[i].Direction);
        XMVECTOR reflectedLightDir = XMVector3TransformNormal(lightDir, R);
        XMStoreFloat3(&mReflectedPassCB.Lights[i].Direction, reflectedLightDir);
    }

    currPassCB->CopyData(1, mReflectedPassCB);
}

inline void StencilApp::DrawRenderItems(ID3D12GraphicsCommandList *cmdList,
                                        const std::vector<const RenderItem *> &ritems) {

    uint32_t objCBByteSize = CalcConstantBufferBytesSize(sizeof(ObjectConstants));
    uint32_t matCBByteSize = CalcConstantBufferBytesSize(sizeof(MaterialConstants));
    auto objectCB = mCurrFrameResource->ObjectCB->Resource();
    auto matCB = mCurrFrameResource->MaterialCB->Resource();
    for (size_t i = 0; i < ritems.size(); i++) {
        // 要绘制一个物体，需要物体本身的信息，和它有关的model
        // 先设置顶点信息
        auto ri = ritems[i];
        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        D3D12_GPU_VIRTUAL_ADDRESS ObjCBStartAddress = objectCB->GetGPUVirtualAddress();
        D3D12_GPU_VIRTUAL_ADDRESS MatCBStartAddress = matCB->GetGPUVirtualAddress();
        auto ObjCBAddress = ObjCBStartAddress + ri->ObjectCBIndex * objCBByteSize;
        auto MatCBAddress = MatCBStartAddress + ri->Mat->MatCBIndex * matCBByteSize;

        CD3DX12_GPU_DESCRIPTOR_HANDLE tex(mSrvDescriptorHeap->GetGPUDescriptorHandleForHeapStart());
        tex.Offset(ri->Mat->DiffuseSrvHeapIndex, mCbvSrvUavDescriptorSize);
        cmdList->SetGraphicsRootConstantBufferView(0, ObjCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(1, MatCBAddress);
        cmdList->SetGraphicsRootDescriptorTable(3, tex);
        assert(ri->IndexCount > 0);
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}


void StencilApp::Draw(const GameTimer &gt) {


    // 取出当前帧的Alloc,然后把cmdlist关联到这个alloc
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    HR(cmdListAlloc->Reset());

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

    auto passCBAddr = mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress();
    auto passCBStride = CalcConstantBufferBytesSize(sizeof(PassConstants));

    std::vector<ID3D12DescriptorHeap *> descriptorHeaps{mSrvDescriptorHeap.Get()};
    mCommandList->SetDescriptorHeaps(descriptorHeaps.size(), descriptorHeaps.data());
    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());
    mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddr);

    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

    if (GetMSAAState()) {
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

    // stage 2 render stencil

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());
    mCommandList->SetPipelineState(mPSOs.at("markStencilMirrors").Get());
    mCommandList->OMSetStencilRef(0x01);
    DrawRenderItems(mCommandList.Get(), mMirrorsRitems);

    // stage 3 render reflected
    mCommandList->SetPipelineState(mPSOs.at("drawStencilReflections").Get());
    // 因为物体反转以后，光源也要跟着反转，而光源保存在passCB里，所以需要两个passCB
    mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddr + 1 * passCBStride);
    DrawRenderItems(mCommandList.Get(), mReflectedRitems);

    // stage 4 render mirror
    mCommandList->SetGraphicsRootConstantBufferView(2, passCBAddr);
    mCommandList->SetPipelineState(mPSOs.at("transparent_blend").Get());
    DrawRenderItems(mCommandList.Get(), mTransparentRitems);

    mCommandList->OMSetStencilRef(0x0);
    // stage last: render imgui
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

inline void StencilApp::BuildDescriptorHeaps() {
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


    for (const auto &it : mTextures) {
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mSrvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());
        srvHandle.Offset(mTextureSrvHandleIndices.at(it.first), mCbvSrvUavDescriptorSize);

        auto resource = it.second->Resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        mD3dDevice->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
    }
}

inline void StencilApp::BuildConstantBuffers() {
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

inline void StencilApp::BuildRootSignature() {
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];
    // 一个放Object，一个放PassCB,一个放MatCB
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);

    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
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

inline void StencilApp::BuildShaderAndInputLayout() {
    spdlog::info("Bulding Shaders");
    auto ShaderPath = mResourceManager.find_path("color.hlsl");

    auto VSBlob = CompileShader(ShaderPath, nullptr, "VSMain", "vs_5_0");
    auto PSBlob = CompileShader(ShaderPath, nullptr, "PSMain", "ps_5_0");
    assert(VSBlob);
    assert(PSBlob);
    mShaders["standardVS"] = VSBlob;
    mShaders["opaquePS"] = PSBlob;

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

inline void StencilApp::BuildShapeGeometry() {
    spdlog::info("Bulding Shape Geometries");
    {
        auto SkullMesh = LoadLunaFileFromFile(mResourceManager.find_path("skull.txt"));
        auto geo = std::make_unique<MeshGeometry>();
        geo->name = "Skull";

        std::vector<Vertex> vertices;
        std::vector<std::uint16_t> indices;
        for (size_t i = 0; i < SkullMesh.mPoses.size(); ++i) {
            Vertex v;
            v.Pos = SkullMesh.mPoses[i];
            v.Normal = SkullMesh.mNormals[i];
            v.TexC = DirectX::XMFLOAT2(0.5, 0.5);
            vertices.push_back(v);
        }
        indices.insert(indices.end(), std::begin(SkullMesh.mIndices16), std::end(SkullMesh.mIndices16));
        const UINT vbByteSize = (UINT) vertices.size() * sizeof(Vertex);
        const UINT ibByteSize = (UINT) indices.size() * sizeof(std::uint16_t);

        ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
        CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

        ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
        CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

        geo->VertexBufferGPU = CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize,
                                                   geo->VertexBufferUploader);

        geo->IndexBufferGPU = CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize,
                                                  geo->IndexBufferUploader);

        geo->VertexBytesStride = sizeof(Vertex);
        geo->VertexBufferByteSize = vbByteSize;
        geo->IndexFormat = DXGI_FORMAT_R16_UINT;
        geo->IndexBufferBytesSize = ibByteSize;

        SubmeshGeometry SkullSubmesh;
        SkullSubmesh.StartIndexLocation = 0;
        SkullSubmesh.BaseVertexLocation = 0;
        SkullSubmesh.IndexCount = SkullMesh.mIndices32.size();
        geo->DrawArgs["skull"] = SkullSubmesh;
        mGeometries[geo->name] = std::move(geo);
    }
    {
        std::array<Vertex, 20> vertices = {// Floor: Observe we tile texture coordinates.
                                           Vertex(-3.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 0.0f, 4.0f),// 0
                                           Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f),
                                           Vertex(7.5f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 4.0f, 0.0f),
                                           Vertex(7.5f, 0.0f, -10.0f, 0.0f, 1.0f, 0.0f, 4.0f, 4.0f),

                                           // Wall: Observe we tile texture coordinates, and that we
                                           // leave a gap in the middle for the mirror.
                                           Vertex(-3.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f),// 4
                                           Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
                                           Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 0.0f),
                                           Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.5f, 2.0f),

                                           Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 2.0f),// 8
                                           Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
                                           Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 0.0f),
                                           Vertex(7.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 2.0f, 2.0f),

                                           Vertex(-3.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f),// 12
                                           Vertex(-3.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
                                           Vertex(7.5f, 6.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 0.0f),
                                           Vertex(7.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 6.0f, 1.0f),

                                           // Mirror
                                           Vertex(-2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f),// 16
                                           Vertex(-2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f),
                                           Vertex(2.5f, 4.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f),
                                           Vertex(2.5f, 0.0f, 0.0f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f)};

        std::array<std::int16_t, 30> indices = {// Floor
                                                0, 1, 2, 0, 2, 3,

                                                // Walls
                                                4, 5, 6, 4, 6, 7,

                                                8, 9, 10, 8, 10, 11,

                                                12, 13, 14, 12, 14, 15,

                                                // Mirror
                                                16, 17, 18, 16, 18, 19};

        SubmeshGeometry floorSubmesh;
        floorSubmesh.IndexCount = 6;
        floorSubmesh.StartIndexLocation = 0;
        floorSubmesh.BaseVertexLocation = 0;

        SubmeshGeometry wallSubmesh;
        wallSubmesh.IndexCount = 18;
        wallSubmesh.StartIndexLocation = 6;
        wallSubmesh.BaseVertexLocation = 0;

        SubmeshGeometry mirrorSubmesh;
        mirrorSubmesh.IndexCount = 6;
        mirrorSubmesh.StartIndexLocation = 24;
        mirrorSubmesh.BaseVertexLocation = 0;

        const UINT vbByteSize = (UINT) vertices.size() * sizeof(Vertex);
        const UINT ibByteSize = (UINT) indices.size() * sizeof(std::uint16_t);

        auto geo = std::make_unique<MeshGeometry>();
        geo->name = "roomGeo";

        ThrowIfFailed(D3DCreateBlob(vbByteSize, &geo->VertexBufferCPU));
        CopyMemory(geo->VertexBufferCPU->GetBufferPointer(), vertices.data(), vbByteSize);

        ThrowIfFailed(D3DCreateBlob(ibByteSize, &geo->IndexBufferCPU));
        CopyMemory(geo->IndexBufferCPU->GetBufferPointer(), indices.data(), ibByteSize);

        geo->VertexBufferGPU = CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), vertices.data(), vbByteSize,
                                                   geo->VertexBufferUploader);

        geo->IndexBufferGPU = CreateDefaultBuffer(mD3dDevice.Get(), mCommandList.Get(), indices.data(), ibByteSize,
                                                  geo->IndexBufferUploader);

        geo->VertexBytesStride = sizeof(Vertex);
        geo->VertexBufferByteSize = vbByteSize;
        geo->IndexFormat = DXGI_FORMAT_R16_UINT;
        geo->IndexBufferBytesSize = ibByteSize;

        geo->DrawArgs["floor"] = floorSubmesh;
        geo->DrawArgs["wall"] = wallSubmesh;
        geo->DrawArgs["mirror"] = mirrorSubmesh;

        mGeometries[geo->name] = std::move(geo);
    }
}

inline void StencilApp::BuildPSOs() {

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
        spdlog::info("Building mask stencil ");

        auto tmp = psoDesc;
        CD3DX12_BLEND_DESC mirrorBlendState(D3D12_DEFAULT);
        mirrorBlendState.RenderTarget[0].RenderTargetWriteMask = 0x00;
        tmp.BlendState = mirrorBlendState;

        CD3DX12_DEPTH_STENCIL_DESC mirrorDSS(D3D12_DEFAULT);
        mirrorDSS.DepthEnable = true;
        mirrorDSS.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ZERO;// 不写入深度
        mirrorDSS.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
        mirrorDSS.StencilEnable = true;
        mirrorDSS.StencilReadMask = 0xff;
        mirrorDSS.StencilWriteMask = 0xff;
        // 当stencil pass的时候替换value
        mirrorDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        mirrorDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        mirrorDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_REPLACE;
        mirrorDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_ALWAYS;
        tmp.DepthStencilState = mirrorDSS;

        // 在绘制时需要设置StencilRef为1，这样通过的时候，会把对应的Pixel的位置设置为1
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mPSOs["markStencilMirrors"])));
    }
    {
        spdlog::info("Building reflection PSO");

        auto tmp = psoDesc;

        CD3DX12_DEPTH_STENCIL_DESC reflectionDSS(D3D12_DEFAULT);
        reflectionDSS.StencilEnable = true;
        reflectionDSS.StencilReadMask = 0xff;
        reflectionDSS.StencilWriteMask = 0x00;// no need to write
        // 当stencil pass的时候替换value
        reflectionDSS.FrontFace.StencilFailOp = D3D12_STENCIL_OP_KEEP;
        reflectionDSS.FrontFace.StencilDepthFailOp = D3D12_STENCIL_OP_KEEP;
        reflectionDSS.FrontFace.StencilPassOp = D3D12_STENCIL_OP_KEEP;
        reflectionDSS.FrontFace.StencilFunc = D3D12_COMPARISON_FUNC_EQUAL;
        tmp.DepthStencilState = reflectionDSS;

        tmp.RasterizerState.FrontCounterClockwise = true;
        // 在绘制时需要设置StencilRef为1，这样通过的时候，会把对应的Pixel的位置设置为1
        HR(mD3dDevice->CreateGraphicsPipelineState(&tmp, IID_PPV_ARGS(&mPSOs["drawStencilReflections"])));
    }
}

inline void StencilApp::BuildFrameResources() {
    for (int i = 0; i < kNumFrameResources; i++) {
        mFrameResources.push_back(std::make_unique<FrameResource>(mD3dDevice.Get(), /*pass count */ 2,
                                                                  (UINT) mAllRitems.size(), mMaterials.size()));
    }
}

inline void StencilApp::BuildRenderItems() {
    spdlog::info("Build Render Items");
    UINT objCBIndex = 0;

    {
        auto FloorItem = std::make_unique<RenderItem>();
        XMStoreFloat4x4(&FloorItem->World, XMMatrixRotationY(XMConvertToRadians(-90.0f)));
        FloorItem->ObjectCBIndex = objCBIndex++;
        FloorItem->Geo = mGeometries["roomGeo"].get();
        FloorItem->Mat = mMaterials.at("checkboard").get();
        FloorItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = FloorItem->Geo->DrawArgs["floor"];
        FloorItem->IndexCount = SubMeshInfo.IndexCount;
        FloorItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        FloorItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;
        mOpaqueRitems.push_back(FloorItem.get());
        mAllRitems.push_back(std::move(FloorItem));
    }

    {
        auto WallItem = std::make_unique<RenderItem>();
        XMStoreFloat4x4(&WallItem->World, XMMatrixRotationY(XMConvertToRadians(-90.0f)));
        WallItem->ObjectCBIndex = objCBIndex++;
        WallItem->Geo = mGeometries["roomGeo"].get();
        WallItem->Mat = mMaterials.at("bricks").get();
        WallItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = WallItem->Geo->DrawArgs["wall"];
        WallItem->IndexCount = SubMeshInfo.IndexCount;
        WallItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        WallItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;
        mOpaqueRitems.push_back(WallItem.get());
        mAllRitems.push_back(std::move(WallItem));
    }

    {
        auto SkullItem = std::make_unique<RenderItem>();
        SimpleMath::Vector3 offset(5, 0, 0);
        XMMATRIX SkullTrans = XMMatrixScaling(0.5, 0.5l, 0.5) * XMMatrixTranslation(offset.x, offset.y, offset.z);
        XMStoreFloat4x4(&SkullItem->World, SkullTrans);
        SkullItem->ObjectCBIndex = objCBIndex++;
        SkullItem->Geo = mGeometries["Skull"].get();
        SkullItem->Mat = mMaterials.at("dummywhite").get();
        SkullItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = (SkullItem->Geo->DrawArgs.begin())->second;
        SkullItem->IndexCount = SubMeshInfo.IndexCount;
        SkullItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        SkullItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;

        auto ReflectedSkullItem = std::make_unique<RenderItem>();
        *ReflectedSkullItem = *SkullItem;
        ReflectedSkullItem->ObjectCBIndex = objCBIndex++;
        XMVECTOR mirrorPlane = XMVectorSet(1.0f, 0.0f, 0.0f, 0.0f);// xz-plane
        XMMATRIX RMatrix = XMMatrixReflect(mirrorPlane);
        XMStoreFloat4x4(&ReflectedSkullItem->World, SkullTrans * RMatrix);

        mOpaqueRitems.push_back(SkullItem.get());
        mAllRitems.push_back(std::move(SkullItem));
        mReflectedRitems.push_back(ReflectedSkullItem.get());
        mAllRitems.push_back(std::move(ReflectedSkullItem));
    }

    {
        auto MirrorItem = std::make_unique<RenderItem>();
        XMStoreFloat4x4(&MirrorItem->World, XMMatrixRotationY(XMConvertToRadians(-90.0f)));
        MirrorItem->ObjectCBIndex = objCBIndex++;
        MirrorItem->Geo = mGeometries["roomGeo"].get();
        MirrorItem->Mat = mMaterials.at("icemirror").get();
        MirrorItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        const auto &SubMeshInfo = MirrorItem->Geo->DrawArgs["mirror"];
        MirrorItem->IndexCount = SubMeshInfo.IndexCount;
        MirrorItem->StartIndexLocation = SubMeshInfo.StartIndexLocation;
        MirrorItem->BaseVertexLocation = SubMeshInfo.BaseVertexLocation;
        mMirrorsRitems.push_back(MirrorItem.get());
        mTransparentRitems.push_back(MirrorItem.get());
        mAllRitems.push_back(std::move(MirrorItem));
    }
}

inline void StencilApp::BuildMaterials() {
    int matIndex = 0;
    {
        auto DummyWhite = std::make_unique<Material>();
        DummyWhite->Name = "dummywhite";
        DummyWhite->MatCBIndex = matIndex++;
        DummyWhite->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("dummy");
        mMaterials["dummywhite"] = std::move(DummyWhite);
    }

    {
        auto bricks = std::make_unique<Material>();
        bricks->Name = "bricks";
        bricks->MatCBIndex = matIndex++;
        bricks->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("bricks");
        mMaterials["bricks"] = std::move(bricks);
    }

    {
        auto checkertile = std::make_unique<Material>();
        checkertile->Name = "checkboard";
        checkertile->MatCBIndex = matIndex++;
        checkertile->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("checkboard");
        mMaterials["checkboard"] = std::move(checkertile);
    }
    {
        auto icemirror = std::make_unique<Material>();
        icemirror->Name = "icemirror";
        icemirror->MatCBIndex = matIndex++;
        icemirror->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("ice");
        icemirror->DiffuseAlbedo.w = 0.5f;
        mMaterials[icemirror->Name] = std::move(icemirror);
    }
}

inline void StencilApp::LoadTextures() {
    int texIndex = 0;
    {
        auto dummyTex = std::make_unique<Texture>();
        dummyTex->Name = "dummy";
        dummyTex->Filename = dummyTex->Name;
        CreateDummy1x1Texture(*dummyTex, mD3dDevice.Get(), mCommandList.Get(), XMFLOAT4(1, 1, 1, 1));
        mTextureSrvHandleIndices[dummyTex->Name] = texIndex++;
        mTextures[dummyTex->Name] = std::move(dummyTex);
    }

    {
        auto bricksTex = std::make_unique<Texture>();
        bricksTex->Name = "bricks";
        bricksTex->Filename = "bricks3.dds";
        bricksTex->Filename = fs::absolute(this->mResourceManager.find_path(bricksTex->Filename)).u8string();
        Texture::LoadAndUploadTexture(*bricksTex, mD3dDevice.Get(), mCommandList.Get());
        mTextureSrvHandleIndices[bricksTex->Name] = texIndex++;
        mTextures[bricksTex->Name] = std::move(bricksTex);
    }

    {
        auto iceTex = std::make_unique<Texture>();
        iceTex->Name = "ice";
        iceTex->Filename = "ice.dds";
        iceTex->Filename = fs::absolute(this->mResourceManager.find_path(iceTex->Filename)).u8string();
        Texture::LoadAndUploadTexture(*iceTex, mD3dDevice.Get(), mCommandList.Get());
        mTextureSrvHandleIndices[iceTex->Name] = texIndex++;
        mTextures[iceTex->Name] = std::move(iceTex);
    }

    {
        auto checkboardTex = std::make_unique<Texture>();
        checkboardTex->Name = "checkboard";
        checkboardTex->Filename = "checkboard.dds";
        checkboardTex->Filename = fs::absolute(this->mResourceManager.find_path(checkboardTex->Filename)).u8string();
        Texture::LoadAndUploadTexture(*checkboardTex, mD3dDevice.Get(), mCommandList.Get());
        mTextureSrvHandleIndices[checkboardTex->Name] = texIndex++;
        mTextures[checkboardTex->Name] = std::move(checkboardTex);
    }
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    StencilApp theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}