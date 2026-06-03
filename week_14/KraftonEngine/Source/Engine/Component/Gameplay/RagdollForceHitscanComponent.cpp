#include "RagdollForceHitscanComponent.h"

#include "Component/Camera/CameraComponent.h"
#include "Component/Gameplay/RagdollForceEmitterComponent.h"
#include "Component/PrimitiveComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "GameFramework/GameMode/PlayerController.h"
#include "GameFramework/Pawn/Pawn.h"
#include "GameFramework/World.h"
#include "Runtime/Engine.h"
#include "Viewport/GameViewportClient.h"

#include <windows.h>

namespace
{
    constexpr int32 DefaultImpulseFireKey = VK_LBUTTON;
    constexpr int32 DefaultShockwaveFireKey = VK_RBUTTON;

    const char* GetNameSafe(const UObject* Object)
    {
        return Object ? Object->GetName().c_str() : "None";
    }
}

URagdollForceHitscanComponent::URagdollForceHitscanComponent()
{
    ImpulseFireKey = DefaultImpulseFireKey;
    ShockwaveFireKey = DefaultShockwaveFireKey;
}

void URagdollForceHitscanComponent::BeginPlay()
{
    UActorComponent::BeginPlay();
    CachedForceEmitter = ResolveForceEmitter();
    CachedAimCamera = GetOwner() ? GetOwner()->GetComponentByClass<UCameraComponent>() : nullptr;
    SetComponentTickEnabled(bEnableInputFiring);
}

void URagdollForceHitscanComponent::TickComponent(
    float DeltaTime,
    ELevelTick TickType,
    FActorComponentTickFunction& ThisTickFunction)
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
    (void)DeltaTime;
    (void)TickType;
    (void)ThisTickFunction;

    ProcessHitscanInput();
}

bool URagdollForceHitscanComponent::FireImpulseHitscan()
{
    URagdollForceEmitterComponent* ForceEmitter = ResolveForceEmitter();
    if (!ForceEmitter)
    {
        LogHitscanResult("Impulse", nullptr, false);
        return false;
    }

    FHitResult Hit;
    if (!ExecuteHitscan(Hit))
    {
        LogHitscanResult("Impulse", nullptr, false);
        return false;
    }

    AActor* TargetActor = ResolveHitActor(Hit);
    if (!TargetActor)
    {
        LogHitscanResult("Impulse", &Hit, false);
        return false;
    }

    FVector AimOrigin;
    FVector AimDirection;
    if (!TryGetAimRay(AimOrigin, AimDirection))
    {
        LogHitscanResult("Impulse", &Hit, false);
        return false;
    }

    const FVector ShotDirection =
        (Hit.WorldHitLocation - AimOrigin).IsNearlyZero()
            ? AimDirection
            : (Hit.WorldHitLocation - AimOrigin).Normalized();

    FRagdollImpulseRequest Request;
    Request.HitBoneName = FName::None;
    Request.WorldLocation = Hit.WorldHitLocation;
    Request.WorldDirection = ShotDirection;
    Request.Strength = ImpulseStrength;
    Request.HoldTimeOverride = -1.0f;
    Request.PreferredPreset = PreferredPreset;
    Request.bUsePreferredPreset = true;
    Request.bAllowEscalationToFullBody = bAllowEscalationToFullBody;
    Request.bAllowWhileMoving = bAllowWhileMoving;

    const bool bSucceeded = ForceEmitter->ApplyImpulseToActor(TargetActor, Request);
    LogHitscanResult("Impulse", &Hit, bSucceeded);
    return bSucceeded;
}

bool URagdollForceHitscanComponent::FireShockwaveHitscan()
{
    URagdollForceEmitterComponent* ForceEmitter = ResolveForceEmitter();
    if (!ForceEmitter)
    {
        LogHitscanResult("Shockwave", nullptr, false);
        return false;
    }

    FHitResult Hit;
    if (!ExecuteHitscan(Hit))
    {
        LogHitscanResult("Shockwave", nullptr, false);
        return false;
    }

    AActor* TargetActor = ResolveHitActor(Hit);
    if (!TargetActor)
    {
        LogHitscanResult("Shockwave", &Hit, false);
        return false;
    }

    FVector AimOrigin;
    FVector AimDirection;
    if (!TryGetAimRay(AimOrigin, AimDirection))
    {
        LogHitscanResult("Shockwave", &Hit, false);
        return false;
    }

    const FVector ShockwaveOrigin = Hit.WorldHitLocation - AimDirection * ShockwaveOriginBackOffset;
    FRagdollShockwaveRequest Request;
    Request.Origin = ShockwaveOrigin;
    Request.Radius = ShockwaveRadius;
    Request.Strength = ShockwaveStrength;
    Request.MinStrength = 0.0f;
    Request.HoldTimeOverride = -1.0f;
    Request.PreferredPreset = PreferredPreset;
    Request.bUsePreferredPreset = true;
    Request.bAllowEscalationToFullBody = bAllowEscalationToFullBody;
    Request.bAllowWhileMoving = bAllowWhileMoving;

    const bool bSucceeded = ForceEmitter->ApplyShockwaveToActor(TargetActor, Request);
    LogHitscanResult("Shockwave", &Hit, bSucceeded);
    return bSucceeded;
}

URagdollForceEmitterComponent* URagdollForceHitscanComponent::ResolveForceEmitter() const
{
    URagdollForceEmitterComponent* CachedEmitter = CachedForceEmitter.Get();
    if (CachedEmitter && CachedEmitter->GetOwner() == GetOwner())
    {
        return CachedEmitter;
    }

    return GetOwner() ? GetOwner()->GetComponentByClass<URagdollForceEmitterComponent>() : nullptr;
}

bool URagdollForceHitscanComponent::CanProcessHitscanInput() const
{
    if (!bEnableInputFiring || !GEngine)
    {
        return false;
    }

    UGameViewportClient* GameViewportClient = GEngine->GetGameViewportClient();
    UWorld* World = GetWorld();
    if (!GameViewportClient || !GameViewportClient->HasGameInputSnapshot() || !World)
    {
        return false;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    APawn* PossessedPawn = PlayerController ? PlayerController->GetPossessedPawn() : nullptr;
    return PossessedPawn && PossessedPawn == GetOwner();
}

bool URagdollForceHitscanComponent::TryGetAimRay(FVector& OutOrigin, FVector& OutDirection) const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return false;
    }

    UCameraComponent* AimCamera = bUseCameraAim
        ? (CachedAimCamera.Get() ? CachedAimCamera.Get() : OwnerActor->GetComponentByClass<UCameraComponent>())
        : nullptr;

    if (AimCamera)
    {
        OutOrigin = AimCamera->GetWorldLocation();
        OutDirection = AimCamera->GetWorldRotation().GetForwardVector();
        return true;
    }

    OutOrigin = OwnerActor->GetActorLocation();
    OutDirection = OwnerActor->GetActorForward();
    return !OutDirection.IsNearlyZero();
}

bool URagdollForceHitscanComponent::ExecuteHitscan(FHitResult& OutHit) const
{
    UWorld* World = GetWorld();
    FVector Origin;
    FVector Direction;
    if (!World || !TryGetAimRay(Origin, Direction))
    {
        return false;
    }

    return World->PhysicsRaycast(Origin, Direction, TraceDistance, OutHit, TraceChannel, GetOwner());
}

AActor* URagdollForceHitscanComponent::ResolveHitActor(const FHitResult& Hit) const
{
    if (Hit.HitActor)
    {
        return Hit.HitActor;
    }

    if (Hit.HitComponent)
    {
        return Hit.HitComponent->GetOwner();
    }

    return nullptr;
}

void URagdollForceHitscanComponent::ProcessHitscanInput()
{
    if (!CanProcessHitscanInput())
    {
        return;
    }

    const FInputSystemSnapshot& Snapshot = GEngine->GetGameViewportClient()->GetGameInputSnapshot();
    if (Snapshot.WasPressed(ImpulseFireKey))
    {
        FireImpulseHitscan();
    }

    if (Snapshot.WasPressed(ShockwaveFireKey))
    {
        FireShockwaveHitscan();
    }
}

void URagdollForceHitscanComponent::LogHitscanResult(
    const char* RequestKind,
    const FHitResult* Hit,
    bool bSucceeded) const
{
    if (!bLogHitscan)
    {
        return;
    }

    UE_LOG("Ragdoll hitscan %s. Component=%s Owner=%s HitActor=%s HitLocation=(%.2f,%.2f,%.2f) Success=%s",
        RequestKind ? RequestKind : "Unknown",
        GetNameSafe(this),
        GetNameSafe(GetOwner()),
        Hit ? GetNameSafe(Hit->HitActor) : "None",
        Hit ? Hit->WorldHitLocation.X : 0.0f,
        Hit ? Hit->WorldHitLocation.Y : 0.0f,
        Hit ? Hit->WorldHitLocation.Z : 0.0f,
        bSucceeded ? "true" : "false");
}
