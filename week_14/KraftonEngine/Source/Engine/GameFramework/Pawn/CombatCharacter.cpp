#include "GameFramework/Pawn/CombatCharacter.h"

#include "Component/Gameplay/CombatCoverAgentComponent.h"
#include "Component/Gameplay/SniperDamageReceiverComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/Script/LuaScriptComponent.h"
#include "Component/Shape/CapsuleComponent.h"
#include "Core/Types/CollisionTypes.h"

ACombatCharacter::ACombatCharacter()
{
	bAutoInputWASD = false;
	bAutoInputMouseLook = false;
	bAutoPossessPlayer = false;
}

void ACombatCharacter::BeginPlay()
{
	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Capsule->SetCollisionObjectType(ECollisionChannel::Pawn);
		Capsule->SetKinematic(true);
	}

	if (!SniperDamageReceiverComponent)
	{
		SniperDamageReceiverComponent = GetComponentByClass<USniperDamageReceiverComponent>();
	}
	if (!SniperDamageReceiverComponent)
	{
		SniperDamageReceiverComponent = AddComponent<USniperDamageReceiverComponent>();
	}

	Super::BeginPlay();

	if (bEnablePersistentQueryBodies)
	{
		if (USkeletalMeshComponent* Mesh = GetMesh())
		{
			Mesh->EnablePhysicsAssetQueryBodies();
		}
	}
}

void ACombatCharacter::InitDefaultComponents(const FString& SkeletalMeshFileName, const FString& ScriptFile)
{
	Super::InitDefaultComponents(SkeletalMeshFileName);

	LuaScriptComponent = AddComponent<ULuaScriptComponent>();
	if (!ScriptFile.empty())
	{
		LuaScriptComponent->SetScriptFile(ScriptFile);
	}

	CombatCoverAgentComponent = AddComponent<UCombatCoverAgentComponent>();
	SniperDamageReceiverComponent = AddComponent<USniperDamageReceiverComponent>();
}

void ACombatCharacter::PostDuplicate()
{
	Super::PostDuplicate();
	LuaScriptComponent = GetComponentByClass<ULuaScriptComponent>();
	CombatCoverAgentComponent = GetComponentByClass<UCombatCoverAgentComponent>();
	SniperDamageReceiverComponent = GetComponentByClass<USniperDamageReceiverComponent>();
}

ULuaScriptComponent* ACombatCharacter::GetLuaScriptComponent() const
{
	if (!LuaScriptComponent)
	{
		LuaScriptComponent = GetComponentByClass<ULuaScriptComponent>();
	}
	return LuaScriptComponent;
}

UCombatCoverAgentComponent* ACombatCharacter::GetCombatCoverAgentComponent() const
{
	if (!CombatCoverAgentComponent)
	{
		CombatCoverAgentComponent = GetComponentByClass<UCombatCoverAgentComponent>();
	}
	return CombatCoverAgentComponent;
}

USniperDamageReceiverComponent* ACombatCharacter::GetSniperDamageReceiverComponent() const
{
	if (!SniperDamageReceiverComponent)
	{
		SniperDamageReceiverComponent = GetComponentByClass<USniperDamageReceiverComponent>();
	}
	return SniperDamageReceiverComponent;
}
