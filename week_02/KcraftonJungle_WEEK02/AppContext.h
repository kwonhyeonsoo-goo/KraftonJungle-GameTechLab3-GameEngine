#pragma once
#include "Engine/Foundation/Core/CoreTypes.h"

#include "Engine/ObjectKernel/UUIDService.h"
#include "Engine/ObjectKernel/ClassRegistry.h"
#include "Engine/ObjectKernel/ObjectFactory.h"
#include "Engine/ObjectKernel/ObjectStore.h"

#include "Engine/World/World.h"

#include "Engine/Editor/EditorSession.h"
#include "Engine/Editor/EditorManager.h"
#include "Engine/Editor/Commands/ICommand.h"

#include "Engine/Rendering/URenderer.h"

#include "Engine/Editor/Events/EditorEvents.h"
#include "Engine/Editor/Panels/PropertyPanel.h"
#include "Engine/Editor/Panels/ToolbarPanel.h"
#include "Engine/Editor/Panels/StatPanel.h"
#include "Engine/Editor/Panels/ConsolePanel.h"
#include "Engine/Editor/Tools/ITool.h"

#include "Engine/Platform/WindowHost.h"

#include "Engine/Services/ConsoleService.h"

// main() 

struct AppContext {
    // Object Kernel
    UUIDService    UUIDs;
    ClassRegistry  Classes;
    ObjectStore    Objects;
    ObjectFactory  Factory;

    // World
    World         CurrentWorld;

    // Services
    ConsoleService Console;

    // Editor
    EditorSession  Editor;
    EditorManager  Panels;

    // Rendering
    URenderer  Renderer;

    // Platform
    WindowHost Window;

    // ���� �ʱ�ȭ ����
    bool Initialize(const FString& windowTitle = "MyEngine",
        int32 width = 1280,
        int32 height = 720);

    void Shutdown();
    void NewScene();

    void Dispatch(ICommand* cmd);

    TDelegate<ObjectDestroyedEvent> OnObjectDestroyed;

private:
    TArray<IEditorPanel*> OwnedPanels;
    TArray<ITool*> OwnedTools;

    // 선택은 항상 가능한 기본 동작
    ITool* SelectionTool = nullptr;

    // 변환 모드 툴
    ITool* Translate = nullptr;
    //RotateTool* Rotate = nullptr;
    //ScaleTool* Scale = nullptr

    // 현재 드래그를 점유한 툴
    ITool* CapturedManipulationTool = nullptr;

    void RegisterBuiltinTypes();   // Cube, Sphere, Plane ClassRegistry ���
    void RegisterPanels();
    void RegisterTools();
    void SubscribeEvents();        // PlatformEvents �� Editor/Renderer ����

    DelegateHandle MouseDownHandle = 0;
    DelegateHandle MouseMoveHandle = 0;
    DelegateHandle MouseUpHandle = 0;
    DelegateHandle KeyDownHandle = 0;
    DelegateHandle KeyUpHandle = 0;
    DelegateHandle ResizeHandle = 0;
};
