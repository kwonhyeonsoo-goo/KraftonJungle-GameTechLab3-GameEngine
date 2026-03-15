
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
        InputRouter::ResetDelta();

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
