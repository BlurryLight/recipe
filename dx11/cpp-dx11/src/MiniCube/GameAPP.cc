#ifndef GAMEAPP_H
#define GAMEAPP_H
#include <d3dApp.hh>
#include <d3dUtils.hh>
#include <winnt.h>

namespace PD {
struct VertexPosColor {
  DirectX::XMFLOAT3 pos;
  DirectX::XMFLOAT4 color;
};

class GameApp : public D3DApp {
public:
  GameApp() : D3DApp() { this->WinTitle_ = "FirstTriangle"; };
  ~GameApp(){};

  bool Init() override {
    auto res = D3DApp::Init();
    if (!res)
      return res;
    if (!InitEffect())
      return false;
    if (!InitResource())
      return false;
    return true;
  }
  void UpdateScene(float dt) override {
    static float phi = 0.0f, theta = 0.0f;
    phi += 0.0001f, theta += 0.00015f;
    CBuffer_.World =
        DirectX::XMMatrixRotationX(phi) * DirectX::XMMatrixRotationY(theta);

    // Transpose all data
    //在hlsl里使用列主元存储矩阵，因为向量右乘以矩阵，所以一次要取矩阵的一个列
    //在C++里矩阵是行主元，从行主元到列主元相当于经过了一次转置
    //所以在C++里再手动转置一下，抵消上面的一次转置
    CBuffer_.World = DirectX::XMMatrixTranspose(CBuffer_.World);
    // map data
    D3D11_MAPPED_SUBRESOURCE mappedData;
    HRESULT hr = S_OK;
    HR(pd3dDeviceIMContext_->Map(pCBuffer_.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0,
                                 &mappedData));
    std::memcpy(mappedData.pData, &CBuffer_, sizeof CBuffer_);
    pd3dDeviceIMContext_->Unmap(pCBuffer_.Get(), 0);
  };
  void DrawScene() override {
    assert(pd3dDeviceIMContext_);
    assert(pSwapChain_);
    UpdateScene(0);
    static float blue[4] = {0.0f, 0.0f, 0.2f, 1.0f};
    pd3dDeviceIMContext_->ClearRenderTargetView(pRenderTargetView_.Get(), blue);
    pd3dDeviceIMContext_->ClearDepthStencilView(
        pDepthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f,
        0);
    pd3dDeviceIMContext_->DrawIndexed(36, 0, 0);
  };

private:
  ComPtr<ID3D11InputLayout> pVertexLayout_;
  ComPtr<ID3D11Buffer> pVertexBuffer_;
  ComPtr<ID3D11Buffer> pVertexIndexBuffer_;
  ComPtr<ID3D11Buffer> pCBuffer_;
  ComPtr<ID3D11VertexShader> pVertexShader_;
  ComPtr<ID3D11PixelShader> pPixelShader_;
  struct MVP {
    DirectX::XMMATRIX World;
    DirectX::XMMATRIX View;
    DirectX::XMMATRIX Proj;
  };
  MVP CBuffer_;

protected:
  bool InitEffect() {
    ComPtr<ID3DBlob> blob;
    HRESULT hr;
    const D3D11_INPUT_ELEMENT_DESC inputLayout[2] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
         D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12,
         D3D11_INPUT_PER_VERTEX_DATA, 0}};

    HR(CreateShaderFromFile(L"HLSL\\Triangle_VS.cso", L"HLSL\\Triangle_VS.hlsl",
                            "VS", "vs_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pd3dDevice_->CreateVertexShader(blob->GetBufferPointer(),
                                       blob->GetBufferSize(), nullptr,
                                       pVertexShader_.GetAddressOf()));
    HR(pd3dDevice_->CreateInputLayout(
        inputLayout, ARRAYSIZE(inputLayout), blob->GetBufferPointer(),
        blob->GetBufferSize(), pVertexLayout_.GetAddressOf()));

    HR(CreateShaderFromFile(L"HLSL\\Triangle_PS.cso", L"HLSL\\Triangle_PS.hlsl",
                            "PS", "ps_5_0", blob.ReleaseAndGetAddressOf()));
    HR(pd3dDevice_->CreatePixelShader(blob->GetBufferPointer(),
                                      blob->GetBufferSize(), nullptr,
                                      pPixelShader_.GetAddressOf()));

    return true;
  }
  bool InitResource() {
    using namespace DirectX;
    VertexPosColor vertices[] = {
        {XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, -1.0f), XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, -1.0f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(-1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
        {XMFLOAT3(-1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 0.0f, 1.0f, 1.0f)},
        {XMFLOAT3(1.0f, 1.0f, 1.0f), XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)},
        {XMFLOAT3(1.0f, -1.0f, 1.0f), XMFLOAT4(0.0f, 1.0f, 1.0f, 1.0f)}};

    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vertexBufferDesc;
    ZeroMemory(&vertexBufferDesc, sizeof(vertexBufferDesc));
    vertexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    vertexBufferDesc.ByteWidth = sizeof vertices;
    vertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vertexBufferDesc.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    HRESULT hr;
    HR(pd3dDevice_->CreateBuffer(&vertexBufferDesc, &InitData,
                                 pVertexBuffer_.GetAddressOf()));
    //索引缓冲区描述
    uint32_t indices[] = {// 正面
                          0, 1, 2, 2, 3, 0,
                          // 左面
                          4, 5, 1, 1, 0, 4,
                          // 顶面
                          1, 5, 6, 6, 2, 1,
                          // 背面
                          7, 6, 5, 5, 4, 7,
                          // 右面
                          3, 2, 6, 6, 7, 3,
                          // 底面
                          4, 0, 3, 3, 7, 4};
    D3D11_BUFFER_DESC indexBufferDesc;
    ZeroMemory(&indexBufferDesc, sizeof(indexBufferDesc));
    indexBufferDesc.Usage = D3D11_USAGE_IMMUTABLE;
    indexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
    indexBufferDesc.CPUAccessFlags = 0;
    indexBufferDesc.ByteWidth = sizeof indices;
    // 申请缓冲区
    InitData.pSysMem = indices;
    HR(pd3dDevice_->CreateBuffer(&indexBufferDesc, &InitData,
                                 pVertexIndexBuffer_.GetAddressOf()));
    // 申请cbuffer
    D3D11_BUFFER_DESC CBufferDesc;
    ZeroMemory(&CBufferDesc, sizeof CBufferDesc);
    CBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    CBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    CBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
    CBufferDesc.ByteWidth = sizeof(MVP);
    HR(pd3dDevice_->CreateBuffer(&CBufferDesc, nullptr,
                                 pCBuffer_.GetAddressOf()));
    CBuffer_.World = XMMatrixIdentity();
    CBuffer_.View = XMMatrixLookAtLH(XMVectorSet(0.0, 0.0, -5.0, 0.0),
                                     XMVectorSet(0.0, 0.0, 0.0, 0.0),
                                     XMVectorSet(0.0, 1.0, 0.0, 0.0));
    CBuffer_.Proj =
        XMMatrixPerspectiveFovLH(XM_PIDIV4, AspectRatio(), 0.1f, 100.0f);
    CBuffer_.View = DirectX::XMMatrixTranspose(CBuffer_.View);
    CBuffer_.Proj = DirectX::XMMatrixTranspose(CBuffer_.Proj);

    //绑定资源
    UINT stride = sizeof(VertexPosColor);
    UINT offset = 0;

    pd3dDeviceIMContext_->IASetVertexBuffers(
        0, 1, pVertexBuffer_.GetAddressOf(), &stride, &offset);
    // 设置图元类型，设定输入布局
    pd3dDeviceIMContext_->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dDeviceIMContext_->IASetInputLayout(pVertexLayout_.Get());
    pd3dDeviceIMContext_->IASetIndexBuffer(pVertexIndexBuffer_.Get(),
                                           DXGI_FORMAT_R32_UINT, 0);
    // 将着色器绑定到渲染管线
    pd3dDeviceIMContext_->VSSetShader(pVertexShader_.Get(), nullptr, 0);
    pd3dDeviceIMContext_->VSSetConstantBuffers(0, 1, pCBuffer_.GetAddressOf());
    pd3dDeviceIMContext_->PSSetShader(pPixelShader_.Get(), nullptr, 0);

    D3D11SetDebugObjectName(pVertexLayout_.Get(), "VertexPosColorLayout");
    D3D11SetDebugObjectName(pVertexBuffer_.Get(), "VertexBuffer");
    D3D11SetDebugObjectName(pVertexShader_.Get(), "Trangle_VS");
    D3D11SetDebugObjectName(pPixelShader_.Get(), "Trangle_PS");
    return true;
  };
};
} // namespace PD
int main() {
  PD::GameApp app;
  app.Init();
  app.Run();
  return 0;
}

#endif