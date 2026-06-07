#include "GameFramework/Pawn/CombatCharacter.h"

#include "Component/Gameplay/CombatCoverAgentComponent.h"
#include "Component/Script/LuaScriptComponent.h"

ACombatCharacter::ACombatCharacter()
{
	bAutoInputWASD = false;
	bAutoInputMouseLook = false;
	bAutoPossessPlayer = false;
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
}

void ACombatCharacter::PostDuplicate()
{
	Super::PostDuplicate();
	LuaScriptComponent = GetComponentByClass<ULuaScriptComponent>();
	CombatCoverAgentComponent = GetComponentByClass<UCombatCoverAgentComponent>();
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
