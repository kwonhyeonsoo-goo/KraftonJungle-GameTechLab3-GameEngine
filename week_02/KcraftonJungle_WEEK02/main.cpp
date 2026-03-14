
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

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, message, wParam, lParam))
    {
        return true;
    }

    switch (message)
    {
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }

    return 0;
}

#include "Engine/Mesh/Triangle.h"
#include "Engine/Mesh/Rect.h"
#include "Engine/Mesh/Cube.h"
#include "Engine/Mesh/Sphere.h"

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

    AppContext ctx;

    URenderer renderer;

    renderer.Create(hWnd);

    renderer.CreateShader();

    renderer.CreateConstantBuffer();

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    ImGui_ImplWin32_Init((void*)hWnd);
    ImGui_ImplDX11_Init(renderer.Device, renderer.DeviceContext);

    UINT numVerticesTriangle = sizeof(triangle_vertices) / sizeof(FVertexSimple);
    UINT numVerticesCube = sizeof(cube_vertices) / sizeof(FVertexSimple);
    UINT numVerticesSphere = sizeof(sphere_vertices) / sizeof(FVertexSimple);
	UINT numIndicesRect = sizeof(rect_indices) / sizeof(UINT);

    float scaleMod = 0.1f;

    for (UINT i = 0; i < numVerticesSphere; ++i)
    {
        sphere_vertices[i].x *= scaleMod;
        sphere_vertices[i].y *= scaleMod;
        sphere_vertices[i].z *= scaleMod;
    }

    ID3D11Buffer* vertexBufferTriangle = renderer.CreateVertexBuffer(triangle_vertices, sizeof(triangle_vertices));
    ID3D11Buffer* vertexBufferCube = renderer.CreateVertexBuffer(cube_vertices, sizeof(cube_vertices));
    ID3D11Buffer* vertexBufferSphere = renderer.CreateVertexBuffer(sphere_vertices, sizeof(sphere_vertices));

	ID3D11Buffer* vertexBufferRect = renderer.CreateVertexBuffer(rect_vertices, sizeof(rect_vertices));
    ID3D11Buffer* indexBufferRect = renderer.CreateIndexBuffer(rect_indices, sizeof(rect_indices));

    bool bIsExit = false;   

    enum ETypePrimitive
    {
        EPT_Triangle,
        EPT_Cube,
        EPT_Sphere,
        EPT_Rect,
        EPT_Max,
    };

    ETypePrimitive typePrimitive = EPT_Sphere
        ;

    const float leftBorder = -1.0f;
    const float rightBorder = 1.0f;
    const float topBorder = -1.0f;
    const float bottomBorder = 1.0f;
    const float frontBorder = 10.0f;
    const float backBorder = -10.0f;

    const float sphereRadius = 1.0f;

    bool bBoundBallToScreen = true;

    bool bPinballMovement = true;

    FVector	offset(0.0f);
    FVector	rotation(0.0f);
    FVector	scale(1.0f, 2.0f, 3.0f);

    FVector	velocity(0.0f);

    const float ballspeed = 0.001f;    
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

            rotation.x += velocity.x;
            rotation.y += velocity.y;
            rotation.z += velocity.z;

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


        FMatrix mTranslation = FMatrix::Translation({ offset.x, offset.y, offset.z });

        rotation.x += 1.0f / targetFrameTime;
        rotation.y += 2.0f / targetFrameTime;
        rotation.z += 3.0f / targetFrameTime;

        FMatrix quat =
            FMatrix::RotationX(rotation.x) *
            FMatrix::RotationY(rotation.y) *
            FMatrix::RotationZ(rotation.z);

        FMatrix mRotationQuat = quat; // DirectX::XMMatrixRotationQuaternion(quat);

        FMatrix mScale = FMatrix::Scale({ scale.x, scale.y, scale.z });

        FMatrix mWorld = mScale * mRotationQuat * mTranslation;

        FVector eye = { 0.0f, 0.0f, -20.0f};   // 카메라 위치
        FVector target = { 0.0f, 0.0f, 0.0f }; // 보는 위치
        FVector up = { 0.0f, 1.0f, 0.0f };     // 위 방향

        FMatrix mView = FMatrix::LookAt(eye, target, up);

        float fov = DirectX::XMConvertToRadians(60.0f);
        float aspect = 1.0f;   // 창이 정사각형이면 1
        float nearZ = 0.1f;
        float farZ = 100.0f;

        FMatrix mProjection =
            FMatrix::Perspective(
                fov,
                aspect,
                nearZ,
                farZ
            );

        FConstants constants;
        //constants.MVP; //= mWorld * mView * mProjection;
        constants.MVP = mWorld * mView * mProjection;

        renderer.Prepare();
        renderer.PrepareShader();

        renderer.UpdateConstant(constants);
		renderer.UpdateMVP(constants.MVP);
        switch (typePrimitive)
        {
        case EPT_Triangle:
            renderer.RenderPrimitive(vertexBufferTriangle, numVerticesTriangle);
            break;
        case EPT_Cube:
            renderer.RenderPrimitive(vertexBufferCube, numVerticesCube);
            break;
        case EPT_Sphere:
            renderer.RenderPrimitive(vertexBufferTriangle, numVerticesTriangle);
            renderer.RenderPrimitive(vertexBufferSphere, numVerticesSphere);
            break;
		case EPT_Rect:
            renderer.RenderIndexedPrimitive(vertexBufferRect, indexBufferRect);

        }

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



        renderer.SwapBuffer();

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

    renderer.ReleaseVertexBuffer(vertexBufferTriangle);
    renderer.ReleaseVertexBuffer(vertexBufferCube);
    renderer.ReleaseVertexBuffer(vertexBufferSphere);

    renderer.ReleaseConstantBuffer();

    renderer.ReleaseShader();

    renderer.Release();



    return 0;
}
