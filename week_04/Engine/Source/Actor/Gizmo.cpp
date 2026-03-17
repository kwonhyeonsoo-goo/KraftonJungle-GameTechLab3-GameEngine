#include "Gizmo.h"

#include "Component/GizmoComponent.h"
#include "Object/Class.h"

namespace 
{
	UObject* CreateAGizmoInstance(UObject* InOuter, const FString& InName)
	{
		return new AGizmo();
	}
}

AGizmo::AGizmo() : AActor(StaticClass(), "")
{
}

UClass* AGizmo::StaticClass()
{
	static UClass ClassInfo("AGizmo", AActor::StaticClass(), &CreateAGizmoInstance);
	return &ClassInfo;
}

void AGizmo::PostSpawnInitialize()
{
	UPrimitiveComponent* GizmoComp = new UGizmoComponent();
	this->AddOwnedComponent(GizmoComp);
}

void AGizmo::BeginPlay()
{
	AActor::BeginPlay();
}

void AGizmo::Tick(float DeltaTime)
{
	AActor::Tick(DeltaTime);
}

void AGizmo::EndPlay()
{
	AActor::EndPlay();
}

void AGizmo::Destroy()
{
	AActor::Destroy();
}
