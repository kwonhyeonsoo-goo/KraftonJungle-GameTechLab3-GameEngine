// WindowHost.cpp
#include "WindowHost.h"
#include "../../Editor/ImGui/imgui_impl_win32.h"
#include "InputRouter.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
        return true;

    InputRouter::Route(hWnd, message, wParam, lParam); // ← 이게 있어야 함

    switch (message) {
    case WM_SIZE:
        // InputRouter를 통해 Resize 이벤트 적재
        // InputRouter::Route(hWnd, message, wParam, lParam);

        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_KILLFOCUS:
        // 윈도우가 포커스를 잃었을 때 입력 비활성화
        InputRouter::SetWindowFocus(false);
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
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(WNDCLASSEXW);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(nullptr);
    wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
    wc.lpszClassName = L"MyEngineClass";
    RegisterClassExW(&wc);

    // ANSI(string)를 Wide(wstring)로 변환
    int nwLen = MultiByteToWideChar(CP_ACP, 0, title.c_str(), -1, NULL, 0);
    std::wstring wTitle(nwLen, L'\0');
    MultiByteToWideChar(CP_ACP, 0, title.c_str(), -1, &wTitle[0], nwLen);

    // 변환된 wTitle.c_str() 사용
    Hwnd = CreateWindowExW(
        0,
        L"MyEngineClass",
        wTitle.c_str(),
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
    UnregisterClassW(L"MyEngineClass", GetModuleHandle(nullptr));
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

void WindowHost::UpdateSize(int32 width, int32 height)
{
    Width = width;
    Height = height;
}

float WindowHost::GetCurrentDeltaTime() const
{
    return DeltaTime;
}

void WindowHost::SetCurrentDeltaTime(float NewDeltaTime)
{
    DeltaTime = NewDeltaTime;
}
