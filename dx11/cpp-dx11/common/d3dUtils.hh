#pragma once
#include <d3d11_1.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <iostream>
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
#define HR_RETURN(op)                                                          \
  if (FAILED(hr = (op))) {                                                     \
    DxTrace(__FILEW__, __LINE__, hr, L#op);                                    \
    return false;                                                              \
  }

#define HR(op)                                                                 \
  if (FAILED(hr = (op))) {                                                     \
    DxTrace(__FILEW__, __LINE__, hr, L#op);                                    \
    assert(0);                                                                 \
  }

template <typename T> void ignore(T &&) {}

std::string utf16_to_utf8(std::u16string const &s);
std::u16string utf8_to_utf16(std::string const &utf8);
// use windows.h api
std::string utf16_to_utf8_windows(std::u16string const &s);
std::u16string utf8_to_utf16_windows(std::string const &utf8);
namespace PD {

void D3D11SetDebugObjectName(ID3D11DeviceChild *resource,
                             std::string_view name);

void D3D11ClearDebugObjectName(ID3D11DeviceChild *resource);

void DXGISetDebugObjectName(IDXGIObject *object, std::string_view name);

void DXGIClearDebugObjectName(IDXGIObject *object);

HRESULT CreateShaderFromFile(std::wstring_view csoFileNameInOut,
                             std::wstring_view hlslPath, std::string entryPoint,
                             std::string ShaderModel, ID3DBlob **ppBlobOut,
                             bool force_compile = false);

void DxTrace(const wchar_t *file, unsigned long line, HRESULT hr,
             const wchar_t *proc);

} // namespace PD
