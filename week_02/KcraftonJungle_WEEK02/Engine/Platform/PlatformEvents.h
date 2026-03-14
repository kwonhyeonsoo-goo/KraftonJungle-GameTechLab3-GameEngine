#pragma once
#include"InputRouter.h"

class PlatformEvents
{
    static TDelegate<MouseMoveEvent> OnMouseMove;
    static TDelegate<MouseDownEvent> OnMouseDown;
    static TDelegate<MouseUpEvent>   OnMouseUp;
    static TDelegate<KeyDownEvent>   OnKeyDown;
    static TDelegate<KeyUpEvent>     OnKeyUp;
    static TDelegate<ResizeEvent>    OnResize;
public:
    static void Poll();
};

