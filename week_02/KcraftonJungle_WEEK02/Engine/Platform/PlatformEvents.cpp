#include "PlatformEvents.h"
#include "../../Editor/ImGui/imgui.h" // ImGui는 cpp에서만 include

TDelegate<MouseMoveEvent> PlatformEvents::OnMouseMove;
TDelegate<MouseDownEvent> PlatformEvents::OnMouseDown;
TDelegate<MouseUpEvent>   PlatformEvents::OnMouseUp;
TDelegate<KeyDownEvent>   PlatformEvents::OnKeyDown;
TDelegate<KeyUpEvent>     PlatformEvents::OnKeyUp;
TDelegate<ResizeEvent>    PlatformEvents::OnResize;

//NewFrame() 이후에 호출되어야함
void PlatformEvents::Poll()
{
	TArray<FInputEvent> events = InputRouter::ConsumePendingEvents();

	bool mouseCaptured = ImGui::GetIO().WantCaptureMouse;
	bool keyboardCaptured = ImGui::GetIO().WantCaptureKeyboard;

    for (auto& event : events) {
        switch (event.Type) {
        case FInputEvent::EType::MouseMove:
            if (!mouseCaptured)
                OnMouseMove.Broadcast(event.MouseMove);
            break;

        case FInputEvent::EType::MouseDown:
            if (!mouseCaptured)
                OnMouseDown.Broadcast(event.MouseDown);
            break;

        case FInputEvent::EType::MouseUp:
            if (!mouseCaptured)
                OnMouseUp.Broadcast(event.MouseUp);
            break;

        case FInputEvent::EType::KeyDown:
            if (!keyboardCaptured)
                OnKeyDown.Broadcast(event.KeyDown);
            break;

        case FInputEvent::EType::KeyUp:
            if (!keyboardCaptured)
                OnKeyUp.Broadcast(event.KeyUp);
            break;

        case FInputEvent::EType::Resize:
              OnResize.Broadcast(event.Resize); // 항상 발행
            break;
        }
    }
    InputRouter::ResetDelta();
}
