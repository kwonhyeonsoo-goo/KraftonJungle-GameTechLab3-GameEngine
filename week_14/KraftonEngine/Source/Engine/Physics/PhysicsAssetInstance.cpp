#include "Physics/PhysicsAssetInstance.h"

#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "GameFramework/World.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Physics/IPhysicsScene.h"
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsRuntime.h"

namespace
{
    // PhysicsAsset body/constraint local frames are authored relative to bones.
    // Runtime creation and pose sync both use the same composition rule so the two
    // directions stay mathematically symmetric.
    FTransform ComposePhysicsTransforms(const FTransform& ParentWorld, const FTransform& Local)
    {
        FTransform Result = Local;
        Result.Location = ParentWorld.Location + ParentWorld.Rotation.RotateVector(Local.Location);
        Result.Rotation = ParentWorld.Rotation * Local.Rotation;
        Result.Scale = FVector::OneVector;
        return Result;
    }

    FTransform GetComponentWorldTransform(const USkeletalMeshComponent* Component)
    {
        FTransform Result;
        if (!Component)
        {
            return Result;
        }

        Result.Location = Component->GetWorldLocation();
        Result.Rotation = Component->GetWorldMatrix().ToQuat();
        Result.Scale = FVector::OneVector;
        return Result;
    }

    IPhysicsRuntime* GetPhysicsRuntime(USkeletalMeshComponent* Component)
    {
        if (!Component)
        {
            return nullptr;
        }

        UWorld* World = Component->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        IPhysicsScene* PhysicsScene = World->GetPhysicsScene();
        return PhysicsScene ? PhysicsScene->GetRuntime() : nullptr;
    }

    const IPhysicsRuntime* GetPhysicsRuntime(const USkeletalMeshComponent* Component)
    {
        if (!Component)
        {
            return nullptr;
        }

        UWorld* World = Component->GetWorld();
        if (!World)
        {
            return nullptr;
        }

        const IPhysicsScene* PhysicsScene = World->GetPhysicsScene();
        return PhysicsScene ? PhysicsScene->GetRuntime() : nullptr;
    }

    void FillShapeFilterDataFromComponent(
        FPhysicsFilterData& OutFilterData,
        const USkeletalMeshComponent* Component,
        bool bForceQueryAndPhysicsCollision,
        bool bUseIndependentRagdollCollision,
        ECollisionEnabled IndependentCollisionEnabled,
        bool bIndependentGenerateOverlapEvents)
    {
        if (!Component)
        {
            return;
        }

        OutFilterData.ObjectType = static_cast<uint32>(Component->GetCollisionObjectType());
        OutFilterData.BlockMask = 0;
        OutFilterData.OverlapMask = 0;
        // Keep ragdoll self-collision decisions at the PhysicsAsset constraint layer.
        OutFilterData.IgnoreGroup = 0;
        OutFilterData.CollisionEnabled = bUseIndependentRagdollCollision
            ? IndependentCollisionEnabled
            : (bForceQueryAndPhysicsCollision
                ? ECollisionEnabled::QueryAndPhysics
                : Component->GetCollisionEnabled());
        OutFilterData.bIsTrigger = false;
        OutFilterData.bGenerateHitEvents = true;
        OutFilterData.bGenerateOverlapEvents = bUseIndependentRagdollCollision
            ? bIndependentGenerateOverlapEvents
            : (bForceQueryAndPhysicsCollision
                ? false
                : Component->GetGenerateOverlapEvents());

        for (int32 ChannelIndex = 0; ChannelIndex < static_cast<int32>(ECollisionChannel::ActiveCount); ++ChannelIndex)
        {
            const ECollisionResponse Response =
                (bForceQueryAndPhysicsCollision && !bUseIndependentRagdollCollision)
                    ? ECollisionResponse::Block
                    : Component->GetCollisionResponseToChannel(static_cast<ECollisionChannel>(ChannelIndex));

            if (Response == ECollisionResponse::Block)
            {
                OutFilterData.BlockMask |= (1u << ChannelIndex);
            }
            else if (Response == ECollisionResponse::Overlap)
            {
                OutFilterData.OverlapMask |= (1u << ChannelIndex);
            }
        }
    }

    void BuildShapeDescs(
        const FPhysicsAssetBodySetup& BodySetup,
        const USkeletalMeshComponent* OwnerComponent,
        const FPhysicsAssetSimulationOptions& Options,
        TArray<FPhysicsShapeDesc>& OutShapes
    )
    {
        OutShapes.clear();

        for (const FPhysicsAssetShapeSetup& ShapeSetup : BodySetup.Shapes)
        {
            FPhysicsShapeDesc ShapeDesc;
            ShapeDesc.LocalTransform = ShapeSetup.LocalTransform;
            ShapeDesc.CollisionEnabled = Options.bUseIndependentRagdollCollision
                ? Options.IndependentCollisionEnabled
                : (Options.bForceQueryAndPhysicsCollision
                    ? ECollisionEnabled::QueryAndPhysics
                    : (OwnerComponent ? OwnerComponent->GetCollisionEnabled() : ECollisionEnabled::QueryAndPhysics));
            ShapeDesc.bIsTrigger = false;
            FillShapeFilterDataFromComponent(
                ShapeDesc.FilterData,
                OwnerComponent,
                Options.bForceQueryAndPhysicsCollision,
                Options.bUseIndependentRagdollCollision,
                Options.IndependentCollisionEnabled,
                Options.bIndependentGenerateOverlapEvents);
            ShapeDesc.QueryIgnoreGroup = (OwnerComponent && OwnerComponent->GetOwner())
                ? OwnerComponent->GetOwner()->GetUUID()
                : 0;

            switch (ShapeSetup.Type)
            {
            case EPhysicsAssetShapeType::Box:
                ShapeDesc.Type = EPhysicsShapeType::Box;
                ShapeDesc.BoxHalfExtent = ShapeSetup.BoxHalfExtent;
                break;
            case EPhysicsAssetShapeType::Sphere:
                ShapeDesc.Type = EPhysicsShapeType::Sphere;
                ShapeDesc.SphereRadius = ShapeSetup.SphereRadius;
                break;
            case EPhysicsAssetShapeType::Capsule:
                ShapeDesc.Type = EPhysicsShapeType::Capsule;
                ShapeDesc.CapsuleRadius = ShapeSetup.CapsuleRadius;
                ShapeDesc.CapsuleHalfHeight = ShapeSetup.CapsuleHalfHeight;
                break;
            default:
                continue;
            }

            OutShapes.push_back(ShapeDesc);
        }
    }

    bool BuildBodyCreationDesc(
        USkeletalMeshComponent* OwnerComponent,
        const FPhysicsAssetBodySetup& BodySetup,
        const FTransform& BoneWorldTransform,
        const FPhysicsAssetSimulationOptions& Options,
        FBodyCreationDesc& OutDesc
    )
    {
        if (!OwnerComponent)
        {
            return false;
        }

        OutDesc                          = FBodyCreationDesc {};
        OutDesc.OwnerActorId             = OwnerComponent->GetOwner() ? OwnerComponent->GetOwner()->GetUUID() : 0;
        OutDesc.OwnerComponentId         = OwnerComponent->GetUUID();
        OutDesc.OwnerComponentGeneration = 1;
        OutDesc.Domain                   = EPhysicsBodyDomain::Ragdoll;
        OutDesc.BoneName                 = BodySetup.BoneName;
        OutDesc.BodyType                 = EPhysicsBodyType::Dynamic;
        // Ragdoll bodies stay manual because skeletal pose sync is driven explicitly by
        // the component rather than by generic component/world transform mirroring.
        OutDesc.SyncMode = EPhysicsSyncMode::Manual;
        OutDesc.WorldTransform = ComposePhysicsTransforms(BoneWorldTransform, BodySetup.BodyLocalFrame);

        BuildShapeDescs(BodySetup, OwnerComponent, Options, OutDesc.Shapes);
        if (OutDesc.Shapes.empty())
        {
            return false;
        }

        OutDesc.Mass = BodySetup.Mass;
        OutDesc.CenterOfMassLocalOffset = BodySetup.CenterOfMassLocalOffset;
        OutDesc.LinearDamping = BodySetup.LinearDamping;
        OutDesc.AngularDamping = BodySetup.AngularDamping;
        OutDesc.MaxAngularVelocity = BodySetup.MaxAngularVelocity;
        OutDesc.PositionSolverIterationCount = BodySetup.PositionSolverIterationCount;
        OutDesc.VelocitySolverIterationCount = BodySetup.VelocitySolverIterationCount;
        OutDesc.bEnableGravity = BodySetup.bEnableGravity;
        OutDesc.bEnableCCD = BodySetup.bEnableCCD;
        OutDesc.bGenerateHitEvents = true;
        OutDesc.bGenerateOverlapEvents = Options.bUseIndependentRagdollCollision
            ? Options.bIndependentGenerateOverlapEvents
            : OwnerComponent->GetGenerateOverlapEvents();
        OutDesc.bLockLinearX = BodySetup.bLockLinearX;
        OutDesc.bLockLinearY = BodySetup.bLockLinearY;
        OutDesc.bLockLinearZ = BodySetup.bLockLinearZ;
        OutDesc.bLockAngularX = BodySetup.bLockAngularX;
        OutDesc.bLockAngularY = BodySetup.bLockAngularY;
        OutDesc.bLockAngularZ = BodySetup.bLockAngularZ;
        return true;
    }

    bool BuildConstraintCreationDesc(
        const FPhysicsAssetConstraintSetup& ConstraintSetup,
        FConstraintCreationDesc& OutDesc
    )
    {
        OutDesc = FConstraintCreationDesc{};
        OutDesc.ParentLocalFrame = ConstraintSetup.ParentLocalFrame;
        OutDesc.ChildLocalFrame = ConstraintSetup.ChildLocalFrame;
        OutDesc.Limits = ConstraintSetup.Limits;
        OutDesc.bDisableCollisionBetweenBodies = ConstraintSetup.bDisableCollisionBetweenBodies;
        return true;
    }

    bool ComputeBoneWorldTransformFromBody(
        const FPhysicsAssetBodySetup& BodySetup,
        const FTransform& BodyWorld,
        FTransform& OutBoneWorld
    )
    {
        // BodyLocalFrame is the authored offset from a bone to its rigid body. Pose sync
        // reverses that offset so the simulated body can become the final bone transform.
        const FQuat InverseBodyLocalRotation = BodySetup.BodyLocalFrame.Rotation.Inverse().GetNormalized();
        OutBoneWorld = BodyWorld;
        OutBoneWorld.Rotation = (BodyWorld.Rotation * InverseBodyLocalRotation).GetNormalized();
        OutBoneWorld.Location =
            BodyWorld.Location - OutBoneWorld.Rotation.RotateVector(BodySetup.BodyLocalFrame.Location);
        return true;
    }


    bool IsSameOrDescendantBone(const FSkeletalMesh* MeshAsset, int32 BoneIndex, int32 PossibleAncestorIndex)
    {
        if (!MeshAsset || BoneIndex < 0 || PossibleAncestorIndex < 0 ||
            BoneIndex >= static_cast<int32>(MeshAsset->Bones.size()) ||
            PossibleAncestorIndex >= static_cast<int32>(MeshAsset->Bones.size()))
        {
            return false;
        }

        int32 Cursor = BoneIndex;
        while (Cursor >= 0 && Cursor < static_cast<int32>(MeshAsset->Bones.size()))
        {
            if (Cursor == PossibleAncestorIndex)
            {
                return true;
            }
            Cursor = MeshAsset->Bones[Cursor].ParentIndex;
        }
        return false;
    }
}

bool FPhysicsAssetInstance::Initialize(USkeletalMeshComponent* InOwner, UPhysicsAsset* InAsset)
{
    Shutdown();

    if (!InOwner || !InAsset)
    {
        return false;
    }

    USkeletalMesh* Mesh = InOwner->GetSkeletalMesh();
    FSkeletalMesh* MeshAsset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
    if (!MeshAsset)
    {
        return false;
    }

    OwnerComponent = InOwner;
    SourceAsset = InAsset;

    const TArray<FPhysicsAssetBodySetup>& BodySetups = InAsset->GetBodySetups();
    BodiesByBone.resize(BodySetups.size());
    Constraints.resize(InAsset->GetConstraintSetups().size());

    // Bone name resolution is cached up front so runtime creation and pose sync do not
    // need to repeatedly scan the skeletal mesh bone list.
    for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(BodySetups.size()); ++BodyIndex)
    {
        const FName& BoneName = BodySetups[BodyIndex].BoneName;
        int32 BoneIndex = -1;

        for (int32 MeshBoneIndex = 0; MeshBoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++MeshBoneIndex)
        {
            if (MeshAsset->Bones[MeshBoneIndex].Name == BoneName.ToString())
            {
                BoneIndex = MeshBoneIndex;
                break;
            }
        }

        BoneNameToIndex[BoneName.ToString()] = BoneIndex;
        if (BodyIndex == 0)
        {
            RagdollRootBoneIndex = BoneIndex;
        }
    }

    ResetRuntimeState();
    bInitialized = true;
    return true;
}

bool FPhysicsAssetInstance::CreateBodiesAndConstraints()
{
    return CreateBodiesAndConstraints(FPhysicsAssetSimulationOptions{});
}

bool FPhysicsAssetInstance::CreateBodiesAndConstraints(const FPhysicsAssetSimulationOptions& Options)
{
    if (!bInitialized)
    {
        return false;
    }

    if (HasLivePhysicsObjects())
    {
        return true;
    }

    USkeletalMeshComponent* Owner = GetOwnerComponent();
    UPhysicsAsset* Asset = GetAsset();
    IPhysicsRuntime* Runtime = GetPhysicsRuntime(Owner);
    if (!Owner || !Asset || !Runtime)
    {
        return false;
    }

    USkeletalMesh* Mesh = Owner->GetSkeletalMesh();
    FSkeletalMesh* MeshAsset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
    if (!MeshAsset)
    {
        return false;
    }

    TArray<FTransform> BoneComponentSpaceTransforms;
    Owner->GetCurrentBoneGlobalTransforms(BoneComponentSpaceTransforms);
    if (BoneComponentSpaceTransforms.size() < MeshAsset->Bones.size())
    {
        UE_LOG("CreateBodiesAndConstraints failed: skeletal pose is incomplete. Component=%s",
            Owner->GetName().c_str());
        return false;
    }

    const FTransform ComponentWorldTransform = GetComponentWorldTransform(Owner);
    const TArray<FPhysicsAssetBodySetup>& BodySetups = Asset->GetBodySetups();
    const TArray<FPhysicsAssetConstraintSetup>& ConstraintSetups = Asset->GetConstraintSetups();

    TArray<uint8> SimulatedBodyMask;
    SimulatedBodyMask.resize(BodySetups.size(), 1);
    if (Options.bSelectedOnly)
    {
        SimulatedBodyMask.assign(BodySetups.size(), 0);
        const int32 SelectedBoneIndex = FindBoneIndexForBody(Options.SelectedBoneName);
        if (SelectedBoneIndex < 0)
        {
            UE_LOG("CreateBodiesAndConstraints failed: selected simulation has no valid selected body. Component=%s Bone=%s",
                Owner->GetName().c_str(),
                Options.SelectedBoneName.ToString().c_str());
            return false;
        }

        int32 SelectedDynamicBodyCount = 0;
        for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(BodySetups.size()); ++BodyIndex)
        {
            const int32 BodyBoneIndex = FindBoneIndexForBody(BodySetups[BodyIndex].BoneName);
            if (IsSameOrDescendantBone(MeshAsset, BodyBoneIndex, SelectedBoneIndex))
            {
                SimulatedBodyMask[BodyIndex] = 1;
                ++SelectedDynamicBodyCount;
            }
        }

        if (SelectedDynamicBodyCount <= 0)
        {
            UE_LOG("CreateBodiesAndConstraints failed: selected body chain is empty. Component=%s Bone=%s",
                Owner->GetName().c_str(),
                Options.SelectedBoneName.ToString().c_str());
            return false;
        }
    }

    if (BodiesByBone.size() != BodySetups.size())
    {
        BodiesByBone.resize(BodySetups.size());
    }
    if (Constraints.size() != ConstraintSetups.size())
    {
        Constraints.resize(ConstraintSetups.size());
    }
    ResetRuntimeState();

    int32 CreatedBodyCount = 0;
    int32 CreatedConstraintCount = 0;

    // Bodies must exist before constraints because joints are expressed in terms of
    // two already-created rigid bodies.
    for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(BodySetups.size()); ++BodyIndex)
    {
        const FPhysicsAssetBodySetup& BodySetup = BodySetups[BodyIndex];
        const int32 BoneIndex = FindBoneIndexForBody(BodySetup.BoneName);
        if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(BoneComponentSpaceTransforms.size()))
        {
            UE_LOG("Skipped PhysicsAsset body: bone not found. Component=%s Bone=%s",
                Owner->GetName().c_str(),
                BodySetup.BoneName.ToString().c_str());
            continue;
        }

        const FTransform BoneWorldTransform =
            ComposePhysicsTransforms(ComponentWorldTransform, BoneComponentSpaceTransforms[BoneIndex]);

        FBodyCreationDesc BodyDesc;
        if (!BuildBodyCreationDesc(
                Owner,
                BodySetup,
                BoneWorldTransform,
                Options,
                BodyDesc))
        {
            UE_LOG("Skipped PhysicsAsset body: invalid body setup. Component=%s Bone=%s",
                Owner->GetName().c_str(),
                BodySetup.BoneName.ToString().c_str());
            continue;
        }

        const bool bSimulateThisBody =
            BodyIndex >= 0 && BodyIndex < static_cast<int32>(SimulatedBodyMask.size())
                ? SimulatedBodyMask[BodyIndex] != 0
                : true;
        if (Options.bSelectedOnly && !bSimulateThisBody)
        {
            BodyDesc.BodyType = EPhysicsBodyType::Kinematic;
            BodyDesc.bEnableGravity = false;
        }
        else if (Options.bNoGravity)
        {
            BodyDesc.bEnableGravity = false;
        }

        FPhysicsBodyHandle BodyHandle = Runtime->CreateRigidBody(BodyDesc);
        if (!BodyHandle.IsValid())
        {
            UE_LOG("Failed to create PhysicsAsset body. Component=%s Bone=%s",
                Owner->GetName().c_str(),
                BodySetup.BoneName.ToString().c_str());
            continue;
        }

        BodiesByBone[BodyIndex] = BodyHandle;
        ++CreatedBodyCount;
    }

    for (int32 ConstraintIndex = 0; ConstraintIndex < static_cast<int32>(ConstraintSetups.size()); ++ConstraintIndex)
    {
        const FPhysicsAssetConstraintSetup& ConstraintSetup = ConstraintSetups[ConstraintIndex];
        const FPhysicsBodyHandle ParentHandle = GetBodyHandleByBoneName(ConstraintSetup.ParentBoneName);
        const FPhysicsBodyHandle ChildHandle = GetBodyHandleByBoneName(ConstraintSetup.ChildBoneName);

        if (Options.bSelectedOnly)
        {
            const int32 ParentBodyIndex = Asset->FindBodySetupIndexByBoneName(ConstraintSetup.ParentBoneName);
            const int32 ChildBodyIndex = Asset->FindBodySetupIndexByBoneName(ConstraintSetup.ChildBoneName);
            const bool bParentSimulated =
                ParentBodyIndex >= 0 && ParentBodyIndex < static_cast<int32>(SimulatedBodyMask.size()) &&
                SimulatedBodyMask[ParentBodyIndex] != 0;
            const bool bChildSimulated =
                ChildBodyIndex >= 0 && ChildBodyIndex < static_cast<int32>(SimulatedBodyMask.size()) &&
                SimulatedBodyMask[ChildBodyIndex] != 0;
            if (!bParentSimulated && !bChildSimulated)
            {
                continue;
            }
        }

        if (!ParentHandle.IsValid() || !ChildHandle.IsValid())
        {
            UE_LOG("Skipped PhysicsAsset constraint: missing body handle. Component=%s Parent=%s Child=%s",
                Owner->GetName().c_str(),
                ConstraintSetup.ParentBoneName.ToString().c_str(),
                ConstraintSetup.ChildBoneName.ToString().c_str());
            continue;
        }

        FConstraintCreationDesc ConstraintDesc;
        if (!BuildConstraintCreationDesc(ConstraintSetup, ConstraintDesc))
        {
            UE_LOG("Skipped PhysicsAsset constraint: invalid constraint setup. Component=%s Parent=%s Child=%s",
                Owner->GetName().c_str(),
                ConstraintSetup.ParentBoneName.ToString().c_str(),
                ConstraintSetup.ChildBoneName.ToString().c_str());
            continue;
        }

        FPhysicsConstraintHandle ConstraintHandle =
            Runtime->CreateConstraint(ParentHandle, ChildHandle, ConstraintDesc);
        if (!ConstraintHandle.IsValid())
        {
            UE_LOG("Failed to create PhysicsAsset constraint. Component=%s Parent=%s Child=%s",
                Owner->GetName().c_str(),
                ConstraintSetup.ParentBoneName.ToString().c_str(),
                ConstraintSetup.ChildBoneName.ToString().c_str());
            continue;
        }

        Constraints[ConstraintIndex] = ConstraintHandle;
        ++CreatedConstraintCount;
    }

    UE_LOG("Created PhysicsAsset runtime objects. Component=%s Bodies=%d Constraints=%d",
        Owner->GetName().c_str(),
        CreatedBodyCount,
        CreatedConstraintCount);

    return CreatedBodyCount > 0;
}

void FPhysicsAssetInstance::DestroyBodiesAndConstraints()
{
    IPhysicsRuntime* Runtime = GetPhysicsRuntime(GetOwnerComponent());
    int32 DestroyedConstraintCount = 0;
    int32 DestroyedBodyCount = 0;

    // Constraints go first so joints never reference a body that has already been released.
    for (FPhysicsConstraintHandle& ConstraintHandle : Constraints)
    {
        if (!ConstraintHandle.IsValid())
        {
            continue;
        }

        if (Runtime)
        {
            Runtime->DestroyConstraint(ConstraintHandle);
        }
        ConstraintHandle = FPhysicsConstraintHandle{};
        ++DestroyedConstraintCount;
    }

    for (FPhysicsBodyHandle& BodyHandle : BodiesByBone)
    {
        if (!BodyHandle.IsValid())
        {
            continue;
        }

        if (Runtime)
        {
            Runtime->DestroyRigidBody(BodyHandle);
        }
        BodyHandle = FPhysicsBodyHandle{};
        ++DestroyedBodyCount;
    }

    if (DestroyedBodyCount > 0 || DestroyedConstraintCount > 0)
    {
        const USkeletalMeshComponent* Owner = GetOwnerComponent();
        UE_LOG("Destroyed PhysicsAsset runtime objects. Component=%s Bodies=%d Constraints=%d",
            Owner ? Owner->GetName().c_str() : "None",
            DestroyedBodyCount,
            DestroyedConstraintCount);
    }
}

void FPhysicsAssetInstance::Shutdown()
{
    // Shutdown is the strongest cleanup path: drop live runtime objects first, then
    // release cached ownership/binding state so the instance becomes fully inert.
    DestroyBodiesAndConstraints();
    BodiesByBone.clear();
    Constraints.clear();
    OwnerComponent.Reset();
    SourceAsset.Reset();
    BoneNameToIndex.clear();
    RagdollRootBoneIndex = -1;
    bInitialized = false;
}

void FPhysicsAssetInstance::ResetRuntimeState()
{
    // Reset keeps the instance attached to the same asset/component pairing while
    // discarding live runtime objects that may no longer match the current state.
    DestroyBodiesAndConstraints();
}

bool FPhysicsAssetInstance::HasLivePhysicsObjects() const
{
    return GetLiveBodyCount() > 0 || GetLiveConstraintCount() > 0;
}

int32 FPhysicsAssetInstance::GetLiveBodyCount() const
{
    int32 LiveBodyCount = 0;
    for (const FPhysicsBodyHandle& BodyHandle : BodiesByBone)
    {
        if (BodyHandle.IsValid())
        {
            ++LiveBodyCount;
        }
    }

    return LiveBodyCount;
}

int32 FPhysicsAssetInstance::GetLiveConstraintCount() const
{
    int32 LiveConstraintCount = 0;
    for (const FPhysicsConstraintHandle& ConstraintHandle : Constraints)
    {
        if (ConstraintHandle.IsValid())
        {
            ++LiveConstraintCount;
        }
    }

    return LiveConstraintCount;
}

bool FPhysicsAssetInstance::PullPhysicsPose(
    TArray<FTransform>& OutBoneWorldTransforms,
    const TArray<FTransform>* ReferenceBoneComponentSpaceTransforms,
    const TArray<FTransform>* ReferenceBoneLocalTransforms) const
{
    const USkeletalMeshComponent* Owner = GetOwnerComponent();
    const UPhysicsAsset* Asset = GetAsset();
    const IPhysicsRuntime* Runtime = GetPhysicsRuntime(Owner);
    if (!Owner || !Asset || !Runtime || GetLiveBodyCount() <= 0)
    {
        return false;
    }

    std::shared_ptr<const FPhysicsWorldSnapshot> Snapshot = Runtime->AcquireLatestSnapshotRef();
    if (!Snapshot)
    {
        return false;
    }
    const uint32 SkeletalMeshComponentId = Owner->GetUUID();

    const USkeletalMesh* Mesh = Owner->GetSkeletalMesh();
    const FSkeletalMesh* MeshAsset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
    if (!MeshAsset)
    {
        return false;
    }

    TArray<FTransform> CurrentBoneComponentSpaceTransforms;
    const bool bUseReferenceComponentSpacePose =
        ReferenceBoneComponentSpaceTransforms &&
        ReferenceBoneComponentSpaceTransforms->size() >= MeshAsset->Bones.size();
    if (bUseReferenceComponentSpacePose)
    {
        CurrentBoneComponentSpaceTransforms = *ReferenceBoneComponentSpaceTransforms;
    }
    else
    {
        Owner->GetCurrentBoneGlobalTransforms(CurrentBoneComponentSpaceTransforms);
    }
    if (CurrentBoneComponentSpaceTransforms.size() < MeshAsset->Bones.size())
    {
        return false;
    }

    TArray<FTransform> CurrentBoneLocalTransforms;
    const bool bUseReferenceLocalPose =
        ReferenceBoneLocalTransforms &&
        ReferenceBoneLocalTransforms->size() >= MeshAsset->Bones.size();
    if (bUseReferenceLocalPose)
    {
        CurrentBoneLocalTransforms = *ReferenceBoneLocalTransforms;
    }
    else
    {
        CurrentBoneLocalTransforms.resize(MeshAsset->Bones.size());
        for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
        {
            CurrentBoneLocalTransforms[BoneIndex] = Owner->GetBoneLocalTransformByIndex(BoneIndex);
        }
    }

    const FTransform ComponentWorldTransform = GetComponentWorldTransform(Owner);
    OutBoneWorldTransforms.resize(MeshAsset->Bones.size());
    // Start from the current animated pose so bones without bodies remain stable instead
    // of collapsing when a PhysicsAsset only covers part of the skeleton.
    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
    {
        OutBoneWorldTransforms[BoneIndex] =
            ComposePhysicsTransforms(ComponentWorldTransform, CurrentBoneComponentSpaceTransforms[BoneIndex]);
    }

    TArray<uint8> AppliedBodyBoneMask;
    AppliedBodyBoneMask.resize(MeshAsset->Bones.size(), 0);

    const TArray<FPhysicsAssetBodySetup>& BodySetups = Asset->GetBodySetups();
    int32 AppliedBodyCount = 0;

    for (int32 BodyIndex = 0; BodyIndex < static_cast<int32>(BodySetups.size()); ++BodyIndex)
    {
        if (BodyIndex >= static_cast<int32>(BodiesByBone.size()))
        {
            continue;
        }

        const FPhysicsBodyHandle BodyHandle = BodiesByBone[BodyIndex];
        if (!BodyHandle.IsValid())
        {
            continue;
        }

        const FPhysicsAssetBodySetup& BodySetup = BodySetups[BodyIndex];
        const int32 BoneIndex = FindBoneIndexForBody(BodySetup.BoneName);
        if (BoneIndex < 0 || BoneIndex >= static_cast<int32>(OutBoneWorldTransforms.size()))
        {
            continue;
        }

        const FPhysicsBodySnapshot* BodySnapshot =
                Snapshot->FindRagdollBone(SkeletalMeshComponentId, BodySetup.BoneName);
        if (!BodySnapshot)
        {
            continue;
        }
        const FTransform BodyWorld = BodySnapshot->CurrentTransform;
        FTransform       BoneWorld = OutBoneWorldTransforms[BoneIndex];
        if (!ComputeBoneWorldTransformFromBody(BodySetup, BodyWorld, BoneWorld))
        {
            continue;
        }

        // Physics bodies do not carry meaningful bone scale, so preserve the existing scale.
        BoneWorld.Scale = OutBoneWorldTransforms[BoneIndex].Scale;
        OutBoneWorldTransforms[BoneIndex] = BoneWorld;
        AppliedBodyBoneMask[BoneIndex] = 1;
        ++AppliedBodyCount;
    }

    if (AppliedBodyCount <= 0)
    {
        return false;
    }

    FVector RootTranslationDelta = FVector::ZeroVector;
    if (RagdollRootBoneIndex >= 0 && RagdollRootBoneIndex < static_cast<int32>(OutBoneWorldTransforms.size()))
    {
        if (AppliedBodyBoneMask[RagdollRootBoneIndex] != 0)
        {
            const FTransform CurrentAnimatedRootWorld = ComposePhysicsTransforms(ComponentWorldTransform, CurrentBoneComponentSpaceTransforms[RagdollRootBoneIndex]);
            RootTranslationDelta = OutBoneWorldTransforms[RagdollRootBoneIndex].Location - CurrentAnimatedRootWorld.Location;
        }
    }

    // Only bones that have PhysicsAsset bodies are sampled directly from PhysX.
    // Descendant bones without bodies must keep their authored/current local offset
    // under the updated parent; otherwise they remain pinned in their pre-simulation
    // world position and vertices weighted to them look frozen outside the body shape.
    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
    {
        if (AppliedBodyBoneMask[BoneIndex] != 0)
        {
            continue;
        }

        const int32 ParentIndex = MeshAsset->Bones[BoneIndex].ParentIndex;
        if (ParentIndex < 0 || ParentIndex >= static_cast<int32>(OutBoneWorldTransforms.size()))
        {
            OutBoneWorldTransforms[BoneIndex].Location += RootTranslationDelta;
            continue;
        }

        OutBoneWorldTransforms[BoneIndex] = ComposePhysicsTransforms(
            OutBoneWorldTransforms[ParentIndex],
            CurrentBoneLocalTransforms[BoneIndex]);
    }

    return true;
}

UPhysicsAsset* FPhysicsAssetInstance::GetAsset() const
{
    return SourceAsset.Get();
}

USkeletalMeshComponent* FPhysicsAssetInstance::GetOwnerComponent() const
{
    return OwnerComponent.Get();
}

FPhysicsBodyHandle FPhysicsAssetInstance::GetBodyHandleByBoneName(const FName& BoneName) const
{
    const int32 BodyIndex = FindBodySetupIndexByBoneName(BoneName);
    if (BodyIndex < 0 || BodyIndex >= static_cast<int32>(BodiesByBone.size()))
    {
        return FPhysicsBodyHandle{};
    }

    return BodiesByBone[BodyIndex];
}

FTransform FPhysicsAssetInstance::GetBodyWorldTransformByBoneName(const FName& BoneName) const
{
    const USkeletalMeshComponent* Owner   = GetOwnerComponent();
    const IPhysicsRuntime*        Runtime = GetPhysicsRuntime(static_cast<const USkeletalMeshComponent*>(Owner));
    if (!Owner || !Runtime)
    {
        return FTransform();
    }

    std::shared_ptr<const FPhysicsWorldSnapshot> Snapshot     = Runtime->AcquireLatestSnapshotRef();
    const FPhysicsBodySnapshot*                  BodySnapshot = Snapshot
            ? Snapshot->FindRagdollBone(Owner->GetUUID(), BoneName)
            : nullptr;
    return BodySnapshot ? BodySnapshot->CurrentTransform : FTransform();
}

bool FPhysicsAssetInstance::HasValidBodyForBone(const FName& BoneName) const
{
    return GetBodyHandleByBoneName(BoneName).IsValid();
}

int32 FPhysicsAssetInstance::FindBodySetupIndexByBoneName(const FName& BoneName) const
{
    UPhysicsAsset* Asset = GetAsset();
    return Asset ? Asset->FindBodySetupIndexByBoneName(BoneName) : -1;
}

int32 FPhysicsAssetInstance::FindBoneIndexForBody(const FName& BoneName) const
{
    auto It = BoneNameToIndex.find(BoneName.ToString());
    if (It != BoneNameToIndex.end())
    {
        return It->second;
    }

    return -1;
}
