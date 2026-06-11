#include "GameFramework/Pawn/CombatCharacter.h"

#include "Component/Gameplay/CombatCoverAgentComponent.h"
#include "Component/Gameplay/SniperDamageReceiverComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/Script/LuaScriptComponent.h"
#include "Component/Shape/CapsuleComponent.h"
#include "Component/SoundComponent.h"
#include "Core/Types/CollisionTypes.h"

namespace
{
	const FString CombatGunfireSoundPath = "SFX/CombatAI/npc_gun_fire.mp3";
	constexpr float CombatGunfireVolume = 0.5f;
	constexpr float CombatGunfireMinDistance = 1.0f;
	constexpr float CombatGunfireMaxDistance = 80.0f;
}

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

	if (!CombatGunfireSoundComponent)
	{
		CombatGunfireSoundComponent = GetComponentByClass<USoundComponent>();
	}
	if (!CombatGunfireSoundComponent)
	{
		CombatGunfireSoundComponent = AddComponent<USoundComponent>();
	}
	ConfigureCombatGunfireSound(CombatGunfireSoundComponent.Get());

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
	CombatGunfireSoundComponent = AddComponent<USoundComponent>();
	ConfigureCombatGunfireSound(CombatGunfireSoundComponent.Get());
	SniperDamageReceiverComponent = AddComponent<USniperDamageReceiverComponent>();
}

void ACombatCharacter::PostDuplicate()
{
	Super::PostDuplicate();
	LuaScriptComponent = GetComponentByClass<ULuaScriptComponent>();
	CombatCoverAgentComponent = GetComponentByClass<UCombatCoverAgentComponent>();
	CombatGunfireSoundComponent = GetComponentByClass<USoundComponent>();
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

USoundComponent* ACombatCharacter::GetCombatGunfireSoundComponent() const
{
	if (!CombatGunfireSoundComponent)
	{
		CombatGunfireSoundComponent = GetComponentByClass<USoundComponent>();
	}
	ConfigureCombatGunfireSound(CombatGunfireSoundComponent.Get());
	return CombatGunfireSoundComponent.Get();
}

USniperDamageReceiverComponent* ACombatCharacter::GetSniperDamageReceiverComponent() const
{
	if (!SniperDamageReceiverComponent)
	{
		SniperDamageReceiverComponent = GetComponentByClass<USniperDamageReceiverComponent>();
	}
	return SniperDamageReceiverComponent;
}

void ACombatCharacter::ConfigureCombatGunfireSound(USoundComponent* Sound) const
{
	if (!Sound)
	{
		return;
	}

	Sound->SetSoundPath(CombatGunfireSoundPath);
	Sound->SetVolume(CombatGunfireVolume);
	Sound->SetPitch(1.0f);
	Sound->SetLooping(false);
	Sound->SetPlayOnBeginPlay(false);
	Sound->SetSpatialized(true);
	Sound->Set3DMinMaxDistance(CombatGunfireMinDistance, CombatGunfireMaxDistance);
	if (USceneComponent* Root = GetRootComponent())
	{
		Sound->SetParent(Root);
	}
	Sound->SetRelativeLocation(FVector::ZeroVector);
	Sound->SetRelativeRotation(FRotator::ZeroRotator);
	Sound->SetRelativeScale(FVector::OneVector);
}
