#include "AppContext.h"

bool AppContext::Initialize(const FString& windowTitle, int32 width, int32 height)
{
    return false;
}

void AppContext::SubscribeEvents()
{
    //OutlinerPanel* outliner = (OutlinerPanel*)Panels.Find("OutLiner");
}

// �� Shutdown() ���� ����:
//   1. �г�/�ý����� DelegateHandle Unbind (use-after-free ����)
//   2. EditorManager (�г� �Ҹ�)
//   3. D3D11Renderer (GPU ���ҽ� ����)
//   4. ObjectStore::Clear() (UObject �޸� ����)
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
