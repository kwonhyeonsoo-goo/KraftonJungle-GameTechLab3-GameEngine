#include "Gizmo.h"

#include "Component/GizmoComponent.h"
#include "Object/Class.h"
#include "Object/ObjectFactory.h"

IMPLEMENT_RTTI(AGizmo, AActor)

void AGizmo::PostSpawnInitialize()
{
	UPrimitiveComponent* GizmoComp = FObjectFactory::ConstructObject<UGizmoComponent>(this);
	static_cast<UGizmoComponent*>(GizmoComp)->Initialize();
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
