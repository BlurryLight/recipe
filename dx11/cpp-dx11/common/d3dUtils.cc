#define _SILENCE_ALL_CXX17_DEPRECATION_WARNINGS
#include "d3dUtils.hh"
#include <codecvt>
#include <comdef.h> // for _com_error
#include <debugapi.h>

std::string utf16_to_utf8(std::u16string const &s) {
  std::wstring_convert<
      std::codecvt_utf8_utf16<char16_t, 0x10ffff,
                              std::codecvt_mode::little_endian>,
      char16_t>
      cnv;
  std::string utf8 = cnv.to_bytes(s);
  if (cnv.converted() < s.size())
    throw std::runtime_error("incomplete conversion");
  return utf8;
}

std::u16string utf8_to_utf16(std::string const &utf8) {
  std::wstring_convert<
      std::codecvt_utf8_utf16<char16_t, 0x10ffff,
                              std::codecvt_mode::little_endian>,
      char16_t>
      cnv;
  std::u16string s = cnv.from_bytes(utf8);
  if (cnv.converted() < utf8.size())
    throw std::runtime_error("incomplete conversion");
  return s;
}
std::u16string utf8_to_utf16_windows(std::string const &utf8) {
  int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(),
                                  -1, NULL, 0);
  std::u16string res;
  res.resize(count);
  int flag = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8.c_str(),
                                 -1, (wchar_t *)(res.data()), count);
  if (!flag) {
    std::cerr << "converted u8 to utf16 failed" << std::endl;
    return u"";
  }
  return res;
}
std::string utf16_to_utf8_windows(std::u16string const &utf16s) {
  int count = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)(utf16s.c_str()), -1,
                                  NULL, 0, NULL, NULL);
  std::string res;
  res.resize(count);
  int flag = WideCharToMultiByte(CP_UTF8, 0, (wchar_t *)utf16s.c_str(), -1,
                                 res.data(), count, NULL, NULL);
  if (!flag) {
    std::cerr << "converted u16 to utf8 failed" << std::endl;
    return "";
  }
  return res;
}
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

/**
 * @brief Format error message about HRESULT
 * I did some experiments about charset convert around utf16, utf8 and gbk.
 * Go
 * https://github.com/BlurryLight/recipe/blob/997d605fbeb053279a4b9981dd8cba8eca719c9f/dx11/cpp-dx11/common/d3dUtils.cc#L133
 * for details.
 * FormatMessageW?????????wstring????????????utf16le??????????????????pwsh(utf-8)?????????????????????cmd/powershell(gbk)??????????????????????????????????????????locale???
 * ????????????utf8?????????
 *
 * ??????SUBLANG_ENGLISH_US????????????FormatMessageA?????????char*????????????GBK???????????????????????????????????????????????????ASCII???????????????????????????pwsh??????????????????cmd?????????
 * ??????SUBLANG_DEFAULT????????????FormatMessageA?????????char*?????????GBK????????????????????????????????????pwsh(utf-8)???????????????????????????cmd(GBK)?????????
 *
 * @param file
 * @param line
 * @param hr
 * @param proc
 */
void DxTrace(const wchar_t *file, unsigned long line, HRESULT hr,
             const wchar_t *proc) {

  char *output;
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                 (char *)&output, 0, NULL);
  std::wcerr << "file:" << file << "line:" << line << ", " << proc << std::endl;
  std::printf("Error Msg: %s", output);
}
} // namespace PD