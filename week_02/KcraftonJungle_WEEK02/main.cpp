
#include <windows.h>


#pragma comment(lib, "user32")
#pragma comment(lib, "d3d11")
#pragma comment(lib, "d3dcompiler")

#include <DirectXMath.h>

#include "Editor/ImGui/imgui.h"
#include "Editor/ImGui/imgui_internal.h"
#include "Editor/ImGui/imgui_impl_dx11.h"
#include "Editor/imGui/imgui_impl_win32.h"

#include "Engine/Rendering/URenderer.h"
#include "Engine/Rendering/RenderSceneExtractor.h"
#include "Engine/Rendering/OverlayBuilder.h"
#include "Engine/Rendering/RenderQueue.h"

#include "Engine/World/Sphere.h"
#include "Engine/World/Transform.h"
#include "Engine/Editor/EditorSession.h"



extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    // 윈도우에 저장된 포인터를 다시 URenderer 타입으로 형변환해서 가져옴
    URenderer* pRenderer = reinterpret_cast<URenderer*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));

    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    case WM_SIZE:
        // pRenderer가 NULL이 아닐 때만 안전하게 호출
        if (wParam != SIZE_MINIMIZED && pRenderer != nullptr)
        {
            UINT width = LOWORD(lParam);
            UINT height = HIWORD(lParam);
            pRenderer->OnResize(width, height);
        }
        break;
    //    break;
    //case WM_PAINT:
    //    if (pRenderer)
    //    {
    //        // 여기서 강제로 한 번 그려줍니다. 
    //        // 단, Flush에 필요한 queue나 session 데이터를 가져올 방법이 필요합니다.
    //        // 보통 AppContext나 전역 객체를 통해 현재 상태를 가져와서 그립니다.

    //        //
    //        //pRenderer->BeginFrame();
    //        //pRenderer->Flush(queue, ed);
    //        //pRenderer->EndFrame();
    //        //

    //        // PAINT 처리를 완료했다고 알림
    //        //ValidateRect(hWnd, NULL);
    //    }
    //    break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

#include "Engine/Platform/PlatformEvents.h"



#include "AppContext.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{


    AppContext ctx;
    if (!ctx.Initialize("MyEngine", 1280, 720)) return -1;

    float deltaTime = 0.016f;

    while (ctx.Window.PollMessages()) {

        // ① ImGui 프레임 시작 — Poll() 보다 반드시 먼저
        //    이후 ImGui::GetIO().WantCaptureMouse/Keyboard 체크 가능
        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        // ② 플랫폼 이벤트 브로드캐스트
        //    InputRouter::Route()가 적재한 큐를 여기서 발행
        //    WantCaptureMouse/Keyboard가 참이면 Mouse/Key 브로드캐스트 건너뜀
        //    OnResize는 ImGui 선점과 무관하게 항상 발행
        PlatformEvents::Poll();

        // ③ 패널 렌더 & UI 입력 수집
        //    Property 수정 → Dispatch(SetTransformCommand)
        //    툴 전환 → ctx.Editor.Tools.SetMode(...)
        ctx.Panels.RenderAll(ctx);

        // ④ 카메라 입력 처리
        //    ImGui가 입력을 선점한 프레임은 건너뜀
        const InputState& input = InputRouter::GetState();
        if (!input.IsMouseCapturedByImGui() && !input.IsKeyboardCapturedByImGui()) {
            ctx.Editor.ProcessCameraInput(input, deltaTime);
        }

        // ⑤ D3D11 렌더
        ctx.Renderer.BeginFrame();

        RenderQueue queue;
        RenderSceneExtractor::Extract(ctx, queue);
        OverlayBuilder::Build(ctx.Editor, ctx, queue);  // ActiveTool Gizmo 포함
        ctx.Renderer.Flush(queue, ctx.Editor);

        // ⑥ ImGui 최종 출력
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


        ctx.Renderer.EndFrame();

    }

    ctx.Shutdown();
    return 0;
}
