#pragma once

#include <windows.h>
#include "../Foundation/Core/CoreTypes.h"

class WindowHost
{
public:
    bool   Initialize(const FString& title, int32 width, int32 height);
    void   Shutdown();
    bool   PollMessages();   // false = WM_QUIT

    HWND   GetHWND()   const;
    int32  GetWidth()  const;
    int32  GetHeight() const;
    void  UpdateSize(int32 width, int32 height); // ¡ç Ãß°¡

private:
    HWND  Hwnd = nullptr;
    int32 Width;
    int32 Height;
};

