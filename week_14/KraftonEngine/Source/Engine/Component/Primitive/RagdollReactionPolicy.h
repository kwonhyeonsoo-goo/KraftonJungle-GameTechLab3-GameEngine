#pragma once

#include "Math/Vector.h"
#include "Object/FName.h"

enum class ERagdollMode : uint8;
enum class EPartialRagdollPhase : uint8;
enum class ERagdollRecoveryPhase : uint8;
enum class EPartialRagdollPreset : uint8;
enum class ECharacterPhysicsOwnershipMode : uint8;
enum class ECharacterPhysicsCollisionMode : uint8;
enum class ECharacterQueryCollisionMode : uint8;
enum class ECharacterGameplayOverlapOwnershipMode : uint8;
enum class EPhysicsCollisionRole : uint8;

enum class ERagdollReactionEventKind : uint8
{
    DirectHit,
    RadialShockwave,
    ScriptedForce,
};

enum class ERagdollReactionType : uint8
{
    None,
    ImpulseOnly,
    Partial,
    FullBody,
    RefreshPartial,
    EscalateToFull,
    Deferred,
    RecoveryLimited,
};

enum class ERagdollReactionDecisionReason : uint8
{
    None,
    InvalidRequest,
    CouldNotResolvePartialSelection,
    OwnerMoving,
    RecoveryProtectedWindow,
    RecoveryImpulseOnly,
    RecoveryEscalatedToFull,
    FullBodyAlreadyActive,
    ExistingPartialRefreshed,
    ExistingPartialRejected,
    ExistingPartialEscalated,
    ShockwaveImpulseOnly,
    ShockwaveFullBody,
    FullBodyThreshold,
    PartialSelected,
    RedundantRequest,
};

struct FRagdollReactionTuning
{
    float DefaultStrengthWhenUnspecified = 0.5f;
    float BaseImpulseMagnitude = 120.0f;
    float AdditionalImpulseMagnitude = 240.0f;
    float HoldTimeBaseScale = 0.75f;
    float HoldTimeStrengthScale = 0.75f;
    float FullBodyThreshold = 0.85f;
    float EscalationThreshold = 0.80f;
    float ShockwaveFullThreshold = 0.90f;
    float ShockwaveImpulseThreshold = 0.45f;
    float RecoveryEscalationThreshold = 0.90f;
    float RecoveryImpulseThreshold = 0.55f;
};

struct FRagdollReactionRequest
{
    ERagdollReactionEventKind EventKind = ERagdollReactionEventKind::DirectHit;
    EPartialRagdollPreset PreferredPreset{};
    FName HitBoneName = FName::None;
    FVector HitWorldLocation = FVector::ZeroVector;
    FVector HitWorldDirection = FVector::ZeroVector;
    float Strength = 0.5f;
    float HoldTimeOverride = -1.0f;
    bool bUsePreferredPreset = false;
    bool bAllowEscalationToFullBody = false;
    bool bAllowWhileMoving = true;

    bool HasHitBone() const
    {
        return HitBoneName != FName::None;
    }

    bool HasHoldTimeOverride() const
    {
        return HoldTimeOverride >= 0.0f;
    }
};

struct FRagdollReactionContext
{
    ERagdollMode CurrentMode{};
    EPartialRagdollPhase CurrentPartialPhase{};
    ERagdollRecoveryPhase CurrentRecoveryPhase{};
    ECharacterPhysicsOwnershipMode CharacterOwnershipMode{};
    ECharacterPhysicsCollisionMode CharacterPhysicsCollisionMode{};
    ECharacterQueryCollisionMode CharacterQueryCollisionMode{};
    ECharacterGameplayOverlapOwnershipMode CharacterGameplayOverlapMode{};
    EPhysicsCollisionRole CapsuleRole{};
    EPhysicsCollisionRole MeshRole{};
    EPhysicsCollisionRole PartialBodyRole{};
    bool bHasLivePhysicsBodies = false;
    bool bIsRecovering = false;
    bool bOwnerMoving = false;
    bool bPendingPartialBlendOut = false;
    bool bPartialSelfSuppressionActive = false;
    bool bHasResolvedPartialSelection = false;
    EPartialRagdollPreset ResolvedPreset{};
    FName ResolvedRootBoneName = FName::None;
    bool bResolvedIncludeDescendants = true;
    bool bSamePartialSelectionActive = false;
    float DefaultPartialHoldTime = 0.0f;
    FName ActivePartialRootBoneName = FName::None;
};

struct FRagdollReactionDecision
{
    ERagdollReactionType Type = ERagdollReactionType::None;
    ERagdollReactionDecisionReason Reason = ERagdollReactionDecisionReason::None;
    EPartialRagdollPreset ResolvedPreset{};
    FName ResolvedRootBoneName = FName::None;
    bool bIncludeDescendants = true;
    float NormalizedStrength = 0.0f;
    float HoldTime = 0.0f;
    float ImpulseMagnitude = 0.0f;
    FVector ImpulseDirection = FVector::ZeroVector;
    bool bApplyImpulse = false;
    bool bEscalationCandidate = false;

    bool HasResolvedPartialSelection() const
    {
        return ResolvedRootBoneName != FName::None;
    }
};

float NormalizeRagdollReactionStrength(const FRagdollReactionRequest& Request, const FRagdollReactionTuning& Tuning);
FRagdollReactionDecision EvaluateRagdollReactionPolicy(
    const FRagdollReactionContext& Context,
    const FRagdollReactionRequest& Request,
    const FRagdollReactionTuning& Tuning);

const char* LexToString(ERagdollReactionEventKind EventKind);
const char* LexToString(ERagdollReactionType ReactionType);
const char* LexToString(ERagdollReactionDecisionReason Reason);
