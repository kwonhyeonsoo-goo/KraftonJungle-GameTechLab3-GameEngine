#pragma once
#include "RenderTypes.h"
#include "../Foundation/Containers/TArray.h"

class RenderQueue {
public:
    void Push(const RenderCommand& cmd);
    void Clear();

    const TArray<RenderCommand>& GetCommands() const;
    bool IsEmpty() const;

private:
	TArray<RenderCommand> Queue;
};