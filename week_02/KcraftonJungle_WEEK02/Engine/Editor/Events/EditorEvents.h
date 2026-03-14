#pragma once
#include "../../Foundation/Core/CoreTypes.h"
#include "../../World/USceneComponent.h"

class EditorEvents
{
	struct ObjectDestroyedEvent { uint32 UUID; };
	struct SelectionChangedEvent { const USceneComponent* Primary; };
};

