#include "EngineTest.h"
#include "Renderer/Renderer.h"
#include <iostream>

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (msg == WM_DESTROY) { PostQuitMessage(0); return 0; }
    return DefWindowProc(hWnd, msg, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE, LPSTR, int)
{
    WNDCLASSEX wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = L"TestWindow";
    RegisterClassEx(&wc);

    HWND hwnd = CreateWindowEx(
        0, L"TestWindow", L"Renderer Test",
        WS_OVERLAPPEDWINDOW, 100, 100, 1280, 720,
        nullptr, nullptr, hInstance, nullptr
    );
    ShowWindow(hwnd, SW_SHOW);

    CRenderer renderer;
    if (!renderer.Initialize(hwnd, 1280, 720))
        return -1;

    MSG msg = {};
    while (msg.message != WM_QUIT)
    {
        if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
        else
        {
            renderer.BeginFrame();
 
            renderer.EndFrame();
        }
    }

    return 0;
}