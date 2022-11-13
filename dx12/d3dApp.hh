//
// Created by zhong on 2021/3/27.
//


#ifndef DXWINDOW_D3DAPP_HH
#define DXWINDOW_D3DAPP_HH

// clang-format off
#include "defines.h"
// clang-format on
#include "d3dUtils.hh"
#include <string>

namespace PD {
    class D3DApp : public Noncopyable {
        //    protected:
    public:
        explicit D3DApp(HINSTANCE hinstance);
        virtual ~D3DApp();

    public:
        HINSTANCE GetHInstance() const;
        HWND GetHMND() const;
        static D3DApp *GetD3dDApp();
        float GetAspectRatio() const;
        bool GetMSAAState() const;
        bool SetMSAAState(bool val);// return old state

        int MessageLoopRun();

        virtual bool Initialize();
        virtual LRESULT AppMessageProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

    protected:
        virtual void OnResizeCallback();
        virtual void OnMouseDownCallback(WPARAM btnState, int x, int y) {
        }
        virtual void OnMouseUpCallback(WPARAM btnState, int x, int y){};
        virtual void OnMouseMoveCallback(WPARAM btnState, int x, int y) {}

        bool InitMainWindow();
        bool initDirect3D() { return true; }


    protected:
        static D3DApp *D3DApp_;
        HINSTANCE hinstance_ = nullptr;
        HWND hMainWindow_ = nullptr;
        bool AppPaused_ = false;
        bool AppMinimized_ = false;
        bool AppMaximized_ = false;
        bool AppResizing_ = false;
        bool AppFullScreen_ = false;

        bool AppMSAA_ = false;
        UINT AppMSAAQuality_ = 0;

        std::wstring AppWindowTitle_ = L"d3d test";
        int width_ = 800;
        int height_ = 600;
    };
}// namespace PD

#endif// DXWINDOW_D3DAPP_HH
