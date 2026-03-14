#pragma once
#include "RenderTypes.h"

class RenderQueue {
public:
    void Push(const RenderCommand& cmd);
    void Clear();

    //const TArray<RenderCommand>& GetCommands() const;
    bool IsEmpty() const;
};