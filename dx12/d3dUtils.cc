//
// Created by zhong on 2021/3/27.
//

#include "d3dUtils.hh"
#include <iostream>
using namespace PD;
void PD::DxTrace(const wchar_t *file, unsigned long line, HRESULT hr,
             const wchar_t *proc) {

  char *output;
  FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 NULL, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                 (char *)&output, 0, NULL);
  std::wcerr << "file:" << file << "line:" << line << ", " << proc << std::endl;
  std::printf("Error Msg: %s", output);
}