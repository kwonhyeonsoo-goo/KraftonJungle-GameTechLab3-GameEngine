#pragma once

#include "GameFramework/Pawn/Character.h"
#include "Object/Ptr/WeakObjectPtr.h"

class UCombatCoverAgentComponent;
class ULuaScriptComponent;

#include "Source/Engine/GameFramework/Pawn/CombatCharacter.generated.h"

UCLASS()
class ACombatCharacter : public ACharacter
{
public:
	GENERATED_BODY()

	ACombatCharacter();
	~ACombatCharacter() override = default;

	void InitDefaultComponents(const FString& SkeletalMeshFileName, const FString& ScriptFile);

	void InitDefaultComponents(const FString& SkeletalMeshFileName) override
	{
		InitDefaultComponents(SkeletalMeshFileName, FString());
	}

	void PostDuplicate() override;

	ULuaScriptComponent* GetLuaScriptComponent() const;
	UCombatCoverAgentComponent* GetCombatCoverAgentComponent() const;

protected:
	mutable TWeakObjectPtr<ULuaScriptComponent> LuaScriptComponent = nullptr;
	mutable TWeakObjectPtr<UCombatCoverAgentComponent> CombatCoverAgentComponent = nullptr;
};
