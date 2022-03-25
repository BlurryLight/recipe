#pragma once
#include <d3d11.h>
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <debugapi.h>
#include <dxgi.h>
#include <string_view>
#include <winerror.h>
#include <winnt.h>


// com release
#define SAFE_RELEASE(p)                                                        \
  {                                                                            \
    if ((p)) {                                                                 \
      (p)->Release();                                                          \
      (p) = nullptr;                                                           \
    }                                                                          \
  }
#define ignore(p) (void)((p))

inline void D3D11SetDebugObjectName(ID3D11DeviceChild *resource,
                                    std::string_view name) {
#ifdef _DEBUG
  resource->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.data());
#else
  ignore(resource);
  ignore(name);
#endif
}

inline void D3D11ClearDebugObjectName(ID3D11DeviceChild *resource) {
  resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
}

inline void DXGISetDebugObjectName(IDXGIObject *object, std::string_view name) {
#ifdef _DEBUG
  object->SetPrivateData(WKPDID_D3DDebugObjectName, name.size(), name.data());
#else
  ignore(object);
  ignore(name);
#endif
}

inline void DXGIClearDebugObjectName(IDXGIObject *object) {
  object->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
}

inline HRESULT CreateShaderFromFile(std::wstring_view csoFileNameInOut,
                                    std::wstring_view hlslPath,
                                    std::string entryPoint,
                                    std::string ShaderModel,
                                    ID3DBlob **ppBlobOut) {
  HRESULT hr = S_OK;
  if (!csoFileNameInOut.empty() &&
      (D3DReadFileToBlob(csoFileNameInOut.data(), ppBlobOut) == S_OK)) {
    return hr;
  } else {
    DWORD dwShaderFlags = D3DCOMPILE_ENABLE_STRICTNESS;
#ifdef _DEBUG
    dwShaderFlags |= D3DCOMPILE_DEBUG;
    dwShaderFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
    ID3DBlob *errorBlob = nullptr;
    hr = D3DCompileFromFile(hlslPath.data(), nullptr,
                            D3D_COMPILE_STANDARD_FILE_INCLUDE,
                            entryPoint.data(), ShaderModel.data(),
                            dwShaderFlags, 0, ppBlobOut, &errorBlob);
    if (FAILED(hr)) {
      if (errorBlob) {
        OutputDebugStringA((const char *)(errorBlob->GetBufferPointer()));
      }
      SAFE_RELEASE(errorBlob);
      return hr;
    }
    if (!csoFileNameInOut.empty()) {
      hr = D3DWriteBlobToFile(*ppBlobOut, csoFileNameInOut.data(), false);
    }
    return hr;
  }
}