//
// Created by zhong on 2021/3/27.
//

#ifndef DXWINDOW_D3DUTILS_HH
#define DXWINDOW_D3DUTILS_HH
#include <windows.h>
namespace PD {

    struct Noncopyable {
        Noncopyable() = default;
        Noncopyable(const Noncopyable &) = delete;
        Noncopyable &operator=(const Noncopyable &) = delete;
    };
    class d3dUtils {};
}// namespace PD

#endif// DXWINDOW_D3DUTILS_HH
