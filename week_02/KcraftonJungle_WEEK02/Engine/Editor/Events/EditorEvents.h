#pragma once
#include "../../Foundation/Core/CoreTypes.h"
#include "../../World/USceneComponent.h"

struct ObjectCreatedEvent { uint32 UUID; };
struct ObjectDestroyedEvent { uint32 UUID; };
struct SelectionChangedEvent { const TArray<USceneComponent*> Primitives; };
//객체 + bool형식으로 바꿀것 : bool -> 오브젝트가 선택된 상태인지 여부, 
// 객체는 선택된 객체의 포인터