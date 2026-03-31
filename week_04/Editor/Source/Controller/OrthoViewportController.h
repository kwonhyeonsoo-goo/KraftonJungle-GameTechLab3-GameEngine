#pragma once
#include "../Source/Controller/EditorViewportController.h"

class FOrthoViewportController : public FViewportController
{

public:
	virtual ~FOrthoViewportController() = default;
	virtual void ProcessCameraInput(float DeltaTime) override;
	virtual void MouseProcess(float DeltaTime) override;

private:


};