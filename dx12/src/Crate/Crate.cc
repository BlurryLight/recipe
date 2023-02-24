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


class CrateApp : public D3DApp {
public:
    CrateApp(HINSTANCE hInstance);
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
    void UpdateMainPassCB(const GameTimer &timer);
    void DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<const RenderItem *> &ritems);
    std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> GetStaticSamplers();
    ComPtr<ID3D12DescriptorHeap> mCbvHeap;
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ResourcePathSearcher mResourceManager;

    std::unordered_map<std::string, ComPtr<ID3D10Blob>> mShaders;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;

    std::unordered_map<std::string, std::unique_ptr<PD::Texture>> mTextures;
    ComPtr<ID3D12DescriptorHeap> mSrvDescriptorHeap = nullptr;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    std::vector<const RenderItem *> mOpaqueRitems;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();
    UINT mPassCbvOffset = 0;
    int mCurrFrameResourceIndex = 0;

    FrameResource *mCurrFrameResource = nullptr;
    PassConstants mMainPassCB;
    bool mbShowWireFrame = false;
    bool mbVsync = true;
    bool mbAnotherTexture = false;

    // scale x scale y transx trans y
    SimpleMath::Vector4 mCrateTextureTrans = SimpleMath::Vector4(1, 1, 0, 0);
};

inline CrateApp::CrateApp(HINSTANCE hInstance) : D3DApp(hInstance) {
    auto path = PD::ResourcePathSearcher::Path(PROJECT_DIR);
    if (!path.is_absolute()) { path = fs::absolute(path); }
    mResourceManager.add_path(path);
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "models");
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "textures");
    mResourceManager.add_path(path / L"Shaders");
    mResourceManager.add_path(path / ".." / ".." / ".." / "dx11" / "cpp-dx11" / "resources" / "models" / "spot");
}

inline void CrateApp::ReleaseResource() {
    mRootSignature.Reset();
    mCbvHeap.Reset();
}

inline bool CrateApp::Initialize() {
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

    return true;
}

inline void CrateApp::Update(const GameTimer &timer) {

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
    ImGui::Checkbox("Another Texture", &mbAnotherTexture);
    if (ImGui::SliderFloat4("CrateTextureTrans", (float *) &mCrateTextureTrans, 0.0f, 2.0f)) {
        // 刷新所有缓冲区的属性
        mAllRitems[0]->NumFramesDirty = kNumFrameResources;
    }

    ImGui::End();
    ImGui::Render();
}

void CrateApp::UpdateObjectCB(const GameTimer &timer) {
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
inline void CrateApp::UpdateMaterialCBs(const GameTimer &timer) {

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
inline void CrateApp::UpdateMainPassCB(const GameTimer &gt) {
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
    mMainPassCB.Lights[2].Direction = {0.0f, -0.707f, -0.707f};
    mMainPassCB.Lights[2].Strength = {0.15f, 0.15f, 0.15f};

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

inline void CrateApp::DrawRenderItems(ID3D12GraphicsCommandList *cmdList,
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
        tex.Offset(ri->Mat->DiffuseSrvHeapIndex + (mbAnotherTexture ? 1 : 0), mCbvSrvUavDescriptorSize);
        cmdList->SetGraphicsRootConstantBufferView(0, ObjCBAddress);
        cmdList->SetGraphicsRootConstantBufferView(1, MatCBAddress);
        cmdList->SetGraphicsRootDescriptorTable(3, tex);
        assert(ri->IndexCount > 0);
        cmdList->DrawIndexedInstanced(ri->IndexCount, 1, ri->StartIndexLocation, ri->BaseVertexLocation, 0);
    }
}

inline std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> CrateApp::GetStaticSamplers() {
    // Applications usually only need a handful of samplers.  So just define them all up front
    // and keep them available as part of the root signature.

    const CD3DX12_STATIC_SAMPLER_DESC pointWrap(0,                               // shaderRegister
                                                D3D12_FILTER_MIN_MAG_MIP_POINT,  // filter
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
                                                D3D12_TEXTURE_ADDRESS_MODE_WRAP);// addressW

    const CD3DX12_STATIC_SAMPLER_DESC pointClamp(1,                                // shaderRegister
                                                 D3D12_FILTER_MIN_MAG_MIP_POINT,   // filter
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
                                                 D3D12_TEXTURE_ADDRESS_MODE_CLAMP);// addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearWrap(2,                               // shaderRegister
                                                 D3D12_FILTER_MIN_MAG_MIP_LINEAR, // filter
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressU
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP, // addressV
                                                 D3D12_TEXTURE_ADDRESS_MODE_WRAP);// addressW

    const CD3DX12_STATIC_SAMPLER_DESC linearClamp(3,                                // shaderRegister
                                                  D3D12_FILTER_MIN_MAG_MIP_LINEAR,  // filter
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressU
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP, // addressV
                                                  D3D12_TEXTURE_ADDRESS_MODE_CLAMP);// addressW

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicWrap(4,                              // shaderRegister
                                                      D3D12_FILTER_ANISOTROPIC,       // filter
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,// addressU
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,// addressV
                                                      D3D12_TEXTURE_ADDRESS_MODE_WRAP,// addressW
                                                      0.0f,                           // mipLODBias
                                                      8);                             // maxAnisotropy

    const CD3DX12_STATIC_SAMPLER_DESC anisotropicClamp(5,                               // shaderRegister
                                                       D3D12_FILTER_ANISOTROPIC,        // filter
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// addressU
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// addressV
                                                       D3D12_TEXTURE_ADDRESS_MODE_CLAMP,// addressW
                                                       0.0f,                            // mipLODBias
                                                       8);                              // maxAnisotropy

    return {pointWrap, pointClamp, linearWrap, linearClamp, anisotropicWrap, anisotropicClamp};
}

void CrateApp::Draw(const GameTimer &gt) {


    // 取出当前帧的Alloc,然后把cmdlist关联到这个alloc
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    HR(cmdListAlloc->Reset());

    auto SwitchPSO = [this]() {
        if (GetMSAAState()) { return mMSAAPSO.Get(); }
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
    mCommandList->SetGraphicsRootConstantBufferView(2, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());

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

inline void CrateApp::BuildDescriptorHeaps() {
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
    int index = 0;
    for (const auto &it : mTextures) {

        srvHandle.Offset(index, mCbvSrvUavDescriptorSize);
        auto resource = it.second->Resource;
        D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
        srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        srvDesc.Format = resource->GetDesc().Format;
        srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        srvDesc.Texture2D.MostDetailedMip = 0;
        srvDesc.Texture2D.MipLevels = resource->GetDesc().MipLevels;
        srvDesc.Texture2D.ResourceMinLODClamp = 0.0f;
        mD3dDevice->CreateShaderResourceView(resource.Get(), &srvDesc, srvHandle);
        index++;
    }
}

inline void CrateApp::BuildConstantBuffers() {
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

inline void CrateApp::BuildRootSignature() {
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];
    // 一个放Object，一个放PassCB,一个放MatCB
    slotRootParameter[0].InitAsConstantBufferView(0);
    slotRootParameter[1].InitAsConstantBufferView(1);
    slotRootParameter[2].InitAsConstantBufferView(2);

    CD3DX12_DESCRIPTOR_RANGE texTable;
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);
    auto staticSamplers = GetStaticSamplers();

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

inline void CrateApp::BuildShaderAndInputLayout() {
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

inline void CrateApp::BuildShapeGeometry() {
    spdlog::info("Bulding Shape Geometries");

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 1.5f, 1.5f, 3);

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
    UINT boxVertexOffset = 0;

    // Cache the starting index for each object in the concatenated index buffer.
    UINT boxIndexOffset = 0;

    // Define the SubmeshGeometry that cover different
    // regions of the vertex/index buffers.

    SubmeshGeometry boxSubmesh;
    boxSubmesh.IndexCount = (UINT) box.Indices32.size();
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;

    auto totalVertexCount = box.Vertices.size();

    std::vector<Vertex> vertices(totalVertexCount);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Normal = box.Vertices[i].Normal;
        vertices[k].TexC = box.Vertices[i].TexC;
    }


    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));


    const UINT vbByteSize = (UINT) vertices.size() * sizeof(Vertex);
    const UINT ibByteSize = (UINT) indices.size() * sizeof(std::uint16_t);

    auto geo = std::make_unique<MeshGeometry>();
    geo->name = "shapeGeo";

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

    geo->DrawArgs["box"] = boxSubmesh;

    mGeometries[geo->name] = std::move(geo);
}

inline void CrateApp::BuildPSOs() {

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

    spdlog::info("Building WireFrame PSO");
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_WIREFRAME;
    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mPSOs["opaque_wireframe"])));

    spdlog::info("Building MSAA PSO");
    psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
    psoDesc.SampleDesc.Count = 4;
    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mMSAAPSO)));
}

inline void CrateApp::BuildFrameResources() {
    for (int i = 0; i < kNumFrameResources; i++) {
        mFrameResources.push_back(
                std::make_unique<FrameResource>(mD3dDevice.Get(), 1, (UINT) mAllRitems.size(), mMaterials.size()));
    }
}

inline void CrateApp::BuildRenderItems() {
    spdlog::info("Build Render Items");
    UINT objCBIndex = 0;

    auto BoxItem = std::make_unique<RenderItem>();
    XMStoreFloat4x4(&BoxItem->World, XMMatrixScaling(2.0f, 2.0f, 2.0f) * XMMatrixTranslation(0.0f, 0.5f, 0.0f));
    BoxItem->ObjectCBIndex = objCBIndex++;
    BoxItem->Geo = mGeometries["shapeGeo"].get();
    BoxItem->Mat = mMaterials["WoodCrate"].get();
    BoxItem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    const auto &BoxSubMeshInfo = BoxItem->Geo->DrawArgs["box"];
    BoxItem->IndexCount = BoxSubMeshInfo.IndexCount;
    BoxItem->StartIndexLocation = BoxSubMeshInfo.StartIndexLocation;
    BoxItem->BaseVertexLocation = BoxSubMeshInfo.BaseVertexLocation;
    mAllRitems.push_back(std::move(BoxItem));

    spdlog::info("Build Opaque Render Items");
    // All the render items are opaque.
    for (auto &e : mAllRitems) mOpaqueRitems.push_back(e.get());
}

inline void CrateApp::BuildMaterials() {
    auto WoodCrate = std::make_unique<Material>();
    WoodCrate->Name = "WoodCrate";
    WoodCrate->MatCBIndex = 0;
    WoodCrate->DiffuseSrvHeapIndex = 0;
    WoodCrate->DiffuseAlbedo = XMFLOAT4(1.0, 1.0, 1.0, 1.0);
    WoodCrate->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    WoodCrate->Roughness = 0.2f;

    mMaterials[WoodCrate->Name] = std::move(WoodCrate);
}

inline void CrateApp::LoadTextures() {
    auto woodCrateTex = std::make_unique<Texture>();
    woodCrateTex->Name = "WoodCrateTexture";
    woodCrateTex->Filename = "WoodCrate01.dds";
    woodCrateTex->Filename = fs::absolute(this->mResourceManager.find_path(woodCrateTex->Filename)).u8string();
    Texture::LoadAndUploadTexture(*woodCrateTex, mD3dDevice.Get(), mCommandList.Get());
    mTextures[woodCrateTex->Name] = std::move(woodCrateTex);

    // init texture

    auto woodTex = std::make_unique<Texture>();
    woodTex->Name = "Wood";
    woodTex->Filename = fs::absolute(this->mResourceManager.find_path(L"wood.png")).u8string();
    Texture::LoadAndUploadTexture(*woodTex, mD3dDevice.Get(), mCommandList.Get());
    mTextures[woodTex->Name] = std::move(woodTex);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE prevInstance, PSTR cmdLine, int showCmd) {

#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
#endif

    AllocConsole();
    freopen("CONIN$", "r", stdin);
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    CrateApp theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}