#ifndef GAMEAPP_H
#define GAMEAPP_H
#include <d3dApp.hh>
namespace PD{
class GameApp : public D3DApp
{
public:
  GameApp() : D3DApp(){};
  ~GameApp(){};

  void UpdateScene(float dt){};
  void DrawScene(){
      assert(pd3dDeviceIMContext_);
      assert(pSwapChain_);
      static float blue[4] = { 0.0f, 0.0f, 1.0f, 1.0f };	// RGBA = (0,0,255,255)
      pd3dDeviceIMContext_->ClearRenderTargetView(pRenderTargetView_.Get(), blue);
      pd3dDeviceIMContext_->ClearDepthStencilView(pDepthStencilView_.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
      HRESULT hr;
      HR(pSwapChain_->Present(0, 0));

  };
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