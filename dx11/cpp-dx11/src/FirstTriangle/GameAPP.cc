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
  void UpdateScene(float dt) override{};
  void DrawScene() override {
    assert(pd3dDeviceIMContext_);
    assert(pSwapChain_);
    static float blue[4] = {0.0f, 0.0f, 0.0f, 1.0f};
    pd3dDeviceIMContext_->ClearRenderTargetView(pRenderTargetView_.Get(), blue);
    pd3dDeviceIMContext_->ClearDepthStencilView(
        pDepthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f,
        0);
    HRESULT hr;
    pd3dDeviceIMContext_->Draw(3, 0);
    HR(pSwapChain_->Present(0, 0));
  };

private:
  ComPtr<ID3D11InputLayout> pVertexLayout_;
  ComPtr<ID3D11Buffer> pVertexBuffer_;
  ComPtr<ID3D11VertexShader> pVertexShader_;
  ComPtr<ID3D11PixelShader> pPixelShader_;

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
        {XMFLOAT3(0.0f, 0.5f, 0.5f), XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f)},
        {XMFLOAT3(0.5f, -0.5f, 0.5f), XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f)},
        {XMFLOAT3(-0.5f, -0.5f, 0.5f), XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f)}};
    // 设置顶点缓冲区描述
    D3D11_BUFFER_DESC vbd;
    ZeroMemory(&vbd, sizeof(vbd));
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = sizeof vertices;
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    // 新建顶点缓冲区
    D3D11_SUBRESOURCE_DATA InitData;
    ZeroMemory(&InitData, sizeof(InitData));
    InitData.pSysMem = vertices;
    HRESULT hr;
    HR(pd3dDevice_->CreateBuffer(&vbd, &InitData,
                                 pVertexBuffer_.GetAddressOf()));

    UINT stride = sizeof(VertexPosColor);
    UINT offset = 0;

    pd3dDeviceIMContext_->IASetVertexBuffers(
        0, 1, pVertexBuffer_.GetAddressOf(), &stride, &offset);
    // 设置图元类型，设定输入布局
    pd3dDeviceIMContext_->IASetPrimitiveTopology(
        D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    pd3dDeviceIMContext_->IASetInputLayout(pVertexLayout_.Get());
    // 将着色器绑定到渲染管线
    pd3dDeviceIMContext_->VSSetShader(pVertexShader_.Get(), nullptr, 0);
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