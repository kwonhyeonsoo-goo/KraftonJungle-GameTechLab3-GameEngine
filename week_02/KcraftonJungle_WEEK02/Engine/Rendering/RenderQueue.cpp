#include "RenderQueue.h"

void RenderQueue::Push(const RenderCommand& cmd)
{
	Queue.push_back(cmd);
}

void RenderQueue::Clear()
{
	Queue.clear();
}

const TArray<RenderCommand>& RenderQueue::GetCommands() const
{
	return Queue;
}

bool RenderQueue::IsEmpty() const
{
	return Queue.empty();
}
