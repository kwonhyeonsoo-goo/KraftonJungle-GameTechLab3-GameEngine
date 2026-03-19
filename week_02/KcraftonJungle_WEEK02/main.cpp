
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

#include "Engine/Platform/PlatformEvents.h"

#include "AppContext.h"
#include <iostream>


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#ifdef _DEBUG
    // 디버그 모드에서만 콘솔 창을 할당합니다.
    AllocConsole();
    FILE* File = nullptr;
    freopen_s(&File, "CONOUT$", "w", stdout);

    std::cout << "Engine Initialization Started..." << std::endl;
#endif

    AppContext ctx;
    if (!ctx.Initialize("MyEngine", 1600, 900)) return -1;

    const int TargetFPS = 60;
    const double TargetFrameTime = 1000.0 / TargetFPS;

    LARGE_INTEGER Frequency;
    QueryPerformanceFrequency(&Frequency);

    LARGE_INTEGER PrevCounter;
    QueryPerformanceCounter(&PrevCounter);

    while (ctx.Window.PollMessages()) {
        LARGE_INTEGER NowCounter;
        QueryPerformanceCounter(&NowCounter);
        float DeltaTime = static_cast<float>(
            (NowCounter.QuadPart - PrevCounter.QuadPart) /
            static_cast<double>(Frequency.QuadPart)
            );
        PrevCounter = NowCounter;
        ctx.Window.SetCurrentDeltaTime(DeltaTime);

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
            ctx.Editor.ProcessCameraInput(input, DeltaTime);
        }

        // ⑤ D3D11 렌더
        ctx.Renderer.BeginFrame();
        InputRouter::ResetDelta();

        RenderQueue queue;
        RenderSceneExtractor::Extract(ctx, queue);
        OverlayBuilder::Build(ctx.Editor, ctx, queue);  // ActiveTool Gizmo 포함
        ctx.Renderer.Flush(queue, ctx.Editor);

        // ⑥ ImGui 최종 출력
        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());


        ctx.Renderer.EndFrame();

        LARGE_INTEGER FrameEndCounter;
        double FrameElapsedMs = 0.0;
        do
        {
            Sleep(0);
            QueryPerformanceCounter(&FrameEndCounter);
            FrameElapsedMs =
                (FrameEndCounter.QuadPart - NowCounter.QuadPart) * 1000.0 /
                static_cast<double>(Frequency.QuadPart);
        } while (FrameElapsedMs < TargetFrameTime);
    }

#ifdef _DEBUG
    // 프로그램 종료 전에 콘솔을 해제합니다.
    FreeConsole();
#endif

    ctx.Shutdown();
    return 0;
}
