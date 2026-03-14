#include "AppContext.h"

bool AppContext::Initialize(const FString& windowTitle, int32 width, int32 height)
{
    if (!Window.Initialize(windowTitle, width, height))
        return false;

    Renderer.Create(Window.GetHWND());

    CurrentWorld.SetContext(this);

    RegisterBuiltinTypes();
    RegisterPanels();   // 패널 등록
    RegisterTools();

    SubscribeEvents();  //이벤트 등록

    return false;
}

void AppContext::RegisterBuiltinTypes()
{
}

void AppContext::RegisterPanels()
{
}

void AppContext::RegisterTools()
{
}

void AppContext::SubscribeEvents()
{
    //OutlinerPanel* outliner = (OutlinerPanel*)Panels.Find("OutLiner");
}

// ★ Shutdown() 해제 순서:
//   1. 패널/시스템의 DelegateHandle Unbind (use-after-free 방지)
//   2. EditorManager (패널 소멸)
//   3. D3D11Renderer (GPU 리소스 해제)
//   4. ObjectStore::Clear() (UObject 메모리 해제)
//   5. WindowHost::Shutdown()
void AppContext::Shutdown()
{
    //unbind
    //PlatformEvents::OnMouseDown.Unbind(MouseDownHandle);
    //PlatformEvents::OnMouseMove.Unbind(MouseMoveHandle);
    //PlatformEvents::OnMouseUp.Unbind(MouseUpHandle);
    //PlatformEvents::OnKeyDown.Unbind(KeyDownHandle);
    //PlatformEvents::OnResize.Unbind(ResizeHandle);

}

// Always receive new Instance -> must delete.
void AppContext::Dispatch(ICommand* cmd)
{
    cmd->Execute();
    delete cmd;
}
