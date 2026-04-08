#pragma once

#include "ViewportClient.h"

class ENGINE_API FGameViewportClient : public FViewportClient
{
public:
	void Attach() override;
	void Detach() override;

	void ProcessCameraInput(float DeltaTime) override;

	void BuildRenderCommands(TArray<AActor*>& InActors, FRenderCommandQueue& OutQueue) override;
};
