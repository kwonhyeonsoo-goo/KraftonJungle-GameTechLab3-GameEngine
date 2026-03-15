#pragma once
#include "Engine/Foundation/Core/CoreTypes.h"

#include "Engine/ObjectKernel/UUIDService.h"
#include "Engine/ObjectKernel/ClassRegistry.h"
#include "Engine/ObjectKernel/ObjectFactory.h"
#include "Engine/ObjectKernel/ObjectStore.h"

#include "Engine/World/World.h"
#include "Engine/Services/ConsoleService.h"

#include "Engine/Editor/EditorSession.h"
#include "Engine/Editor/EditorManager.h"
#include "Engine/Editor/Commands/ICommand.h"

#include "Engine/Rendering/URenderer.h"

#include "Engine/Platform/WindowHost.h"
#include "Engine/ObjectKernel/UUIDService.h"
#include "Engine/ObjectKernel/ClassRegistry.h"
#include "Engine/ObjectKernel/ObjectFactory.h"
#include "Engine/Editor/EditorSession.h"
#include "Engine/Editor/EditorManager.h"
#include "Engine/Editor/Events/EditorEvents.h"
#include "Engine/Editor/Commands/ICommand.h"
#include "Engine/Editor/Panels/PropertyPanel.h"
#include "Engine/Editor/Panels/ToolbarPanel.h"
#include "Engine/Editor/Panels/StatPanel.h"
#include "Engine/Editor/Panels/ConsolePanel.h"

#include "Engine/Foundation/Core/CoreTypes.h"

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
