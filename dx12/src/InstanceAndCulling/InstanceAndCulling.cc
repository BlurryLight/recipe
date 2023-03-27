#include "FrameResource.h"
#include "GeometryGenerator.h"
#include "MathHelper.h"
#include "UploadBuffer.hh"
#include "resource_path_searcher.h"
#include <MeshGeometry.hh>
#include <array>
#include <d3dApp.hh>
#include <spdlog/spdlog.h>


#include "imgui.h"
#include "imgui_impl_dx12.h"
#include "imgui_impl_win32.h"


using namespace PD;

using namespace DirectX;

using DirectX::PackedVector::XMCOLOR;
const static int kNumFrameResources = 3;
const int instanceCount = 100;
// RenderItem is related to per object
struct RenderItem {
    // init: every resource needs to be updated
    int NumFramesDirty = kNumFrameResources;

    MeshGeometry *Geo = nullptr;
    D3D12_PRIMITIVE_TOPOLOGY PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

    std::vector<InstanceDataCPU> instanceData;
    // draw instance params
    UINT IndexCount = 0;
    UINT StartIndexLocation = 0;
    UINT InstanceCount = 1;
    BoundingBox Bounds;
    int BaseVertexLocation = 0;
};


class LightColumnsApp : public D3DApp {
public:
    LightColumnsApp(HINSTANCE hInstance);
    virtual void ReleaseResource() override;
    virtual bool Initialize() override;
    virtual void OnResizeCallback() override;
    virtual void OnMouseDownCallback(WPARAM btnState, int x, int y) override;
    void Update(const GameTimer &timer) override;
    void Draw(const GameTimer &gt) override;
    void AABBPick(int x, int y);

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

    void UpdateObjectInstanceData(const GameTimer &timer);
    void UpdateMaterialBuffers(const GameTimer &timer);
    void UpdateMainPassCB(const GameTimer &timer);
    void DrawRenderItems(ID3D12GraphicsCommandList *cmdList, const std::vector<const RenderItem *> &ritems);
    ComPtr<ID3D12DescriptorHeap> mCbvHeap = nullptr;
    ComPtr<ID3D12DescriptorHeap> mSrvHeap;
    ComPtr<ID3D12RootSignature> mRootSignature = nullptr;
    ResourcePathSearcher mResourceManager;

    std::unordered_map<std::string, ComPtr<ID3D10Blob>> mShaders;
    std::unordered_map<std::string, std::unique_ptr<MeshGeometry>> mGeometries;
    std::unordered_map<std::string, ComPtr<ID3D12PipelineState>> mPSOs;
    std::vector<std::unique_ptr<FrameResource>> mFrameResources;
    std::unordered_map<std::string, std::unique_ptr<Material>> mMaterials;
    std::unordered_map<std::string, std::unique_ptr<PD::Texture>> mTextures;
    std::unordered_map<std::string, int> mTextureSrvHandleIndices;

    // List of all the render items.
    std::vector<std::unique_ptr<RenderItem>> mAllRitems;
    std::vector<const RenderItem *> mOpaqueRitems;

    std::vector<D3D12_INPUT_ELEMENT_DESC> mInputLayout;
    XMFLOAT4X4 mView = MathHelper::Identity4x4();
    XMFLOAT4X4 mProj = MathHelper::Identity4x4();
    int mCurrFrameResourceIndex = 0;

    FrameResource *mCurrFrameResource = nullptr;
    PassConstants mMainPassCB;
    bool mbShowWireFrame = false;
    bool mbVsync = true;
    BoundingFrustum mCamFrustum;
    bool mbFrustumCulling = false;
    std::string mCullingInfo = "";
};

inline LightColumnsApp::LightColumnsApp(HINSTANCE hInstance) : D3DApp(hInstance) {
    auto path = PD::ResourcePathSearcher::Path(PROJECT_DIR);
    if (!path.is_absolute()) { path = fs::absolute(path); }
    mResourceManager.add_path(path);
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "models");
    mResourceManager.add_path(PD::ResourcePathSearcher::root_path / "resources" / "textures");
    mResourceManager.add_path(path / L"Shaders");
    mResourceManager.add_path(path / ".." / ".." / ".." / "dx11" / "cpp-dx11" / "resources" / "models" / "spot");
}

inline void LightColumnsApp::ReleaseResource() {
    mRootSignature.Reset();
    mCbvHeap.Reset();
}

inline bool LightColumnsApp::Initialize() {
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

inline void LightColumnsApp::OnResizeCallback() {
    D3DApp::OnResizeCallback();
    float aspect = GetAspectRatio();
    XMMATRIX P = XMMatrixPerspectiveFovLH(0.25f * MathHelper::Pi, aspect, 1.0f, 1000.0f);
    XMStoreFloat4x4(&mProj, P);
    BoundingFrustum::CreateFromMatrix(mCamFrustum, P);
}

inline void LightColumnsApp::OnMouseDownCallback(WPARAM btnState, int x, int y) {
    D3DApp::OnMouseDownCallback(btnState, x, y);
    if (btnState & MK_RBUTTON) {
        fmt::print("Picking: {} {}", x, y);
        AABBPick(x, y);
    }
}

inline void LightColumnsApp::Update(const GameTimer &timer) {

    D3DApp::Update(timer);
    XMStoreFloat4x4(&mView, mCamera->GetViewMatrix());

    mCurrFrameResourceIndex = (mCurrFrameResourceIndex + 1) % kNumFrameResources;
    mCurrFrameResource = mFrameResources[mCurrFrameResourceIndex].get();

    // 此时GPU已经落后CPU kNumFrameResources帧
    if (mCurrFrameResource->Fence != 0 && mFence->GetCompletedValue() < mCurrFrameResource->Fence) {
        HANDLE eventHandle = CreateEventEx(nullptr, nullptr, 0, EVENT_ALL_ACCESS);
        HR(mFence->SetEventOnCompletion(mCurrFrameResource->Fence, eventHandle));
        WaitForSingleObject(eventHandle, INFINITE);
        CloseHandle(eventHandle);
    }

    UpdateObjectInstanceData(timer);
    UpdateMainPassCB(timer);
    UpdateMaterialBuffers(timer);

    ImGuiPrepareDraw();
    static bool bShowDemoWindow = false;
    if (bShowDemoWindow) ImGui::ShowDemoWindow(&bShowDemoWindow);

    ImGui::Begin("Hello, world!");
    ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate,
                ImGui::GetIO().Framerate);
    ImGui::Checkbox("WireFrame", &mbShowWireFrame);
    ImGui::Checkbox("VSync", &mbVsync);
    ImGui::Checkbox("DemoWindow", &bShowDemoWindow);
    ImGui::Checkbox("Frustum Culling", &mbFrustumCulling);
    ImGui::TextUnformatted(mCullingInfo.data());
    ImGui::End();
    ImGui::Render();
}

void LightColumnsApp::UpdateObjectInstanceData(const GameTimer &timer) {
    XMMATRIX view = mCamera->GetViewMatrix();
    XMMATRIX invView = XMMatrixInverse(nullptr, view);

    auto currInstanceBuffer = mCurrFrameResource->ObjectInstanceBuffer.get();
    // 目前只允许绘制一个物体的实例
    auto &element = mAllRitems[0];
    // upload data when needed
    const auto &instanceData = element->instanceData;
    int visiableInstanceCount = 0;

    for (int i = 0; i < instanceData.size(); i++) {
        auto world = XMLoadFloat4x4(&instanceData[i].World);
        auto invWorld = XMMatrixInverse(nullptr, world);
        auto TexTrans = XMLoadFloat4x4(&instanceData[i].TexTransform);
        auto invTransWorld = XMLoadFloat4x4(&instanceData[i].InvTransWorld);
        auto viewToLocal = invView * invWorld;
        BoundingFrustum localFrustum;
        mCamFrustum.Transform(localFrustum, viewToLocal);
        if (!mbFrustumCulling || (localFrustum.Contains(element->Bounds) != DirectX::DISJOINT)) {
            InstanceDataGPU data;
            XMStoreFloat4x4(&data.World, XMMatrixTranspose(world));
            XMStoreFloat4x4(&data.InvTransWorld, XMMatrixTranspose(invTransWorld));
            XMStoreFloat4x4(&data.TexTransform, XMMatrixTranspose(TexTrans));
            data.MaterialIndex = instanceData[i].MaterialIndex;
            currInstanceBuffer->CopyData(visiableInstanceCount++, data);
        }
    }
    element->InstanceCount = visiableInstanceCount;
    mCullingInfo = fmt::format("Visible: {} / {}", visiableInstanceCount, instanceData.size());
}
inline void LightColumnsApp::UpdateMaterialBuffers(const GameTimer &timer) {

    auto currMaterialBuffer = mCurrFrameResource->MaterialStructuralBuffer.get();
    for (auto &element : mMaterials) {
        // upload data when needed
        Material *mat = element.second.get();
        if (mat->NumFramesDirty > 0) {
            MaterialData matData{};
            XMMATRIX matTransform = XMLoadFloat4x4(&mat->MatTransform);
            matData.DiffuseAlbedo = mat->DiffuseAlbedo;
            matData.FresnelR0 = mat->FresnelR0;
            matData.Roughness = mat->Roughness;
            XMStoreFloat4x4(&matData.MatTransform, XMMatrixTranspose(matTransform));
            matData.DiffuseMapIndex = mat->DiffuseSrvHeapIndex;
            currMaterialBuffer->CopyData(mat->MatCBIndex, matData);
            mat->NumFramesDirty--;
        }
    }
}
inline void LightColumnsApp::UpdateMainPassCB(const GameTimer &gt) {
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
    mMainPassCB.AmbientLight = {0.25f, 0.25f, 0.35f, 1.0f};

    mMainPassCB.Lights[0].Direction = {0.57735f, -0.57735f, 0.57735f};
    mMainPassCB.Lights[0].Strength = {0.6f, 0.6f, 0.6f};
    mMainPassCB.Lights[1].Direction = {-0.57735f, -0.57735f, 0.57735f};
    mMainPassCB.Lights[1].Strength = {0.3f, 0.3f, 0.3f};
    mMainPassCB.Lights[2].Direction = {0.0f, -0.707f, -0.707f};
    mMainPassCB.Lights[2].Strength = {0.15f, 0.15f, 0.15f};

    auto currPassCB = mCurrFrameResource->PassCB.get();
    currPassCB->CopyData(0, mMainPassCB);
}

inline void LightColumnsApp::DrawRenderItems(ID3D12GraphicsCommandList *cmdList,
                                             const std::vector<const RenderItem *> &ritems) {

    for (size_t i = 0; i < ritems.size(); i++) {
        // 要绘制一个物体，需要物体本身的信息，和它有关的model
        // 先设置顶点信息
        auto ri = ritems[i];
        cmdList->IASetVertexBuffers(0, 1, &ri->Geo->VertexBufferView());
        cmdList->IASetIndexBuffer(&ri->Geo->IndexBufferView());
        cmdList->IASetPrimitiveTopology(ri->PrimitiveType);

        assert(ri->IndexCount > 0);
        cmdList->DrawIndexedInstanced(ri->IndexCount, ri->InstanceCount, ri->StartIndexLocation, ri->BaseVertexLocation,
                                      0);
    }
}

void LightColumnsApp::Draw(const GameTimer &gt) {


    // 取出当前帧的Alloc,然后把cmdlist关联到这个alloc
    auto cmdListAlloc = mCurrFrameResource->CmdListAlloc;
    HR(cmdListAlloc->Reset());

    auto SwitchPSO = [this]() { return mbShowWireFrame ? mPSOs["opaque_wireframe"].Get() : mPSOs["opaque"].Get(); };
    HR(mCommandList->Reset(cmdListAlloc.Get(), SwitchPSO()));

    mCommandList->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(CurrentBackBuffer(),
                                                                           D3D12_RESOURCE_STATE_PRESENT,
                                                                           D3D12_RESOURCE_STATE_RENDER_TARGET));

    mCommandList->RSSetViewports(1, &mScreenViewport);
    mCommandList->RSSetScissorRects(1, &mScissorRect);


    mCommandList->ClearRenderTargetView(CurrentBackBufferView(), DirectX::Colors::LightBlue, 0, nullptr);
    mCommandList->ClearDepthStencilView(DepthStencilView(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0,
                                        0, nullptr);

    mCommandList->OMSetRenderTargets(1, &CurrentBackBufferView(), true, &DepthStencilView());

    mCommandList->SetGraphicsRootSignature(mRootSignature.Get());

    ID3D12DescriptorHeap *descriptorHeaps[] = {mSrvHeap.Get()};
    mCommandList->SetDescriptorHeaps(1, descriptorHeaps);
    mCommandList->SetGraphicsRootConstantBufferView(0, mCurrFrameResource->PassCB->Resource()->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootShaderResourceView(
            1, mCurrFrameResource->MaterialStructuralBuffer->Resource()->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootShaderResourceView(
            2, mCurrFrameResource->ObjectInstanceBuffer->Resource()->GetGPUVirtualAddress());
    mCommandList->SetGraphicsRootDescriptorTable(3, mSrvHeap->GetGPUDescriptorHandleForHeapStart());

    DrawRenderItems(mCommandList.Get(), mOpaqueRitems);

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

inline void LightColumnsApp::AABBPick(int x, int y) {
    float vx = (2.0 * x / mWidth - 1) / mProj._11;
    float vy = (-2.0 * y / mHeight + 1) / mProj._22;

    SimpleMath::Vector4 rayOrigin(0, 0, 0, 1);
    SimpleMath::Vector4 rayDir(vx, vy, 1, 0);

    auto view = mCamera->GetViewMatrix();
    XMMATRIX invView = XMMatrixInverse(nullptr, view);

    auto &element = mAllRitems[0];
    auto &instanceData = element->instanceData;

    int target_index = -1;
    float tmin = std::numeric_limits<float>::max();

    // 从CPU Buffer里重建所有三角形
    auto *vertices = (Vertex *) element->Geo->VertexBufferCPU->GetBufferPointer();
    auto *indices = (uint16_t *) element->Geo->IndexBufferCPU->GetBufferPointer();
    UINT triangle_count = element->IndexCount / 3;
    struct Triangle {
        XMVECTOR v0, v1, v2;
    };
    std::vector<Triangle> tris(triangle_count);
    uint16_t baseOffset = element->StartIndexLocation;
    for (int i = 0; i < triangle_count; i++) {
        uint16_t i0 = indices[baseOffset + i * 3 + 0];
        uint16_t i1 = indices[baseOffset + i * 3 + 1];
        uint16_t i2 = indices[baseOffset + i * 3 + 2];
        tris[i].v0 = XMLoadFloat3(&vertices[i0].Pos);
        tris[i].v1 = XMLoadFloat3(&vertices[i1].Pos);
        tris[i].v2 = XMLoadFloat3(&vertices[i2].Pos);
    }

    for (int i = 0; i < instanceData.size(); i++) {
        // ignore already highlighted objects
        if (instanceData[i].picked) continue;

        auto world = XMLoadFloat4x4(&instanceData[i].World);
        auto invWorld = XMMatrixInverse(nullptr, world);
        auto toLocal = invView * invWorld;

        auto lRayOrigin = XMVector3TransformCoord(rayOrigin, toLocal);
        SimpleMath::Vector4 lRayDir = XMVector3TransformNormal(rayDir, toLocal);
        lRayDir.Normalize();

        float t = std::numeric_limits<float>::max();
        if (element->Bounds.Intersects(lRayOrigin, lRayDir, t)) {
            if (t > tmin) continue;
            // 精细判断每个三角形
            for (int j = 0; j < triangle_count; j++) {
                if (TriangleTests::Intersects(lRayOrigin, lRayDir, tris[j].v0, tris[j].v1, tris[j].v2, t)) {
                    if (t < tmin) {
                        tmin = t;
                        target_index = i;
                    }
                }
            }
        }
    }
    if (target_index >= 0) {
        instanceData[target_index].MaterialIndex = mMaterials.at("highlightMat")->MatCBIndex;
        instanceData[target_index].picked = true;
    }
}

inline void LightColumnsApp::BuildDescriptorHeaps() {
    assert(mRtvHeap.Get());
    assert(mDsvHeap.Get());
    spdlog::info("Building Descriptor Heaps");

    UINT numDescriptor = kNumFrameResources;

    D3D12_DESCRIPTOR_HEAP_DESC cbvHeapDesc{};
    cbvHeapDesc.NumDescriptors = numDescriptor;
    cbvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    cbvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    cbvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&cbvHeapDesc, IID_PPV_ARGS(&mCbvHeap)));

    int textureSize = mTextures.size();
    D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc{};
    srvHeapDesc.NumDescriptors = textureSize;
    srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
    srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
    srvHeapDesc.NodeMask = 0;
    HR(mD3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&mSrvHeap)));


    for (const auto &it : mTextures) {
        CD3DX12_CPU_DESCRIPTOR_HANDLE srvHandle(mSrvHeap->GetCPUDescriptorHandleForHeapStart());
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

inline void LightColumnsApp::BuildConstantBuffers() {
    spdlog::info("Building Passes Constant Buffers");
    UINT passCBByteSize = CalcConstantBufferBytesSize(sizeof(PassConstants));

    // Last three descriptors are the pass CBVs for each frame resource.
    for (int frameIndex = 0; frameIndex < kNumFrameResources; ++frameIndex) {
        auto passCB = mFrameResources[frameIndex]->PassCB->Resource();
        D3D12_GPU_VIRTUAL_ADDRESS cbAddress = passCB->GetGPUVirtualAddress();

        // Offset to the pass cbv in the descriptor heap.
        int heapIndex = frameIndex;
        auto handle = CD3DX12_CPU_DESCRIPTOR_HANDLE(mCbvHeap->GetCPUDescriptorHandleForHeapStart());
        handle.Offset(heapIndex, mCbvSrvUavDescriptorSize);

        D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc;
        cbvDesc.BufferLocation = cbAddress;
        cbvDesc.SizeInBytes = passCBByteSize;
        mD3dDevice->CreateConstantBufferView(&cbvDesc, handle);
    }
}

inline void LightColumnsApp::BuildRootSignature() {
    CD3DX12_ROOT_PARAMETER slotRootParameter[4];
    slotRootParameter[0].InitAsConstantBufferView(0);   // pass cbv
    slotRootParameter[1].InitAsShaderResourceView(0, 1);// space 1, structural buffer,material buffer
    slotRootParameter[2].InitAsShaderResourceView(1, 1);// space 1, instance Data

    CD3DX12_DESCRIPTOR_RANGE texTable;
    assert(mTextures.size() > 0);
    texTable.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, mTextures.size(), 0);
    slotRootParameter[3].InitAsDescriptorTable(1, &texTable, D3D12_SHADER_VISIBILITY_PIXEL);

    auto static_samplers = GetStaticSamplers();
    CD3DX12_ROOT_SIGNATURE_DESC rootSigDesc(_countof(slotRootParameter), slotRootParameter, static_samplers.size(),
                                            static_samplers.data(),
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

inline void LightColumnsApp::BuildShaderAndInputLayout() {
    spdlog::info("Bulding Shaders");
    auto ShaderPath = mResourceManager.find_path("color.hlsl");

    auto VSBlob = CompileShader(ShaderPath, nullptr, "VSMain", "vs_5_1");
    auto PSBlob = CompileShader(ShaderPath, nullptr, "PSMain", "ps_5_1");
    assert(VSBlob);
    assert(PSBlob);
    mShaders["standardVS"] = VSBlob;
    mShaders["opaquePS"] = PSBlob;

    spdlog::info("Bulding Input Layout");
    mInputLayout = {
            {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
            {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, sizeof(XMFLOAT3), D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,
             0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 2 * sizeof(XMFLOAT3),
             D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
    };
}

inline void LightColumnsApp::BuildShapeGeometry() {
    spdlog::info("Bulding Shape Geometries");

    GeometryGenerator geoGen;
    GeometryGenerator::MeshData box = geoGen.CreateBox(1.5f, 0.5f, 1.5f, 3);
    GeometryGenerator::MeshData grid = geoGen.CreateGrid(20.0f, 30.0f, 60, 40);
    GeometryGenerator::MeshData sphere = geoGen.CreateSphere(0.5f, 20, 20);
    GeometryGenerator::MeshData cylinder = geoGen.CreateCylinder(0.5f, 0.3f, 3.0f, 20, 20);

    // ex 7.9.3 Load Model file
    auto ModelMeshes = LoadModelFromFile(mResourceManager.find_path("spot_triangulated.obj"));
    auto SkullMesh = LoadLunaFileFromFile(mResourceManager.find_path("skull.txt"));
    //
    // We are concatenating all the geometry into one big vertex/index buffer.  So
    // define the regions in the buffer each submesh covers.
    //

    // Cache the vertex offsets to each object in the concatenated vertex buffer.
    UINT boxVertexOffset = 0;
    UINT gridVertexOffset = (UINT) box.Vertices.size();
    UINT sphereVertexOffset = gridVertexOffset + (UINT) grid.Vertices.size();
    UINT cylinderVertexOffset = sphereVertexOffset + (UINT) sphere.Vertices.size();

    // Cache the starting index for each object in the concatenated index buffer.
    UINT boxIndexOffset = 0;
    UINT gridIndexOffset = (UINT) box.Indices32.size();
    UINT sphereIndexOffset = gridIndexOffset + (UINT) grid.Indices32.size();
    UINT cylinderIndexOffset = sphereIndexOffset + (UINT) sphere.Indices32.size();

    // Define the SubmeshGeometry that cover different
    // regions of the vertex/index buffers.

    SubmeshGeometry boxSubmesh;
    boxSubmesh.IndexCount = (UINT) box.Indices32.size();
    boxSubmesh.StartIndexLocation = boxIndexOffset;
    boxSubmesh.BaseVertexLocation = boxVertexOffset;

    SubmeshGeometry gridSubmesh;
    gridSubmesh.IndexCount = (UINT) grid.Indices32.size();
    gridSubmesh.StartIndexLocation = gridIndexOffset;
    gridSubmesh.BaseVertexLocation = gridVertexOffset;

    SubmeshGeometry sphereSubmesh;
    sphereSubmesh.IndexCount = (UINT) sphere.Indices32.size();
    sphereSubmesh.StartIndexLocation = sphereIndexOffset;
    sphereSubmesh.BaseVertexLocation = sphereVertexOffset;

    SubmeshGeometry cylinderSubmesh;
    cylinderSubmesh.IndexCount = (UINT) cylinder.Indices32.size();
    cylinderSubmesh.StartIndexLocation = cylinderIndexOffset;
    cylinderSubmesh.BaseVertexLocation = cylinderVertexOffset;


    //
    // Extract the vertex elements we are interested in and pack the
    // vertices of all the meshes into one vertex buffer.
    //

    auto totalVertexCount =
            box.Vertices.size() + grid.Vertices.size() + sphere.Vertices.size() + cylinder.Vertices.size();

    std::vector<Vertex> vertices(totalVertexCount);

    UINT k = 0;
    for (size_t i = 0; i < box.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = box.Vertices[i].Position;
        vertices[k].Normal = box.Vertices[i].Normal;
        vertices[k].TexC = box.Vertices[i].TexC;
    }

    for (size_t i = 0; i < grid.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = grid.Vertices[i].Position;
        vertices[k].Normal = grid.Vertices[i].Normal;
        vertices[k].TexC = grid.Vertices[i].TexC;
    }

    for (size_t i = 0; i < sphere.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = sphere.Vertices[i].Position;
        vertices[k].Normal = sphere.Vertices[i].Normal;
        vertices[k].TexC = sphere.Vertices[i].TexC;
    }

    for (size_t i = 0; i < cylinder.Vertices.size(); ++i, ++k) {
        vertices[k].Pos = cylinder.Vertices[i].Position;
        vertices[k].Normal = cylinder.Vertices[i].Normal;
        vertices[k].TexC = cylinder.Vertices[i].TexC;
    }


    std::vector<std::uint16_t> indices;
    indices.insert(indices.end(), std::begin(box.GetIndices16()), std::end(box.GetIndices16()));
    indices.insert(indices.end(), std::begin(grid.GetIndices16()), std::end(grid.GetIndices16()));
    indices.insert(indices.end(), std::begin(sphere.GetIndices16()), std::end(sphere.GetIndices16()));
    indices.insert(indices.end(), std::begin(cylinder.GetIndices16()), std::end(cylinder.GetIndices16()));

    SubmeshGeometry SpotSubmesh;
    SpotSubmesh.StartIndexLocation = indices.size();
    SpotSubmesh.BaseVertexLocation = vertices.size();
    XMVECTOR vMin = SimpleMath::Vector3(MathHelper::Infinity, MathHelper::Infinity, MathHelper::Infinity);
    XMVECTOR vMax = SimpleMath::Vector3(-MathHelper::Infinity, -MathHelper::Infinity, -MathHelper::Infinity);
    size_t SpotMeshIndicesCount = 0;
    for (const auto &MeshInfo : ModelMeshes) {
        totalVertexCount += MeshInfo.mPoses.size();
        for (size_t i = 0; i < MeshInfo.mPoses.size(); ++i) {
            Vertex v{};
            v.Pos = MeshInfo.mPoses[i];
            v.Normal = MeshInfo.mNormals[i];
            if (!MeshInfo.mTexs.empty()) v.TexC = MeshInfo.mTexs[i];
            vertices.push_back(v);

            XMVECTOR P = XMLoadFloat3(&v.Pos);
            vMin = XMVectorMin(vMin, P);
            vMax = XMVectorMax(vMax, P);
        }
        indices.insert(indices.end(), std::begin(MeshInfo.mIndices16), std::end(MeshInfo.mIndices16));
        SpotMeshIndicesCount += MeshInfo.mIndices32.size();
    }
    SpotSubmesh.IndexCount = SpotMeshIndicesCount;
    BoundingBox SpotBounds;
    XMStoreFloat3(&SpotBounds.Center, (vMin + vMax));
    XMStoreFloat3(&SpotBounds.Extents, (vMax - vMin) * 0.5f);
    SpotSubmesh.Bounds = SpotBounds;

    totalVertexCount += SkullMesh.mPoses.size();
    SubmeshGeometry SkullSubmesh;
    SkullSubmesh.StartIndexLocation = indices.size();
    SkullSubmesh.BaseVertexLocation = vertices.size();
    SkullSubmesh.IndexCount = SkullMesh.mIndices32.size();
    for (size_t i = 0; i < SkullMesh.mPoses.size(); ++i) {
        Vertex v = {};
        v.Pos = SkullMesh.mPoses[i];
        v.Normal = SkullMesh.mNormals[i];
        if (!SkullMesh.mTexs.empty()) { v.TexC = SkullMesh.mTexs[i]; }
        vertices.push_back(v);
    }
    indices.insert(indices.end(), std::begin(SkullMesh.mIndices16), std::end(SkullMesh.mIndices16));


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
    geo->DrawArgs["grid"] = gridSubmesh;
    geo->DrawArgs["sphere"] = sphereSubmesh;
    geo->DrawArgs["cylinder"] = cylinderSubmesh;
    geo->DrawArgs["spot"] = SpotSubmesh;
    geo->DrawArgs["skull"] = SkullSubmesh;

    mGeometries[geo->name] = std::move(geo);
}

inline void LightColumnsApp::BuildPSOs() {

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
    HR(mD3dDevice->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&mMSAAOpaquePSO)));
}

inline void LightColumnsApp::BuildFrameResources() {
    for (int i = 0; i < kNumFrameResources; i++) {
        mFrameResources.push_back(
                std::make_unique<FrameResource>(mD3dDevice.Get(), 1, (UINT) instanceCount, mMaterials.size()));
    }
}

inline void LightColumnsApp::BuildRenderItems() {
    spdlog::info("Build Render Items");


    auto spotRitem = std::make_unique<RenderItem>();
    spotRitem->Geo = mGeometries["shapeGeo"].get();
    spotRitem->PrimitiveType = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
    spotRitem->IndexCount = spotRitem->Geo->DrawArgs["spot"].IndexCount;
    spotRitem->StartIndexLocation = spotRitem->Geo->DrawArgs["spot"].StartIndexLocation;
    spotRitem->BaseVertexLocation = spotRitem->Geo->DrawArgs["spot"].BaseVertexLocation;
    spotRitem->InstanceCount = instanceCount;
    spotRitem->instanceData.resize(instanceCount);
    spotRitem->Bounds = spotRitem->Geo->DrawArgs["spot"].Bounds.value();
    srand(1234);
    auto get_random_float = []() { return (double) rand() / RAND_MAX; };
    for (int i = 0; i < instanceCount; i++) {
        float x = (get_random_float() * 2 - 1.0) * 30;
        float y = (get_random_float() * 2 - 1.0) * 30;
        float z = (get_random_float() * 2) * 10;
        float scale = (get_random_float() + 1) * 2;
        auto trans = XMMatrixScaling(scale, scale, scale) * XMMatrixTranslation(x, y, z);
        XMStoreFloat4x4(&spotRitem->instanceData[i].World, trans);
        XMStoreFloat4x4(&spotRitem->instanceData[i].InvTransWorld, MathHelper::InverseTranspose(trans));
        spotRitem->instanceData[i].TexTransform = MathHelper::Identity4x4();
        spotRitem->instanceData[i].MaterialIndex = i % (mMaterials.size() - 1);// exluce highlightMat
    }
    mAllRitems.push_back(std::move(spotRitem));


    spdlog::info("Build Opaque Render Items");
    // All the render items are opaque.
    for (auto &e : mAllRitems) mOpaqueRitems.push_back(e.get());
}

inline void LightColumnsApp::BuildMaterials() {
    int matIndex = 0;
    auto bricks0 = std::make_unique<Material>();
    bricks0->Name = "bricks0";
    bricks0->MatCBIndex = matIndex++;
    bricks0->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("bricks");
    bricks0->DiffuseAlbedo = XMFLOAT4(Colors::ForestGreen);
    bricks0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    bricks0->Roughness = 0.1f;

    auto stone0 = std::make_unique<Material>();
    stone0->Name = "stone0";
    stone0->MatCBIndex = matIndex++;
    stone0->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("stone");
    stone0->DiffuseAlbedo = XMFLOAT4(Colors::LightSteelBlue);
    stone0->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    stone0->Roughness = 0.3f;

    auto tile0 = std::make_unique<Material>();
    tile0->Name = "tile0";
    tile0->MatCBIndex = matIndex++;
    tile0->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("tile");
    tile0->DiffuseAlbedo = XMFLOAT4(Colors::LightGray);
    tile0->FresnelR0 = XMFLOAT3(0.02f, 0.02f, 0.02f);
    tile0->Roughness = 0.2f;

    auto skullMat = std::make_unique<Material>();
    skullMat->Name = "skullMat";
    skullMat->MatCBIndex = matIndex++;
    skullMat->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("dummy");
    skullMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    skullMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    skullMat->Roughness = 1.0f;

    auto spotMat = std::make_unique<Material>();
    spotMat->Name = "spotMat";
    spotMat->MatCBIndex = matIndex++;
    spotMat->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("spot");
    spotMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
    spotMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    spotMat->Roughness = 1.0f;

    auto hightlightMat = std::make_unique<Material>();
    hightlightMat->Name = "highlightMat";
    hightlightMat->MatCBIndex = matIndex++;
    hightlightMat->DiffuseSrvHeapIndex = mTextureSrvHandleIndices.at("dummy");
    hightlightMat->DiffuseAlbedo = XMFLOAT4(1.0f, 1.0f, 0.0f, 0.6f);
    hightlightMat->FresnelR0 = XMFLOAT3(0.05f, 0.05f, 0.05f);
    hightlightMat->Roughness = 1.0f;

    mMaterials["bricks0"] = std::move(bricks0);
    mMaterials["stone0"] = std::move(stone0);
    mMaterials["tile0"] = std::move(tile0);
    mMaterials["skullMat"] = std::move(skullMat);
    mMaterials["spotMat"] = std::move(spotMat);
    mMaterials["highlightMat"] = std::move(hightlightMat);
}

inline void LightColumnsApp::LoadTextures() {
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
        auto stoneTex = std::make_unique<Texture>();
        stoneTex->Name = "stone";
        stoneTex->Filename = fs::absolute(this->mResourceManager.find_path("stone.dds")).u8string();
        mTextureSrvHandleIndices[stoneTex->Name] = texIndex++;
        Texture::LoadAndUploadTexture(*stoneTex, mD3dDevice.Get(), mCommandList.Get());
        mTextures[stoneTex->Name] = std::move(stoneTex);
    }

    {
        auto tileTex = std::make_unique<Texture>();
        tileTex->Name = "tile";
        tileTex->Filename = fs::absolute(this->mResourceManager.find_path("tile.dds")).u8string();
        Texture::LoadAndUploadTexture(*tileTex, mD3dDevice.Get(), mCommandList.Get());
        mTextureSrvHandleIndices[tileTex->Name] = texIndex++;
        mTextures[tileTex->Name] = std::move(tileTex);
    }

    {
        auto woodCrateTex = std::make_unique<Texture>();
        woodCrateTex->Name = "woodCrate";
        woodCrateTex->Filename = fs::absolute(this->mResourceManager.find_path("woodCrate01.dds")).u8string();
        Texture::LoadAndUploadTexture(*woodCrateTex, mD3dDevice.Get(), mCommandList.Get());
        mTextureSrvHandleIndices[woodCrateTex->Name] = texIndex++;
        mTextures[woodCrateTex->Name] = std::move(woodCrateTex);
    }
    {
        auto bricksTex = std::make_unique<Texture>();
        bricksTex->Name = "bricks";
        bricksTex->Filename = fs::absolute(this->mResourceManager.find_path("bricks.dds")).u8string();
        Texture::LoadAndUploadTexture(*bricksTex, mD3dDevice.Get(), mCommandList.Get());
        mTextureSrvHandleIndices[bricksTex->Name] = texIndex++;
        mTextures[bricksTex->Name] = std::move(bricksTex);
    }

    {
        auto spotTex = std::make_unique<Texture>();
        spotTex->Name = "spot";
        spotTex->Filename = fs::absolute(this->mResourceManager.find_path("spot_texture.png")).u8string();
        Texture::LoadAndUploadTexture(*spotTex, mD3dDevice.Get(), mCommandList.Get(), true);
        mTextureSrvHandleIndices[spotTex->Name] = texIndex++;
        mTextures[spotTex->Name] = std::move(spotTex);
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
    LightColumnsApp theApp(hInstance);
    if (!theApp.Initialize()) return 0;

    return theApp.MessageLoopRun();
}