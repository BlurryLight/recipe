#include "d3dUtils.hh"
#include <comdef.h> // for _com_error
#include <debugapi.h>
namespace PD {
void D3D11SetDebugObjectName(ID3D11DeviceChild *resource,
                             std::string_view name) {
#ifdef _DEBUG
  resource->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(),
                           name.data());
#else
  ignore(resource);
  ignore(name);
#endif
}

void D3D11ClearDebugObjectName(ID3D11DeviceChild *resource) {
  resource->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
}
void DXGIClearDebugObjectName(IDXGIObject *object) {
  object->SetPrivateData(WKPDID_D3DDebugObjectName, 0, nullptr);
}
void DXGISetDebugObjectName(IDXGIObject *object, std::string_view name) {
#ifdef _DEBUG
  object->SetPrivateData(WKPDID_D3DDebugObjectName, (UINT)name.size(),
                         name.data());
#else
  ignore(object);
  ignore(name);
#endif
}
HRESULT CreateShaderFromFile(std::wstring_view csoFileNameInOut,
                             std::wstring_view hlslPath, std::string entryPoint,
                             std::string ShaderModel, ID3DBlob **ppBlobOut,
                             bool force_compile) {
  HRESULT hr = S_OK;
  if (!force_compile && !csoFileNameInOut.empty() &&
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
      // default: don't override existed cso
      // force_compile: override it
      hr = D3DWriteBlobToFile(*ppBlobOut, csoFileNameInOut.data(),
                              force_compile);
    }
    return hr;
  }
}
void DxTrace(const wchar_t *file, unsigned long line, HRESULT hr,
             const wchar_t *proc) {

  char *output;
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                 (char *)&output, 0, NULL);
  std::wcerr << "file:" << file << "line:" << line << ", " << proc << std::endl;
  //  << "ErrorDesc: " << err.Description()
  //  << "ErrorMsg: " << output << std::endl;
  std::printf("Error Msg: %s", output);
}
} // namespace PD