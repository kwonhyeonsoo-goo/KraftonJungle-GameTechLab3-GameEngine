#include "AppContext.h"

bool AppContext::Initialize(const FString& windowTitle, int32 width, int32 height)
{
    if (!Window.Initialize(windowTitle, width, height))
        return false;

    Renderer.Create(Window.GetHWND());

    CurrentWorld.SetContext(this);

    RegisterBuiltinTypes();
    RegisterPanels();   // ÆÐ³Î µî·Ï
    RegisterTools();

    SubscribeEvents();  //ÀÌº¥Æ® µî·Ï

    return false;
}

void AppContext::RegisterBuiltinTypes()
{
}

void AppContext::RegisterPanels()
{
    Panels.Register(new PropertyPanel());
    Panels.Register(new ToolbarPanel());
    Panels.Register(new StatPanel());
    Panels.Register(new ConsolePanel());
}

void AppContext::RegisterTools()
{
}

void AppContext::SubscribeEvents()
{
    //OutlinerPanel* outliner = (OutlinerPanel*)Panels.Find("OutLiner");
}

// ï¿½ï¿½ Shutdown() ï¿½ï¿½ï¿½ï¿½ ï¿½ï¿½ï¿½ï¿½:
//   1. ï¿½Ð³ï¿½/ï¿½Ã½ï¿½ï¿½ï¿½ï¿½ï¿½ DelegateHandle Unbind (use-after-free ï¿½ï¿½ï¿½ï¿½)
//   2. EditorManager (ï¿½Ð³ï¿½ ï¿½Ò¸ï¿½)
//   3. D3D11Renderer (GPU ï¿½ï¿½ï¿½Ò½ï¿½ ï¿½ï¿½ï¿½ï¿½)
//   4. ObjectStore::Clear() (UObject ï¿½Þ¸ï¿½ ï¿½ï¿½ï¿½ï¿½)
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
