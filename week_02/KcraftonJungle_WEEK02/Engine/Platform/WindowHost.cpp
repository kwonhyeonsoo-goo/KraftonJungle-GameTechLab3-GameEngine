// WindowHost.cpp
#include "WindowHost.h"
#include "../../Editor/ImGui/imgui_impl_win32.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    switch (message) {
    case WM_SIZE:
        // InputRouter를 통해 Resize 이벤트 적재
        // InputRouter::Route(hWnd, message, wParam, lParam);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

bool WindowHost::Initialize(const FString& title, int32 width, int32 height) {
    Width = width;
    Height = height;

    // 윈도우 클래스 등록
    WNDCLASSEXA wc = {};
    wc.cbSize = sizeof(WNDCLASSEXA);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = "MyEngineClass";
    RegisterClassExA(&wc);

    // 윈도우 생성
    Hwnd = CreateWindowExA(
        0,
        "MyEngineClass",
        title.c_str(),
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT,
        width, height,
        nullptr, nullptr,
        GetModuleHandle(nullptr),
        nullptr
    );

    if (!Hwnd) return false;

    ShowWindow(Hwnd, SW_SHOW);
    UpdateWindow(Hwnd);
    return true;
}

void WindowHost::Shutdown() {
    if (Hwnd) {
        DestroyWindow(Hwnd);
        Hwnd = nullptr;
    }
    UnregisterClassA("MyEngineClass", GetModuleHandle(nullptr));
}

bool WindowHost::PollMessages() {
    MSG msg = {};
    while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
        if (msg.message == WM_QUIT)
            return false;
    }
    return true;
}

HWND  WindowHost::GetHWND()   const { return Hwnd; }
int32 WindowHost::GetWidth()  const { return Width; }
int32 WindowHost::GetHeight() const { return Height; }