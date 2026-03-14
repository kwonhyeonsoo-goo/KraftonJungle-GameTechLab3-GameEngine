#include "RenderQueue.h"

void RenderQueue::Push(const RenderCommand& cmd)
{
	Queue->push_back(cmd);
}

void RenderQueue::Clear()
{
	Queue->clear();
}

const TArray<RenderCommand>& RenderQueue::GetCommands() const
{
	// TODO: 여기에 return 문을 삽입합니다.
	return *Queue;
}

bool RenderQueue::IsEmpty() const
{
	return Queue->empty();
}
