#include "PhysicsAssetRagdollTestComponent.h"

#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "GameFramework/Pawn/Character.h"
#include "Input/InputSystem.h"
#include "Physics/PhysicsAsset.h"

#include <windows.h>

namespace
{
    constexpr int32 DefaultEnableRagdollKey = VK_F6;
    constexpr int32 DefaultDisableRagdollKey = VK_F7;
    constexpr int32 DefaultRecoveryKey = VK_F9;
    constexpr int32 DefaultDumpStateKey = VK_F10;

    const char* GetAssetNameSafe(UPhysicsAsset* PhysicsAsset)
    {
        return PhysicsAsset ? PhysicsAsset->GetName().c_str() : "None";
    }
}

UPhysicsAssetRagdollTestComponent::UPhysicsAssetRagdollTestComponent()
{
    EnableRagdollKey = DefaultEnableRagdollKey;
    DisableRagdollKey = DefaultDisableRagdollKey;
    BeginRecoveryKey = DefaultRecoveryKey;
    DumpStateKey = DefaultDumpStateKey;
}

void UPhysicsAssetRagdollTestComponent::BeginPlay()
{
    UActorComponent::BeginPlay();

    ResolveTargetMeshComponent();
    LogControls();
    LogResolvedTarget(true);
}

void UPhysicsAssetRagdollTestComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
    (void)TickType;
    (void)ThisTickFunction;

    ResolveTargetMeshComponent();
    ProcessDebugInput();
    ProcessPeriodicActiveLogging(DeltaTime);
    MonitorStateTransitions();
}

void UPhysicsAssetRagdollTestComponent::ResolveTargetMeshComponent()
{
    USkeletalMeshComponent* CurrentTarget = TargetMeshComponent.Get();
    if (CurrentTarget && CurrentTarget->GetOwner() == GetOwner())
    {
        return;
    }

    TargetMeshComponent = FindTargetMeshComponent();
    ActiveLogTimer = 0.0f;

    if (TargetMeshComponent.Get())
    {
        bLoggedMissingTarget = false;
        bWasRagdollActive = TargetMeshComponent->IsRagdollActive();
        bWasRecovering = TargetMeshComponent->IsRecoveringFromRagdoll();
        LogResolvedTarget();
    }
    else if (!bLoggedMissingTarget)
    {
        LogResolvedTarget(true);
        bLoggedMissingTarget = true;
    }
}

USkeletalMeshComponent* UPhysicsAssetRagdollTestComponent::FindTargetMeshComponent() const
{
    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return nullptr;
    }

    if (TargetMeshComponentName.empty())
    {
        return OwnerActor->GetComponentByClass<USkeletalMeshComponent>();
    }

    const TArray<UActorComponent*> Components = OwnerActor->GetComponents();
    for (UActorComponent* Component : Components)
    {
        USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(Component);
        if (!SkeletalMeshComponent)
        {
            continue;
        }

        if (SkeletalMeshComponent->GetName() == TargetMeshComponentName)
        {
            return SkeletalMeshComponent;
        }
    }

    return nullptr;
}

bool UPhysicsAssetRagdollTestComponent::CanProcessDebugInput() const
{
    return InputSystem::Get().IsWindowFocused();
}

void UPhysicsAssetRagdollTestComponent::ProcessDebugInput()
{
    if (!CanProcessDebugInput())
    {
        return;
    }

    USkeletalMeshComponent* MeshComponent = TargetMeshComponent.Get();
    const InputSystem& Input = InputSystem::Get();

    if (Input.GetKeyDown(DumpStateKey))
    {
        LogCurrentState("ManualDump");
    }

    if (!MeshComponent)
    {
        return;
    }

    if (Input.GetKeyDown(EnableRagdollKey))
    {
        UE_LOG("[RagdollTest] EnableRequested Actor=%s MeshComponent=%s EffectivePhysicsAsset=%s",
            GetOwnerNameSafe(),
            GetComponentNameSafe(),
            GetAssetNameSafe(MeshComponent->GetEffectivePhysicsAsset()));

        const bool bUsesCharacterWrapper = Cast<ACharacter>(GetOwner()) != nullptr;
        const bool bEnabled = [this, MeshComponent]()
        {
            if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
            {
                return OwnerCharacter->EnterRagdoll();
            }
            return MeshComponent->EnableRagdollPhysics();
        }();
        const char* EventLabel = bEnabled
            ? (bUsesCharacterWrapper ? "EnableQueued" : "EnableSucceeded")
            : "EnableFailed";
        LogCurrentState(EventLabel);
        ActiveLogTimer = 0.0f;
    }

    if (Input.GetKeyDown(DisableRagdollKey))
    {
        if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
        {
            OwnerCharacter->ExitRagdoll();
        }
        else
        {
            MeshComponent->DisableRagdollPhysics();
        }
        LogCurrentState("DisableCompleted");
        ActiveLogTimer = 0.0f;
    }

    if (Input.GetKeyDown(BeginRecoveryKey))
    {
        UE_LOG("[RagdollTest] RecoveryRequested Actor=%s MeshComponent=%s",
            GetOwnerNameSafe(),
            GetComponentNameSafe());

        const bool bStarted = [this, MeshComponent]()
        {
            if (ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner()))
            {
                return OwnerCharacter->BeginRagdollRecovery();
            }
            return MeshComponent->BeginRagdollRecovery();
        }();
        LogCurrentState(bStarted ? "RecoveryStarted" : "RecoveryIgnored");
        ActiveLogTimer = 0.0f;
    }
}

void UPhysicsAssetRagdollTestComponent::ProcessPeriodicActiveLogging(float DeltaTime)
{
    if (!bLogWhileRagdollActive)
    {
        return;
    }

    USkeletalMeshComponent* MeshComponent = TargetMeshComponent.Get();
    if (!MeshComponent || !MeshComponent->IsRagdollActive())
    {
        ActiveLogTimer = 0.0f;
        return;
    }

    const float SafeInterval = ActiveLogInterval > 0.1f ? ActiveLogInterval : 0.1f;
    ActiveLogTimer += DeltaTime;
    if (ActiveLogTimer < SafeInterval)
    {
        return;
    }

    ActiveLogTimer = 0.0f;
    LogCurrentState("ActiveTick");
}

void UPhysicsAssetRagdollTestComponent::MonitorStateTransitions()
{
    USkeletalMeshComponent* MeshComponent = TargetMeshComponent.Get();
    const bool bIsRagdollActive = MeshComponent && MeshComponent->IsRagdollActive();
    const bool bIsRecovering = MeshComponent && MeshComponent->IsRecoveringFromRagdoll();

    if (!bWasRagdollActive && bIsRagdollActive)
    {
        LogCurrentState("RagdollActive");
    }
    else if (bWasRecovering && !bIsRecovering)
    {
        LogCurrentState("RecoveryCompleted");
    }
    else if (bWasRagdollActive && !bIsRagdollActive && !bIsRecovering)
    {
        LogCurrentState("RagdollInactive");
    }

    bWasRagdollActive = bIsRagdollActive;
    bWasRecovering = bIsRecovering;
}

void UPhysicsAssetRagdollTestComponent::LogControls() const
{
    UE_LOG("[RagdollTest] Controls: F6=Enable Ragdoll, F7=Disable Ragdoll, F9=Begin Recovery, F10=Dump State. Actor=%s RequestedMesh=%s",
        GetOwnerNameSafe(),
        TargetMeshComponentName.empty() ? "<First SkeletalMeshComponent>" : TargetMeshComponentName.c_str());
}

void UPhysicsAssetRagdollTestComponent::LogResolvedTarget(bool bForceLogFailure)
{
    if (USkeletalMeshComponent* MeshComponent = TargetMeshComponent.Get())
    {
        UE_LOG("[RagdollTest] Resolved target mesh component. Actor=%s MeshComponent=%s",
            GetOwnerNameSafe(),
            MeshComponent->GetName().c_str());
        return;
    }

    if (bForceLogFailure)
    {
        UE_LOG("[RagdollTest] No valid target skeletal mesh component found. Actor=%s RequestedMesh=%s",
            GetOwnerNameSafe(),
            TargetMeshComponentName.empty() ? "<First SkeletalMeshComponent>" : TargetMeshComponentName.c_str());
    }
}

void UPhysicsAssetRagdollTestComponent::LogCurrentState(const char* EventLabel) const
{
    USkeletalMeshComponent* MeshComponent = TargetMeshComponent.Get();
    UPhysicsAsset* EffectivePhysicsAsset = MeshComponent ? MeshComponent->GetEffectivePhysicsAsset() : nullptr;
    UPhysicsAsset* ActivePhysicsAsset = MeshComponent ? MeshComponent->GetActivePhysicsAsset() : nullptr;

    UE_LOG("[RagdollTest] %s Actor=%s MeshComponent=%s EffectivePhysicsAsset=%s ActivePhysicsAsset=%s Active=%s PhysicsPose=%s Recovering=%s LiveBodies=%d LiveConstraints=%d BlendWeight=%.2f",
        EventLabel ? EventLabel : "State",
        GetOwnerNameSafe(),
        GetComponentNameSafe(),
        GetAssetNameSafe(EffectivePhysicsAsset),
        GetAssetNameSafe(ActivePhysicsAsset),
        (MeshComponent && MeshComponent->IsRagdollActive()) ? "true" : "false",
        (MeshComponent && MeshComponent->IsUsingPhysicsAssetPose()) ? "true" : "false",
        (MeshComponent && MeshComponent->IsRecoveringFromRagdoll()) ? "true" : "false",
        MeshComponent ? MeshComponent->GetLiveRagdollBodyCount() : 0,
        MeshComponent ? MeshComponent->GetLiveRagdollConstraintCount() : 0,
        MeshComponent ? MeshComponent->GetPhysicsAssetBlendWeight() : 0.0f);
}

const char* UPhysicsAssetRagdollTestComponent::GetComponentNameSafe() const
{
    if (USkeletalMeshComponent* MeshComponent = TargetMeshComponent.Get())
    {
        return MeshComponent->GetName().c_str();
    }
    return "None";
}

const char* UPhysicsAssetRagdollTestComponent::GetOwnerNameSafe() const
{
    if (AActor* OwnerActor = GetOwner())
    {
        return OwnerActor->GetName().c_str();
    }
    return "None";
}
