#pragma once
#include "Engine/World/World.h"
#include "Engine/Rendering/URenderer.h"
#include "Engine/ObjectKernel/ObjectStore.h"
#include "Engine/Editor/Commands/ICommand.h"
#include "Engine/Editor/Events/EditorEvents.h"
#include "Engine/Foundation/Core/CoreTypes.h"
#include "Engine/ObjectKernel/UUIDService.h"
#include "Engine/ObjectKernel/ClassRegistry.h"
#include "Engine/ObjectKernel/ObjectFactory.h"

// main() 스택에서 생성
struct AppContext {
    // Object Kernel
    UUIDService    UUIDs;
    ClassRegistry  Classes;
    ObjectStore    Objects;
    ObjectFactory  Factory;

    // World
    World         CurrentScene;

    // Services
    //ConsoleService Console;

    // Editor
    //EditorSession  Editor;
    //EditorManager  Panels;

    // Rendering
    URenderer  Renderer;

    // Platform
    //WindowHost     Window;

    // ── 초기화 ──
    bool Initialize(const FString& windowTitle = "MyEngine",
        int32 width = 1280,
        int32 height = 720);

    void Shutdown();

    void Dispatch(ICommand* cmd);

    TDelegate<ObjectDestroyedEvent> OnObjectDestroyed;

private:
    void RegisterBuiltinTypes();   // Cube, Sphere, Plane ClassRegistry 등록
    void RegisterPanels();
    void RegisterTools();
    void SubscribeEvents();        // PlatformEvents → Editor/Renderer 연결
};
