#pragma once
#include "SceneComponent.h"
#include "Primitive/PrimitiveBase.h"
#include "Math/Frustum.h"
#include <memory>
#include <algorithm>
#include <cmath>

class FMaterial;

struct FBoxSphereBounds
{
	FVector Center;
	float Radius;
	FVector BoxExtent;
};

class ENGINE_API UPrimitiveComponent : public USceneComponent
{
public:
	DECLARE_RTTI(UPrimitiveComponent, USceneComponent)

	~UPrimitiveComponent();

	CPrimitiveBase* GetPrimitive() const { return Primitive.get(); }

	void SetMaterial(FMaterial* InMaterial) { Material = InMaterial; }
	FMaterial* GetMaterial() const { return Material; }

	virtual FBoxSphereBounds GetWorldBounds() const;

	void UpdateLocalBound();

	FString GetPrimitiveFileName() const 
	{ 
		if (Primitive) return Primitive->GetPrimitiveFileName();
		else return ""; 
	}

	void OnRegister() override;
	void OnUnregister() override;
	/** Mesh 별로 다른 Proxy 를 생성할 수 있도록 virtual 로 선언 */
	virtual FMeshData* CreateProxy() const;

	FMeshData* GetSceneProxy() const { return SceneProxy; }

protected:
	std::shared_ptr<CPrimitiveBase> Primitive;
	FMaterial* Material = nullptr;
	bool bDrawDebugBounds = true;

	// UpdateRenderState 에서 dirty 상태일 때 World 에 등록된 자신의 정보 수정 용도의 포인터
	FMeshData* SceneProxy = nullptr;
	// 어떤 상태를 dirty 로 기록할지는 좀 생각 필요

public:
	bool ShouldDrawDebugBounds() const { return bDrawDebugBounds; }
	void SetDrawDebugBounds(bool bEnable) { bDrawDebugBounds = bEnable; }
};
