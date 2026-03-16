#include "AppContext.h"
#include "Editor/ImGui/imgui.h"
#include "Editor/ImGui/imgui_impl_dx11.h"
#include "Editor/ImGui/imgui_impl_win32.h"

#include "Engine/Editor/Commands/DeleteObjectCommand.h"
#include "Engine/Platform/PlatformEvents.h"
#include "Engine/Editor/Tools/SelectTool.h"
#include "Engine/Editor/Tools/TranslateTool.h"
#include "Engine/Editor/Tools/RotateTool.h"
#include "Engine/Editor/Tools/ScaleTool.h"
#include "Engine/Foundation/Core/log.h"

bool AppContext::Initialize(const FString& windowTitle, int32 width, int32 height)
{
    if (!Window.Initialize(windowTitle, width, height)) {
        OutputDebugStringA("[ERROR] Window 초기화 실패\n");
        return false;
    }

    if (!Renderer.Initialize(Window.GetHWND(), width, height)) {
        OutputDebugStringA("[ERROR] Renderer 초기화 실패\n");
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplWin32_Init((void*)Window.GetHWND());
    ImGui_ImplDX11_Init(Renderer.Device, Renderer.DeviceContext);

    CurrentWorld.SetContext(this);
    RegisterBuiltinTypes();
    RegisterPanels();
    RegisterTools();
    SubscribeEvents();

    UE_LOG("Hello World = %d", 2024);
    UE_LOG("Hello World = %d", 2025);
    UE_LOG("Hello World = %d", 2026);

    return true;
}

void AppContext::RegisterBuiltinTypes()
{
    Classes.Register("Cube", UCubeComp::StaticClass());
    Classes.Register("Sphere", USphereComp::StaticClass());
    Classes.Register("Plane", UPlaneComp::StaticClass());
    Classes.Register("Triangle", UTriangleComp::StaticClass());
}

void AppContext::RegisterPanels()
{
      
    auto* property = new PropertyPanel();
    auto* stat = new StatPanel();
    auto* toolbar = new ToolbarPanel();
    auto* console = new ConsolePanel();
    auto* outliner = new OutlinerPanel();


    OwnedPanels.push_back(property);
    OwnedPanels.push_back(stat);
    OwnedPanels.push_back(toolbar);
    OwnedPanels.push_back(console);;
    OwnedPanels.push_back(outliner);

    Panels.Register(property);
    Panels.Register(stat);
    Panels.Register(toolbar);
    Panels.Register(console);
    Panels.Register(outliner);
}

void AppContext::RegisterTools()
{
    auto* select = new SelectTool();
    auto* translate = new TranslateTool();
    auto* rotate = new RotateTool();
    auto* scale = new ScaleTool();

    SelectionTool = select;
    Translate = translate;

    OwnedTools.push_back(select);
    OwnedTools.push_back(translate);
    OwnedTools.push_back(rotate);
    OwnedTools.push_back(scale);

    Editor.Tools.RegisterTool(translate);
    Editor.Tools.RegisterTool(rotate);
    Editor.Tools.RegisterTool(scale);
    // 기본 활성 툴
    Editor.Tools.ActivateTool("Translate");
    CapturedManipulationTool = nullptr;
}

void AppContext::SubscribeEvents()
{
    MouseDownHandle = PlatformEvents::OnMouseDown.Bind(
        [this](const MouseDownEvent& e) {
            const auto& keys = InputRouter::GetState().Keys;
            MouseEvent mouseEvent{
                e.X, e.Y,
                e.Button == 0, e.Button == 1, e.Button == 2,
                keys[VK_CONTROL], keys[VK_SHIFT], keys[VK_MENU],
                0, 0 };
            ITool* activeTransformTool = Editor.Tools.GetActiveTool();
            if (activeTransformTool != nullptr &&
                activeTransformTool->TryBeginManipulation(mouseEvent, *this))
            {
                CapturedManipulationTool = activeTransformTool;
                return;
            }
            CapturedManipulationTool = nullptr;
            if (SelectionTool != nullptr)
            {
                SelectionTool->OnMouseDown(mouseEvent, *this);
            }
        });

    MouseMoveHandle = PlatformEvents::OnMouseMove.Bind(
        [this](const MouseMoveEvent& e) {
            const auto& keys = InputRouter::GetState().Keys;
            MouseEvent mouseEvent{
                e.X, e.Y,
                false, e.RightDown, false,
                keys[VK_CONTROL], keys[VK_SHIFT], keys[VK_MENU],
                e.DeltaX, e.DeltaY };
            if (Editor.Tools.GetActiveTool())
                Editor.Tools.GetActiveTool()->OnMouseMove(mouseEvent, *this);
        });

    MouseUpHandle = PlatformEvents::OnMouseUp.Bind(
        [this](const MouseUpEvent& e) {
            const auto& keys = InputRouter::GetState().Keys;
            MouseEvent mouseEvent{
                e.X, e.Y,
                e.Button == 0, e.Button == 1, e.Button == 2,
                keys[VK_CONTROL], keys[VK_SHIFT], keys[VK_MENU],
                0, 0 };
            if (CapturedManipulationTool != nullptr)
            {
                CapturedManipulationTool->OnMouseUp(mouseEvent, *this);
                CapturedManipulationTool = nullptr;
            }
        });

    KeyDownHandle = PlatformEvents::OnKeyDown.Bind(
        [this](const KeyDownEvent& e) {
            // Space Bar → Transform 모드 순환 (Translate → Rotate → Scale)
            if (e.KeyCode == VK_SPACE) {
                using M = ETransformMode;
                auto cur = Editor.Tools.GetMode();
                Editor.Tools.SetMode(
                    cur == M::Translate ? M::Rotate :
                    cur == M::Rotate ? M::Scale : M::Translate);
            }else if (e.KeyCode == VK_DELETE) {
                TArray<USceneComponent*> selected = Editor.Selection.GetAll();

                for (USceneComponent* Object : selected) {
                    if (Object)
                        Dispatch(new DeleteObjectCommand(*this, Object->GetUUID()));
                }
            }
            // 툴에도 전달
            KeyEvent keyEvent{ e.KeyCode, true };
            if (Editor.Tools.GetActiveTool())
                Editor.Tools.GetActiveTool()->OnKeyDown(keyEvent, *this);
        });

    KeyUpHandle = PlatformEvents::OnKeyUp.Bind(
        [this](const KeyUpEvent& e) {
            // 현재 KeyUp을 처리하는 ITool 인터페이스는 없으므로 빈 상태 유지
            // 향후 OnKeyUp 슬롯 추가 시 여기에 구현
        });

    ResizeHandle = PlatformEvents::OnResize.Bind(
        [this](const ResizeEvent& e) { 
            Renderer.OnResize(e.Width, e.Height);
            Window.UpdateSize(e.Width, e.Height); 

            if (e.Height > 0)
                Editor.AspectRatio = (float)e.Width / (float)e.Height;
        });


    OutlinerPanel* outliner = (OutlinerPanel*)Panels.Find("Outliner");
    PropertyPanel* property = (PropertyPanel*)Panels.Find("Property");

    if (property) {
        property->ObjectDestroyedHandle = OnObjectDestroyed.Bind(
            [property](const ObjectDestroyedEvent& e) {
                property->OnObjectDestroyed(e);
            });
        property->SelectionChangedHandle = Editor.Selection.OnSelectionChanged.Bind(
            [property](const SelectionChangedEvent& e) {
                property->OnSelectionChanged(e);
            });
    }
}


void AppContext::Shutdown()
{
    //unbind
    PlatformEvents::OnMouseDown.Unbind(MouseDownHandle);
    PlatformEvents::OnMouseMove.Unbind(MouseMoveHandle);
    PlatformEvents::OnMouseUp.Unbind(MouseUpHandle);
    PlatformEvents::OnKeyDown.Unbind(KeyDownHandle);
    PlatformEvents::OnKeyUp.Unbind(KeyUpHandle);
    PlatformEvents::OnResize.Unbind(ResizeHandle);

    PropertyPanel* property = static_cast<PropertyPanel*>(Panels.Find("Property"));

    if (property) {
        Editor.Selection.OnSelectionChanged.Unbind(property->SelectionChangedHandle);
        OnObjectDestroyed.Unbind(property->ObjectDestroyedHandle);
    }
    OutlinerPanel* outliner = static_cast<OutlinerPanel*>(Panels.Find("Outliner"));

    if (outliner) {
        Editor.Selection.OnSelectionChanged.Unbind(outliner->SelectionChangedHandle);
        OnObjectDestroyed.Unbind(outliner->ObjectDestroyedHandle);
    }

    for (IEditorPanel* Panel : OwnedPanels) {
        delete Panel;
    }
    OwnedPanels.clear();

    for (ITool* tool : OwnedTools)
        delete tool;
    OwnedTools.clear();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // 4. Renderer GPU 리소스 해제
    Renderer.Shutdown();

    // 5. ObjectStore 전체 비움 + UObject 메모리 해제
    Objects.Clear();

    // 6. Window 해제
    Window.Shutdown();
}

// Always receive new Instance -> must delete.
void AppContext::Dispatch(ICommand* cmd)
{
    cmd->Execute();
    delete cmd;
}

void AppContext::NewScene()
{
    // Load() 수명 규약과 동일한 순서
    // 1. Selection 초기화
    Editor.Selection.Clear();

    // 2. UUID 목록 복사 후 OnObjectDestroyed 순차 발행
    TArray<uint32> uuids;
    for (UObject* obj : CurrentWorld.GetAllObjects())
        uuids.push_back(obj->GetUUID());

    for (uint32 uuid : uuids)
        OnObjectDestroyed.Broadcast({ uuid });

    // 3. ObjectStore 전체 비움 + 메모리 해제
    Objects.Clear();

    // 4. UUID 카운터 초기화
    UUIDs.SyncNextUUID(1);
}
