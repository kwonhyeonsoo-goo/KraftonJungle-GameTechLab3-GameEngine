#include "UWindow.h"
#include "UWindow.h"

#include <stdexcept>

UWindow::UWindow(const HINSTANCE hInst, FWindowDesc desc) : HInstance(hInst), Desc(std::move(desc))
{
    RegisterWindowClass();
    Create();
}

UWindow::~UWindow()
{
    if (Hwnd)
    {
        ::DestroyWindow(Hwnd);
        Hwnd = nullptr;
    }

    UnregisterWindowClass();
}

HWND UWindow::Handle() const
{
    return Hwnd;
}

void UWindow::Show(const int nShowCmd) const
{
    ShowWindow(Hwnd, nShowCmd);
    UpdateWindow(Hwnd);
}

void UWindow::RegisterWindowClass() const
{
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = &UWindow::WndProcSetup; // Setup -> Thunk -> instance
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = HInstance;
    wc.hIcon = nullptr;
    wc.hCursor = ::LoadCursor(nullptr, IDC_ARROW);
    wc.hbrBackground = nullptr;
    wc.lpszMenuName = nullptr;
    wc.lpszClassName = Desc.ClassName.c_str();
    wc.hIconSm = nullptr;

    if (!RegisterClassExW(&wc))
    {
        throw std::runtime_error("RegisterClassExW failed.");
    }
}

void UWindow::UnregisterWindowClass() const
{
    UnregisterClassW(Desc.ClassName.c_str(), HInstance);
}

void UWindow::Create()
{
    RECT rc{ 0, 0, Desc.Width, Desc.Height };
    AdjustWindowRectEx(&rc, Desc.Style, FALSE, Desc.ExStyle);

    Hwnd = CreateWindowExW(
        Desc.ExStyle,
        Desc.ClassName.c_str(),
        Desc.Title.c_str(),
        Desc.Style,
        CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left,
        rc.bottom - rc.top,
        nullptr, nullptr,
        HInstance,
        this
    );

    if (!Hwnd)
    {
        throw std::runtime_error("CreateWindowExW failed.");
    }
}

void UWindow::OnCreate()
{

}

void UWindow::OnDestroy()
{
    PostQuitMessage(0);
}

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg,
    WPARAM wParam, LPARAM lParam);

LRESULT UWindow::OnMessage(const UINT msg, const WPARAM wp, const LPARAM lp)
{
    if (ImGui_ImplWin32_WndProcHandler(Hwnd, msg, wp, lp))
    {
        return true;
    }

    switch (msg)
    {
    case WM_CREATE: OnCreate(); return 0;
    case WM_DESTROY: OnDestroy(); return 0;
    default:        return DefWindowProcW(Hwnd, msg, wp, lp);
    }
}

LRESULT UWindow::WndProcSetup(const HWND hWnd, const UINT msg, const WPARAM wp, const LPARAM lp)
{
    if (msg == WM_NCCREATE)
    {
        const auto* cs = reinterpret_cast<CREATESTRUCTW*>(lp);  // NOLINT(performance-no-int-to-ptr)
        auto* self = static_cast<UWindow*>(cs->lpCreateParams);

        ::SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(self));
        ::SetWindowLongPtrW(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&UWindow::WndProcThunk));

        self->Hwnd = hWnd;
        return self->OnMessage(msg, wp, lp);
    }
    return ::DefWindowProcW(hWnd, msg, wp, lp);
}

LRESULT UWindow::WndProcThunk(const HWND hWnd, const UINT msg, const WPARAM wp, const LPARAM lp)
{
    auto* self = reinterpret_cast<UWindow*>(::GetWindowLongPtrW(hWnd, GWLP_USERDATA));  // NOLINT(performance-no-int-to-ptr)
    return self ? self->OnMessage(msg, wp, lp) : ::DefWindowProcW(hWnd, msg, wp, lp);
}
