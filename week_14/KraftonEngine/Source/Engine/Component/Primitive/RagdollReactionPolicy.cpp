#include "Component/Primitive/RagdollReactionPolicy.h"

#include "Component/Primitive/SkeletalMeshComponent.h"

#include <algorithm>

namespace
{
    float Clamp01(float Value)
    {
        return std::clamp(Value, 0.0f, 1.0f);
    }

    FVector ResolveImpulseDirection(const FVector& Direction)
    {
        if (Direction.Dot(Direction) <= 1.0e-6f)
        {
            return FVector::ForwardVector;
        }

        return Direction.Normalized();
    }

    FRagdollReactionDecision BuildBaseDecision(
        const FRagdollReactionContext& Context,
        const FRagdollReactionRequest& Request,
        const FRagdollReactionTuning& Tuning)
    {
        FRagdollReactionDecision Decision;
        Decision.ResolvedPreset = Context.ResolvedPreset;
        Decision.ResolvedRootBoneName = Context.ResolvedRootBoneName;
        Decision.bIncludeDescendants = Context.bResolvedIncludeDescendants;
        Decision.NormalizedStrength = NormalizeRagdollReactionStrength(Request, Tuning);
        Decision.HoldTime = Request.HasHoldTimeOverride()
            ? Request.HoldTimeOverride
            : (Context.DefaultPartialHoldTime * (Tuning.HoldTimeBaseScale + Tuning.HoldTimeStrengthScale * Decision.NormalizedStrength));
        Decision.ImpulseMagnitude = Tuning.BaseImpulseMagnitude + Tuning.AdditionalImpulseMagnitude * Decision.NormalizedStrength;
        Decision.ImpulseDirection = ResolveImpulseDirection(Request.HitWorldDirection);
        Decision.bApplyImpulse = Decision.ImpulseMagnitude > 1.0e-3f;
        Decision.bEscalationCandidate =
            Request.bAllowEscalationToFullBody && Decision.NormalizedStrength >= Tuning.FullBodyThreshold;
        return Decision;
    }
}

float NormalizeRagdollReactionStrength(const FRagdollReactionRequest& Request, const FRagdollReactionTuning& Tuning)
{
    const float RawStrength =
        Request.Strength > 0.0f
            ? Request.Strength
            : Tuning.DefaultStrengthWhenUnspecified;
    return Clamp01(RawStrength);
}

FRagdollReactionDecision EvaluateRagdollReactionPolicy(
    const FRagdollReactionContext& Context,
    const FRagdollReactionRequest& Request,
    const FRagdollReactionTuning& Tuning)
{
    FRagdollReactionDecision Decision = BuildBaseDecision(Context, Request, Tuning);

    if (!Request.bAllowWhileMoving && Context.bOwnerMoving)
    {
        Decision.Type = ERagdollReactionType::None;
        Decision.Reason = ERagdollReactionDecisionReason::OwnerMoving;
        return Decision;
    }

    if (Context.bIsRecovering)
    {
        if (Request.bAllowEscalationToFullBody && Decision.NormalizedStrength >= Tuning.RecoveryEscalationThreshold)
        {
            Decision.Type = ERagdollReactionType::EscalateToFull;
            Decision.Reason = ERagdollReactionDecisionReason::RecoveryEscalatedToFull;
            return Decision;
        }

        if (Context.bHasLivePhysicsBodies && Decision.NormalizedStrength >= Tuning.RecoveryImpulseThreshold)
        {
            Decision.Type = ERagdollReactionType::RecoveryLimited;
            Decision.Reason = ERagdollReactionDecisionReason::RecoveryImpulseOnly;
            return Decision;
        }

        Decision.Type = ERagdollReactionType::None;
        Decision.Reason = ERagdollReactionDecisionReason::RecoveryProtectedWindow;
        return Decision;
    }

    if (Request.EventKind == ERagdollReactionEventKind::RadialShockwave)
    {
        if (Request.bAllowEscalationToFullBody && Decision.NormalizedStrength >= Tuning.ShockwaveFullThreshold)
        {
            Decision.Type =
                Context.CurrentMode == ERagdollMode::Partial
                    ? ERagdollReactionType::EscalateToFull
                    : ERagdollReactionType::FullBody;
            Decision.Reason = ERagdollReactionDecisionReason::ShockwaveFullBody;
            return Decision;
        }

        if (Context.bHasLivePhysicsBodies && Decision.NormalizedStrength >= Tuning.ShockwaveImpulseThreshold)
        {
            Decision.Type = ERagdollReactionType::ImpulseOnly;
            Decision.Reason = ERagdollReactionDecisionReason::ShockwaveImpulseOnly;
            return Decision;
        }

        if (!Context.bHasResolvedPartialSelection)
        {
            Decision.Type = ERagdollReactionType::None;
            Decision.Reason = ERagdollReactionDecisionReason::CouldNotResolvePartialSelection;
            return Decision;
        }

        Decision.Type =
            Context.bSamePartialSelectionActive
                ? ERagdollReactionType::RefreshPartial
                : ERagdollReactionType::Partial;
        Decision.Reason = ERagdollReactionDecisionReason::PartialSelected;
        return Decision;
    }

    if (Context.CurrentMode == ERagdollMode::FullBody)
    {
        Decision.Type =
            Context.bHasLivePhysicsBodies
                ? ERagdollReactionType::ImpulseOnly
                : ERagdollReactionType::None;
        Decision.Reason = ERagdollReactionDecisionReason::FullBodyAlreadyActive;
        return Decision;
    }

    if (!Context.bHasResolvedPartialSelection)
    {
        if (Decision.bEscalationCandidate)
        {
            Decision.Type = ERagdollReactionType::FullBody;
            Decision.Reason = ERagdollReactionDecisionReason::FullBodyThreshold;
            return Decision;
        }

        Decision.Type = ERagdollReactionType::None;
        Decision.Reason = ERagdollReactionDecisionReason::CouldNotResolvePartialSelection;
        return Decision;
    }

    if (Context.CurrentMode == ERagdollMode::Partial)
    {
        if (Context.bSamePartialSelectionActive)
        {
            if (Request.bAllowEscalationToFullBody && Decision.NormalizedStrength >= Tuning.EscalationThreshold)
            {
                Decision.Type = ERagdollReactionType::EscalateToFull;
                Decision.Reason = ERagdollReactionDecisionReason::ExistingPartialEscalated;
                return Decision;
            }

            Decision.Type = ERagdollReactionType::RefreshPartial;
            Decision.Reason = ERagdollReactionDecisionReason::ExistingPartialRefreshed;
            return Decision;
        }

        if (Request.bAllowEscalationToFullBody && Decision.NormalizedStrength >= Tuning.EscalationThreshold)
        {
            Decision.Type = ERagdollReactionType::EscalateToFull;
            Decision.Reason = ERagdollReactionDecisionReason::ExistingPartialEscalated;
            return Decision;
        }

        Decision.Type = ERagdollReactionType::None;
        Decision.Reason = ERagdollReactionDecisionReason::ExistingPartialRejected;
        return Decision;
    }

    if (Decision.bEscalationCandidate)
    {
        Decision.Type = ERagdollReactionType::FullBody;
        Decision.Reason = ERagdollReactionDecisionReason::FullBodyThreshold;
        return Decision;
    }

    Decision.Type = ERagdollReactionType::Partial;
    Decision.Reason = ERagdollReactionDecisionReason::PartialSelected;
    return Decision;
}

const char* LexToString(ERagdollReactionEventKind EventKind)
{
    switch (EventKind)
    {
    case ERagdollReactionEventKind::RadialShockwave:
        return "RadialShockwave";
    case ERagdollReactionEventKind::ScriptedForce:
        return "ScriptedForce";
    default:
        return "DirectHit";
    }
}

const char* LexToString(ERagdollReactionType ReactionType)
{
    switch (ReactionType)
    {
    case ERagdollReactionType::ImpulseOnly:
        return "ImpulseOnly";
    case ERagdollReactionType::Partial:
        return "Partial";
    case ERagdollReactionType::FullBody:
        return "FullBody";
    case ERagdollReactionType::RefreshPartial:
        return "RefreshPartial";
    case ERagdollReactionType::EscalateToFull:
        return "EscalateToFull";
    case ERagdollReactionType::Deferred:
        return "Deferred";
    case ERagdollReactionType::RecoveryLimited:
        return "RecoveryLimited";
    default:
        return "None";
    }
}

const char* LexToString(ERagdollReactionDecisionReason Reason)
{
    switch (Reason)
    {
    case ERagdollReactionDecisionReason::InvalidRequest:
        return "InvalidRequest";
    case ERagdollReactionDecisionReason::CouldNotResolvePartialSelection:
        return "CouldNotResolvePartialSelection";
    case ERagdollReactionDecisionReason::OwnerMoving:
        return "OwnerMoving";
    case ERagdollReactionDecisionReason::RecoveryProtectedWindow:
        return "RecoveryProtectedWindow";
    case ERagdollReactionDecisionReason::RecoveryImpulseOnly:
        return "RecoveryImpulseOnly";
    case ERagdollReactionDecisionReason::RecoveryEscalatedToFull:
        return "RecoveryEscalatedToFull";
    case ERagdollReactionDecisionReason::FullBodyAlreadyActive:
        return "FullBodyAlreadyActive";
    case ERagdollReactionDecisionReason::ExistingPartialRefreshed:
        return "ExistingPartialRefreshed";
    case ERagdollReactionDecisionReason::ExistingPartialRejected:
        return "ExistingPartialRejected";
    case ERagdollReactionDecisionReason::ExistingPartialEscalated:
        return "ExistingPartialEscalated";
    case ERagdollReactionDecisionReason::ShockwaveImpulseOnly:
        return "ShockwaveImpulseOnly";
    case ERagdollReactionDecisionReason::ShockwaveFullBody:
        return "ShockwaveFullBody";
    case ERagdollReactionDecisionReason::FullBodyThreshold:
        return "FullBodyThreshold";
    case ERagdollReactionDecisionReason::PartialSelected:
        return "PartialSelected";
    case ERagdollReactionDecisionReason::RedundantRequest:
        return "RedundantRequest";
    default:
        return "None";
    }
}
