#include "Gizmo.h"

#include "Component/GizmoComponent.h"
#include "Object/Class.h"
#include "Object/ObjectFactory.h"

namespace 
{
	UObject* CreateAGizmoInstance(UObject* InOuter, const FString& InName)
	{
		return new AGizmo(AGizmo::StaticClass(), InName, InOuter);
	}
}

AGizmo::AGizmo() : AActor(StaticClass(), "")
{
}

AGizmo::AGizmo(UClass* InClass, const FString& InName, UObject* InOuter)
	: AActor(InClass, InName, InOuter)
{
}

UClass* AGizmo::StaticClass()
{
	static UClass ClassInfo("AGizmo", AActor::StaticClass(), &CreateAGizmoInstance);
	return &ClassInfo;
}

void AGizmo::PostSpawnInitialize()
{
	UPrimitiveComponent* GizmoComp = FObjectFactory::ConstructObject<UGizmoComponent>(this);
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
