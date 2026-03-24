#pragma once
#include "PrimitiveComponent.h"
#include "Types/String.h"

class ENGINE_API UGizmoComponent : public UPrimitiveComponent
{
public:
	DECLARE_RTTI(UGizmoComponent, UPrimitiveComponent)

	const FString GizmoMaterialName = "M_Gizmos";

	void Initialize();
};

