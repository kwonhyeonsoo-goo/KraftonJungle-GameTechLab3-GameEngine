#pragma once
#include "../../Foundation/Core/CoreTypes.h"
#include "../../World/USceneComponent.h"

struct ObjectCreatedEvent { uint32 UUID; };
struct ObjectDestroyedEvent { uint32 UUID; };
struct SelectionChangedEvent { const USceneComponent* Primary; };