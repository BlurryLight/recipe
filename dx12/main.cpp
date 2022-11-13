#define _CRT_SECURE_NO_WARNINGS
#include "d3dApp.hh"
#include "d3dUtils.hh"
#include <windows.h>

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow) {
    // Register the window class.
    AllocConsole();
	freopen("CONIN$", "r", stdin);
	freopen("CONOUT$", "w", stdout);
	freopen("CONOUT$", "w", stderr);
    PD::D3DApp app(hInstance);
    app.Initialize();
    return app.MessageLoopRun();
}
