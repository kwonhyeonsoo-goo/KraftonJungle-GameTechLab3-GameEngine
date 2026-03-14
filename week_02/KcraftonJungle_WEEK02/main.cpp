
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


#include "AppContext.h"


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{

    //TODO : WindowHost로 옮기기

    HINSTANCE _hInstance = GetModuleHandle(nullptr); // WinMain 파라미터 대신 이걸로 대체 가능

    //Window.Initialize(hInstance, ...);

    WCHAR WindowClass[] = L"JungleWindowClass";

    WCHAR Title[] = L"Game Tech Lab";

    WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };

    RegisterClassW(&wndclass);

    HWND hWnd = CreateWindowExW(0, WindowClass, Title, WS_POPUP | WS_VISIBLE | WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1024, 1024,
        nullptr, nullptr, hInstance, nullptr);
#pragma endregion

    URenderer renderer;

    renderer.Create(hWnd);

    // hWnd(윈도우)에 renderer 객체의 주소를 '사용자 데이터'로 저장
    SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)&renderer);

    renderer.CreateShader();

    renderer.CreateConstantBuffer();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);



    float scaleMod = 0.1f;

    bool bIsExit = false;   


    const float leftBorder = -10.0f;
    const float rightBorder = 10.0f;
    const float topBorder = -10.0f;
    const float bottomBorder = 10.0f;
    const float frontBorder = 10.0f;
    const float backBorder = -10.0f;

    const float sphereRadius = 1.0f;

    bool bBoundBallToScreen = true;

    bool bPinballMovement = true;

    FVector	offset(0.0f);
    FVector	rotation(0.0f);
    FVector	scale(1.0f, 2.0f, 3.0f);

    FVector	velocity(0.0f);

    const float ballspeed = 0.01f;    
    velocity.x = ((float)(rand() % 100 - 50)) * ballspeed;
    velocity.y = ((float)(rand() % 100 - 50)) * ballspeed;
    velocity.z = ((float)(rand() % 100 - 50)) * ballspeed;

    const int targetFPS = 30;  
    const double targetFrameTime = 1000.0 / targetFPS;

    LARGE_INTEGER frequency;
    QueryPerformanceFrequency(&frequency);

    LARGE_INTEGER startTime, endTime;
    double elapsedTime = 0.0;


    while (bIsExit == false)
    {
        QueryPerformanceCounter(&startTime);

        MSG msg;

        while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);

            if (msg.message == WM_QUIT)
            {
                bIsExit = true;
                break;
            }
            else if (msg.message == WM_KEYDOWN) 
            {
                if (msg.wParam == VK_LEFT)
                {
                    offset.x -= 0.01f;
                }
                if (msg.wParam == VK_RIGHT)
                {
                    offset.x += 0.01f;
                }
                if (msg.wParam == VK_UP)
                {
                    offset.y += 0.01f;
                }
                if (msg.wParam == VK_DOWN)
                {
                    offset.y -= 0.01f;
                }

                if (bBoundBallToScreen)
                {
                    float renderRadius = sphereRadius * scaleMod;
                    if (offset.x < leftBorder + renderRadius)
                    {
                        offset.x = leftBorder + renderRadius;
                    }
                    if (offset.x > rightBorder - renderRadius)
                    {
                        offset.x = rightBorder - renderRadius;
                    }
                    if (offset.y < topBorder + renderRadius)
                    {
                        offset.y = topBorder + renderRadius;
                    }
                    if (offset.y > bottomBorder - renderRadius)
                    {
                        offset.y = bottomBorder - renderRadius;
                    }
                    if (offset.z < frontBorder + renderRadius)
                    {          
                        offset.z = frontBorder + renderRadius;
                    }          
                    if (offset.z > backBorder + renderRadius)
                    {          
                        offset.z = backBorder - renderRadius;
                    }

                }
            }
        }


        if (bPinballMovement)
        {
            offset.x += velocity.x;
            offset.y += velocity.y;
            offset.z += velocity.z;


            rotation.y += velocity.y;


            float renderRadius = sphereRadius * scaleMod;
            if (offset.x < leftBorder + renderRadius)
            {
                velocity.x *= -1.0f;
            }
            if (offset.x > rightBorder - renderRadius)
            {
                velocity.x *= -1.0f;
            }
            if (offset.y < topBorder + renderRadius)
            {
                velocity.y *= -1.0f;
            }
            if (offset.y > bottomBorder - renderRadius)
            {
                velocity.y *= -1.0f;
            }
            if (offset.z < frontBorder + renderRadius)
            {
                velocity.z *= -1.0f;
            }
            if (offset.z > backBorder - renderRadius)
            {
                velocity.z *= -1.0f;
            }
        }



        AppContext ctx = {};


        UCubeComp* sphere = new UCubeComp();

        Transform trans = Transform();
        trans.Location = offset;
        trans.Rotation = { 0,0,0 };
        trans.Scale = scale;
        sphere->SetTransform(trans);

        ctx.Objects.Add(sphere);

        USphereComp* cube = new USphereComp();

        trans = Transform();
        trans.Location = offset + 1;
        trans.Rotation = rotation;
        trans.Scale = scale;
        cube->SetTransform(trans);

        ctx.Objects.Add(cube);

        UPlaneComp* plane = new UPlaneComp();

        trans = Transform();
        trans.Location = offset - 1;
        trans.Rotation = rotation;
        trans.Scale = scale;
        plane->SetTransform(trans);

        ctx.Objects.Add(plane);

        RenderQueue queue = RenderQueue();;
        EditorSession ed;
        RenderSceneExtractor::Extract(ctx, queue);
        OverlayBuilder::Build(ed, ctx, queue);

        renderer.BeginFrame();

        renderer.Flush(queue, ed);



#pragma region ImGui

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Jungle Property Window");
        ImGui::Text("Hello, Jungle!");


        ImGui::Checkbox("Bound Ball To Screen", &bBoundBallToScreen);

        ImGui::Checkbox("Pinball Movement", &bPinballMovement);

        ImGui::End();

        ImGui::Render();
        ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

#pragma endregion

        renderer.EndFrame();


        do
        {
            Sleep(0);

            QueryPerformanceCounter(&endTime);

            elapsedTime = (endTime.QuadPart - startTime.QuadPart) * 1000.0 / frequency.QuadPart;

        } while (elapsedTime < targetFrameTime);

    }

    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();


    renderer.ReleaseConstantBuffer();

    renderer.ReleaseShader();

    renderer.Release();



    return 0;
}
