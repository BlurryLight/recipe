#ifndef GAMEAPP_H
#define GAMEAPP_H
#include <d3dApp.hh>
namespace PD{
class GameApp : public D3DApp
{
public:
  GameApp() : D3DApp(){
    this->WinTitle_ = "FirstTriangle";
  };
  ~GameApp(){};

  bool Init() override{
     auto res = D3DApp::Init();
     if(!res) return res;
    //  if(!InitEffect()) return false;
    //  if(!InitResource()) return false;
     return true;
  }
  void UpdateScene(float dt) override{};
  void DrawScene() override{
      assert(pd3dDeviceIMContext_);
      assert(pSwapChain_);
      static float blue[4] = { 0.0f, 0.0f, 1.0f, 1.0f };	// RGBA = (0,0,255,255)
      pd3dDeviceIMContext_->ClearRenderTargetView(pRenderTargetView_.Get(), blue);
      pd3dDeviceIMContext_->ClearDepthStencilView(pDepthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
      HRESULT hr;
      HR(pSwapChain_->Present(0, 0));

  };
protected:
  bool InitEffect(){
    ComPtr<ID3DBlob> blob;
    // HR(CreateShaderFrom)
    return false;
  }
  bool InitResource(){return false;};
};
}
int main()
{
  PD::GameApp app;
  app.Init();
  app.Run();
  return 0;
}

#endif