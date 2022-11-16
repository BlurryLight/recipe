//
// Created by zhong on 2021/3/27.
//

#pragma once
#include <D3Dcompiler.h>
#include <DirectXCollision.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include <DirectXPackedVector.h>
#include <d3d12.h>
#include <dxgi1_4.h>
#include <windows.h>
#include <wrl.h>
#include "d3dx12.h"

namespace PD {

    struct Noncopyable {
        Noncopyable() = default;
        Noncopyable(const Noncopyable &) = delete;
        Noncopyable &operator=(const Noncopyable &) = delete;
    };
    class d3dUtils {};

    void DxTrace(const wchar_t *file, unsigned long line, HRESULT hr,
                 const wchar_t *proc);
}// namespace PD

// com release
#define SAFE_RELEASE(p)     \
    {                       \
        if ((p)) {          \
            (p)->Release(); \
            (p) = nullptr;  \
        }                   \
    }
#define HR_RETURN(op)                                 \
    do {                                              \
        HRESULT hr__ = (op);                          \
        if (FAILED(hr__) {                    \
            DxTrace(__FILEW__, __LINE__, hr__, L#op); \
            return false;                             \
        }                                             \
    } while (0)

#define HR(op)                                        \
    do {                                              \
        HRESULT hr__ = (op);                          \
        if (FAILED(hr__)) {                    \
            DxTrace(__FILEW__, __LINE__, hr__, L#op); \
            assert(0);                                \
        }                                             \
    } while (0)

#ifndef ThrowIfFailed
#define ThrowIfFailed(x) HR(x)
#endif