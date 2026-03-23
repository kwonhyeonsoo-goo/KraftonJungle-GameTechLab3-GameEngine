#pragma once
#include "World/SceneComponent.h"

class ULightComponent : public USceneComponent {
private:

public:
	DECLARE_CLASS(ULightComponent, USceneComponent)
	ULightComponent() = default;
	~ULightComponent() = default;
};