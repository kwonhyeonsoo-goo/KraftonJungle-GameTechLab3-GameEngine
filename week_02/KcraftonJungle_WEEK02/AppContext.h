#pragma once
#include "Engine/World/World.h"
#include "Engine/Rendering/URenderer.h"
#include "Engine/ObjectKernel/ObjectStore.h"
#include "Engine/ObjectKernel/UUIDService.h"


// main() ���ÿ��� ����
class TDelegate;
class ObjectStore;

struct AppContext {

    //임시!

    //// Object Kernel
    UUIDService    UUIDs;
    //ClassRegistry  Classes;
    ObjectStore    Objects;
    //ObjectFactory  Factory;

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

    // ���� �ʱ�ȭ ����
    bool Initialize(const FString& windowTitle = "MyEngine",
        int32 width = 1280,
        int32 height = 720);

    void Shutdown();

    //void Dispatch(ICommand* cmd);

    //Delegate<ObjectDestroyedEvent> OnObjectDestroyed;

private:
    void RegisterBuiltinTypes();   // Cube, Sphere, Plane ClassRegistry ���
    void RegisterPanels();
    void RegisterTools();
    void SubscribeEvents();        // PlatformEvents �� Editor/Renderer ����
};
