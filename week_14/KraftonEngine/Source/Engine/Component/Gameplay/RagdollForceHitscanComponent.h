#pragma once

#include "Component/ActorComponent.h"
#include "Core/Types/CollisionTypes.h"
#include "Math/Vector.h"
#include "Object/Ptr/WeakObjectPtr.h"
#include "Component/Primitive/SkeletalMeshComponent.h"

#include "Source/Engine/Component/Gameplay/RagdollForceHitscanComponent.generated.h"

class AActor;
class APawn;
class UCameraComponent;
class URagdollForceEmitterComponent;
class UWorld;
struct FHitResult;
enum class EPartialRagdollPreset : uint8;

UCLASS()
class URagdollForceHitscanComponent : public UActorComponent
{
public:
    GENERATED_BODY()
    URagdollForceHitscanComponent();
    ~URagdollForceHitscanComponent() override = default;

    void BeginPlay() override;

protected:
    void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction) override;

public:
    bool FireImpulseHitscan();
    bool FireShockwaveHitscan();

private:
    URagdollForceEmitterComponent* ResolveForceEmitter() const;
    bool CanProcessHitscanInput() const;
    bool TryGetAimRay(FVector& OutOrigin, FVector& OutDirection) const;
    bool ExecuteHitscan(FHitResult& OutHit) const;
    AActor* ResolveHitActor(const FHitResult& Hit) const;
    void ProcessHitscanInput();
    void LogHitscanResult(const char* RequestKind, const FHitResult* Hit, bool bSucceeded) const;

private:
    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Enable Input Firing")
    bool bEnableInputFiring = true;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Impulse Fire Key")
    int32 ImpulseFireKey = 0;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Shockwave Fire Key")
    int32 ShockwaveFireKey = 0;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Trace Distance", Min=0.0f, Max=100000.0f, Speed=1.0f)
    float TraceDistance = 2000.0f;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Trace Channel", Enum=ECollisionChannel)
    ECollisionChannel TraceChannel = ECollisionChannel::WorldDynamic;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Impulse Strength", Min=0.0f, Max=3.0f, Speed=0.01f)
    float ImpulseStrength = 1.0f;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Shockwave Strength", Min=0.0f, Max=3.0f, Speed=0.01f)
    float ShockwaveStrength = 1.0f;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Shockwave Radius", Min=0.0f, Max=10000.0f, Speed=1.0f)
    float ShockwaveRadius = 300.0f;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Preferred Preset", Enum=EPartialRagdollPreset)
    EPartialRagdollPreset PreferredPreset = EPartialRagdollPreset::UpperBody;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Allow Escalation To Full Body")
    bool bAllowEscalationToFullBody = true;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Allow While Moving")
    bool bAllowWhileMoving = true;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Shockwave Origin Back Offset", Min=0.0f, Max=1000.0f, Speed=1.0f)
    float ShockwaveOriginBackOffset = 15.0f;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Use Camera Aim")
    bool bUseCameraAim = true;

    UPROPERTY(Edit, Save, Category="Physics|Ragdoll Hitscan", DisplayName="Log Hitscan")
    bool bLogHitscan = false;

    TWeakObjectPtr<URagdollForceEmitterComponent> CachedForceEmitter;
    TWeakObjectPtr<UCameraComponent> CachedAimCamera;
};
