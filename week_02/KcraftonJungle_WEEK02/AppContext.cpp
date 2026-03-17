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
#include <iostream>

bool AppContext::Initialize(const FString& windowTitle, int32 width, int32 height)
{
    if (!Window.Initialize(windowTitle, width, height)) {
        OutputDebugStringA("[ERROR] Window �ʱ�ȭ ����\n");
        return false;
    }

    if (!Renderer.Initialize(Window.GetHWND(), width, height)) {
        OutputDebugStringA("[ERROR] Renderer �ʱ�ȭ ����\n");
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
    auto property = std::make_unique<PropertyPanel>();
    auto stat     = std::make_unique<StatPanel>();
    auto toolbar  = std::make_unique<ToolbarPanel>();
    auto console  = std::make_unique<ConsolePanel>();
    auto outliner = std::make_unique<OutlinerPanel>();

    Panels.Register(property.get());
    Panels.Register(stat.get());
    Panels.Register(toolbar.get());
    Panels.Register(console.get());
    Panels.Register(outliner.get());

    OwnedPanels.push_back(std::move(property));
    OwnedPanels.push_back(std::move(stat));
    OwnedPanels.push_back(std::move(toolbar));
    OwnedPanels.push_back(std::move(console));
    OwnedPanels.push_back(std::move(outliner));
}

void AppContext::RegisterTools()
{
    auto select    = std::make_unique<SelectTool>();
    auto translate = std::make_unique<TranslateTool>();
    auto rotate    = std::make_unique<RotateTool>();
    auto scale     = std::make_unique<ScaleTool>();

    SelectionTool = select.get();
    Translate     = translate.get();

    Editor.Tools.RegisterTool(translate.get());
    Editor.Tools.RegisterTool(rotate.get());
    Editor.Tools.RegisterTool(scale.get());

    OwnedTools.push_back(std::move(select));
    OwnedTools.push_back(std::move(translate));
    OwnedTools.push_back(std::move(rotate));
    OwnedTools.push_back(std::move(scale));
    // �⺻ Ȱ�� ��
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
            // Space Bar �� Transform ��� ��ȯ (Translate �� Rotate �� Scale)
            if (e.KeyCode == VK_SPACE) {
                using M = ETransformMode;
                auto cur = Editor.Tools.GetMode();
                Editor.Tools.SetMode(
                    cur == M::Translate ? M::Rotate :
                    cur == M::Rotate ? M::Scale : M::Translate);
            }
            else if (e.KeyCode == '1') {
                using M = ETransformMode;
                auto cur = Editor.Tools.GetMode();
                Editor.Tools.SetMode( M::Translate );
            }else if (e.KeyCode == '2') {
                using M = ETransformMode;
                auto cur = Editor.Tools.GetMode();
                Editor.Tools.SetMode( M::Rotate);
            }else if (e.KeyCode == '3') {
                using M = ETransformMode;
                auto cur = Editor.Tools.GetMode();
                Editor.Tools.SetMode( M::Scale);
            }else if (e.KeyCode == VK_DELETE) {
                TArray<USceneComponent*> selected = Editor.Selection.GetAll();

                for (USceneComponent* Object : selected) {
                    if (Object)
                        Dispatch(std::make_unique<DeleteObjectCommand>(*this, Object->GetUUID()));
                }
            }
            // ������ ����
            KeyEvent keyEvent{ e.KeyCode, true };
            if (Editor.Tools.GetActiveTool())
                Editor.Tools.GetActiveTool()->OnKeyDown(keyEvent, *this);
        });

    KeyUpHandle = PlatformEvents::OnKeyUp.Bind(
        [this](const KeyUpEvent& e) {
            // ���� KeyUp�� ó���ϴ� ITool �������̽��� �����Ƿ� �� ���� ����
            // ���� OnKeyUp ���� �߰� �� ���⿡ ����
        });

    ResizeHandle = PlatformEvents::OnResize.Bind(
        [this](const ResizeEvent& e) { 
            Renderer.OnResize(e.Width, e.Height);
            Window.UpdateSize(e.Width, e.Height); 

            if (e.Height > 0)
                Editor.GetActiveViewport().Projection.AspectRatio = (float)e.Width / (float)e.Height;
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

    if (outliner) {
        outliner->ObjectDestroyedHandle = OnObjectDestroyed.Bind(
            [outliner](const ObjectDestroyedEvent& e) {
                outliner->OnObjectDestroyed(e);
            });
        outliner->SelectionChangedHandle = Editor.Selection.OnSelectionChanged.Bind(
            [outliner](const SelectionChangedEvent& e) {
                outliner->OnSelectionChanged(e);
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

    OwnedPanels.clear();
    OwnedTools.clear();
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    // 4. Renderer GPU ���ҽ� ����
    Renderer.Shutdown();

    // 5. ObjectStore ��ü ��� + UObject �޸� ����
    Objects.Clear();

    // 6. Window ����
    Window.Shutdown();
}

void AppContext::Dispatch(std::unique_ptr<ICommand> cmd)
{
    cmd->Execute();
}

void AppContext::NewScene()
{
    // Load() ���� �Ծ�� ������ ����
    // 1. Selection �ʱ�ȭ
    Editor.Selection.Clear();

    // 2. UUID ��� ���� �� OnObjectDestroyed ���� ����
    TArray<uint32> uuids;
    for (const auto& obj_uptr : CurrentWorld.GetAllObjects())
        uuids.push_back(obj_uptr->GetUUID());

    for (uint32 uuid : uuids)
        OnObjectDestroyed.Broadcast({ uuid });

    // 3. ObjectStore ��ü ��� + �޸� ����
    Objects.Clear();

    // 4. UUID ī���� �ʱ�ȭ
    UUIDs.SyncNextUUID(1);
}
