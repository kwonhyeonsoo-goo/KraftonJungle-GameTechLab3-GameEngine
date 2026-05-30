#include "SkeletalMeshComponent.h"
#include "Render/Proxy/SkeletalMeshSceneProxy.h"

#include "Animation/AnimationManager.h"
#include "Animation/AnimInstance.h"
#include "Animation/Sequence/AnimSequence.h"
#include "Animation/Sequence/AnimSequenceBase.h"
#include "Animation/Instance/AnimSingleNodeInstance.h"
#include "Animation/PoseContext.h"
#include "Animation/Skeleton/Skeleton.h"
#include "Animation/Skeleton/SkeletonManager.h"
#include "Asset/AssetRegistry.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "Math/Quat.h"
#include "Math/Vector.h"
#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Object/Object.h"
#include "Object/Reflection/ObjectFactory.h"
#include "Object/Reflection/UClass.h"
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsAssetManager.h"
#include "Physics/PhysicsAssetInstance.h"
#include "Render/Proxy/SkeletalMeshSceneProxy.h"
#include "Serialization/Archive.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace
{
    FMatrix GetAffineInverseForPoseSync(const FMatrix& Matrix)
    {
        const double A = Matrix.M[0][0];
        const double B = Matrix.M[0][1];
        const double C = Matrix.M[0][2];
        const double D = Matrix.M[1][0];
        const double E = Matrix.M[1][1];
        const double F = Matrix.M[1][2];
        const double G = Matrix.M[2][0];
        const double H = Matrix.M[2][1];
        const double I = Matrix.M[2][2];

        const double Det = A * (E * I - F * H) - B * (D * I - F * G) + C * (D * H - E * G);
        if (std::fabs(Det) < 1.0e-12)
        {
            return Matrix.GetInverse();
        }

        const double InvDet = 1.0 / Det;

        FMatrix Result = FMatrix::Identity;
        Result.M[0][0] = static_cast<float>((E * I - F * H) * InvDet);
        Result.M[0][1] = static_cast<float>((C * H - B * I) * InvDet);
        Result.M[0][2] = static_cast<float>((B * F - C * E) * InvDet);
        Result.M[1][0] = static_cast<float>((F * G - D * I) * InvDet);
        Result.M[1][1] = static_cast<float>((A * I - C * G) * InvDet);
        Result.M[1][2] = static_cast<float>((C * D - A * F) * InvDet);
        Result.M[2][0] = static_cast<float>((D * H - E * G) * InvDet);
        Result.M[2][1] = static_cast<float>((B * G - A * H) * InvDet);
        Result.M[2][2] = static_cast<float>((A * E - B * D) * InvDet);

        const FVector Translation = Matrix.GetLocation();
        Result.M[3][0] = -(Translation.X * Result.M[0][0] + Translation.Y * Result.M[1][0] + Translation.Z * Result.M[2][0]);
        Result.M[3][1] = -(Translation.X * Result.M[0][1] + Translation.Y * Result.M[1][1] + Translation.Z * Result.M[2][1]);
        Result.M[3][2] = -(Translation.X * Result.M[0][2] + Translation.Y * Result.M[1][2] + Translation.Z * Result.M[2][2]);
        return Result;
    }
}

USkeletalMeshComponent::~USkeletalMeshComponent()
{
    DestroyPhysicsAssetInstance();
    ClearAnimInstance();
}

FPrimitiveSceneProxy* USkeletalMeshComponent::CreateSceneProxy()
{
    return new FSkeletalMeshSceneProxy(this);
}

void USkeletalMeshComponent::SetSkeletalMesh(USkeletalMesh* InMesh)
{
    Super::SetSkeletalMesh(InMesh);
    if (InMesh)
    {
        ResolvePhysicsAssetOverride();
    }
    else
    {
        PhysicsAssetOverride.Reset();
    }

    OnPhysicsAssetChanged();
    // Mesh 가 바뀌면 이전 AnimInstance 가 가리키던 본 인덱스/카운트가 무의미해진다.
    // 새 SkeletalMesh 기준으로 AnimInstance 를 재인스턴스화한다.
    InitializeAnimation();
}

void USkeletalMeshComponent::SetPhysicsAssetOverride(UPhysicsAsset* InPhysicsAsset)
{
    if (!InPhysicsAsset)
    {
        ClearPhysicsAssetOverride();
        return;
    }

    FSkeletonCompatibilityReport Report;
    if (!CanUsePhysicsAsset(InPhysicsAsset, &Report))
    {
        UE_LOG("SetPhysicsAssetOverride rejected: skeleton mismatch. Component=%s PhysicsAsset=%s Reason=%s",
            GetName().c_str(),
            InPhysicsAsset->GetName().c_str(),
            Report.Reason.c_str());
        ClearPhysicsAssetOverride();
        return;
    }

    PhysicsAssetOverride = InPhysicsAsset;
    PhysicsAssetOverridePath = InPhysicsAsset->GetAssetPathFileName().empty()
        ? FString("None")
        : InPhysicsAsset->GetAssetPathFileName();
    OnPhysicsAssetChanged();
}

void USkeletalMeshComponent::SetPhysicsAssetOverridePath(const FString& InPath)
{
    PhysicsAssetOverridePath.SetPath(InPath.empty() ? FString("None") : InPath);
    PhysicsAssetOverride.Reset();

    if (!PhysicsAssetOverridePath.IsNull())
    {
        ResolvePhysicsAssetOverride();
    }
    else
    {
        OnPhysicsAssetChanged();
    }
}

UPhysicsAsset* USkeletalMeshComponent::GetPhysicsAssetOverride() const
{
    if (PhysicsAssetOverride.Get())
    {
        FSkeletonCompatibilityReport Report;
        if (CanUsePhysicsAsset(PhysicsAssetOverride.Get(), &Report))
        {
            return PhysicsAssetOverride.Get();
        }

        UE_LOG("GetPhysicsAssetOverride cleared incompatible cached PhysicsAsset. Component=%s PhysicsAsset=%s Reason=%s",
            GetName().c_str(),
            PhysicsAssetOverride->GetName().c_str(),
            Report.Reason.c_str());
        const_cast<USkeletalMeshComponent*>(this)->ClearPhysicsAssetOverride();
        return nullptr;
    }

    if (PhysicsAssetOverridePath.IsNull())
    {
        return nullptr;
    }

    USkeletalMeshComponent* MutableThis = const_cast<USkeletalMeshComponent*>(this);
    return MutableThis->ResolvePhysicsAssetOverride() ? PhysicsAssetOverride.Get() : nullptr;
}

UPhysicsAsset* USkeletalMeshComponent::GetEffectivePhysicsAsset() const
{
    if (UPhysicsAsset* OverridePhysicsAsset = GetPhysicsAssetOverride())
    {
        return OverridePhysicsAsset;
    }

    USkeletalMesh* Mesh = GetSkeletalMesh();
    if (!Mesh)
    {
        return nullptr;
    }

    if (UPhysicsAsset* MeshPhysicsAsset = Mesh->GetPhysicsAsset())
    {
        return MeshPhysicsAsset;
    }

    // Components resolve the final fallback order so later ragdoll code can ask only once.
    USkeleton* Skeleton = Mesh->GetSkeleton();
    return Skeleton ? Skeleton->GetDefaultPhysicsAsset() : nullptr;
}

bool USkeletalMeshComponent::ResolvePhysicsAssetOverride()
{
    if (PhysicsAssetOverride.Get())
    {
        FSkeletonCompatibilityReport Report;
        if (CanUsePhysicsAsset(PhysicsAssetOverride.Get(), &Report))
        {
            return true;
        }

        UE_LOG("ResolvePhysicsAssetOverride cleared incompatible cached PhysicsAsset. Component=%s PhysicsAsset=%s Reason=%s",
            GetName().c_str(),
            PhysicsAssetOverride->GetName().c_str(),
            Report.Reason.c_str());
        ClearPhysicsAssetOverride();
        return false;
    }

    if (PhysicsAssetOverridePath.IsNull())
    {
        PhysicsAssetOverride.Reset();
        return false;
    }

    USkeletalMesh* Mesh = GetSkeletalMesh();
    if (!Mesh)
    {
        PhysicsAssetOverride.Reset();
        return false;
    }

    UPhysicsAsset* LoadedPhysicsAsset = FPhysicsAssetManager::Get().LoadPhysicsAsset(PhysicsAssetOverridePath.ToString());
    if (!LoadedPhysicsAsset)
    {
        PhysicsAssetOverride.Reset();
        return false;
    }

    FSkeletonCompatibilityReport Report;
    if (!CanUsePhysicsAsset(LoadedPhysicsAsset, &Report))
    {
        UE_LOG("ResolvePhysicsAssetOverride rejected loaded PhysicsAsset: skeleton mismatch. Component=%s PhysicsAsset=%s Reason=%s",
            GetName().c_str(),
            LoadedPhysicsAsset->GetName().c_str(),
            Report.Reason.c_str());
        ClearPhysicsAssetOverride();
        return false;
    }

    PhysicsAssetOverride = LoadedPhysicsAsset;
    return true;
}

void USkeletalMeshComponent::ClearPhysicsAssetOverride()
{
    PhysicsAssetOverride.Reset();
    PhysicsAssetOverridePath.Reset();
    OnPhysicsAssetChanged();
}

void USkeletalMeshComponent::ResetRagdollRuntimeState()
{
    bUsePhysicsAssetPose = false;
    if (PhysicsAssetInstance)
    {
        PhysicsAssetInstance->ResetRuntimeState();
    }
}

void USkeletalMeshComponent::OnPhysicsAssetChanged()
{
    // Asset changes invalidate the current runtime shell entirely; rebuilding is cheaper than
    // trying to salvage stale body/constraint state across different bindings.
    DestroyPhysicsAssetInstance();
}

FPhysicsAssetInstance* USkeletalMeshComponent::GetPhysicsAssetInstance() const
{
    return PhysicsAssetInstance.get();
}

FPhysicsAssetInstance* USkeletalMeshComponent::GetOrCreatePhysicsAssetInstance()
{
    UPhysicsAsset* EffectivePhysicsAsset = GetEffectivePhysicsAsset();
    if (!EffectivePhysicsAsset)
    {
        DestroyPhysicsAssetInstance();
        return nullptr;
    }

    if (PhysicsAssetInstance &&
        PhysicsAssetInstance->GetAsset() == EffectivePhysicsAsset &&
        PhysicsAssetInstance->IsInitialized())
    {
        return PhysicsAssetInstance.get();
    }

    DestroyPhysicsAssetInstance();

    auto NewInstance = std::make_unique<FPhysicsAssetInstance>();
    if (!NewInstance->Initialize(this, EffectivePhysicsAsset))
    {
        return nullptr;
    }

    PhysicsAssetInstance = std::move(NewInstance);
    return PhysicsAssetInstance.get();
}

void USkeletalMeshComponent::DestroyPhysicsAssetInstance()
{
    bUsePhysicsAssetPose = false;
    if (!PhysicsAssetInstance)
    {
        return;
    }

    PhysicsAssetInstance->Shutdown();
    PhysicsAssetInstance.reset();
}

bool USkeletalMeshComponent::EnableRagdollPhysics()
{
    FPhysicsAssetInstance* Instance = GetOrCreatePhysicsAssetInstance();
    if (!Instance)
    {
        SetUsePhysicsAssetPose(false);
        return false;
    }

    if (!Instance->CreateBodiesAndConstraints())
    {
        SetUsePhysicsAssetPose(false);
        return false;
    }

    SetUsePhysicsAssetPose(true);
    return IsRagdollActive();
}

void USkeletalMeshComponent::DisableRagdollPhysics()
{
    SetUsePhysicsAssetPose(false);
    if (PhysicsAssetInstance)
    {
        PhysicsAssetInstance->DestroyBodiesAndConstraints();
    }
}

bool USkeletalMeshComponent::IsRagdollActive() const
{
    return bUsePhysicsAssetPose && PhysicsAssetInstance && PhysicsAssetInstance->HasLivePhysicsObjects();
}

int32 USkeletalMeshComponent::GetLiveRagdollBodyCount() const
{
    return PhysicsAssetInstance ? PhysicsAssetInstance->GetLiveBodyCount() : 0;
}

int32 USkeletalMeshComponent::GetLiveRagdollConstraintCount() const
{
    return PhysicsAssetInstance ? PhysicsAssetInstance->GetLiveConstraintCount() : 0;
}

UPhysicsAsset* USkeletalMeshComponent::GetActivePhysicsAsset() const
{
    return IsRagdollActive() && PhysicsAssetInstance ? PhysicsAssetInstance->GetAsset() : nullptr;
}

bool USkeletalMeshComponent::CreatePhysicsAssetInstanceBodies()
{
    return EnableRagdollPhysics();
}

void USkeletalMeshComponent::DestroyPhysicsAssetInstanceBodies()
{
    DisableRagdollPhysics();
}

void USkeletalMeshComponent::SetUsePhysicsAssetPose(bool bEnable)
{
    bUsePhysicsAssetPose =
        bEnable &&
        PhysicsAssetInstance &&
        PhysicsAssetInstance->HasLivePhysicsObjects();
}

bool USkeletalMeshComponent::ApplyPhysicsAssetPose()
{
    if (!bUsePhysicsAssetPose)
    {
        return false;
    }

    FPhysicsAssetInstance* Instance = GetPhysicsAssetInstance();
    if (!Instance || !Instance->HasLivePhysicsObjects())
    {
        SetUsePhysicsAssetPose(false);
        return false;
    }

    USkeletalMesh* Mesh = GetSkeletalMesh();
    FSkeletalMesh* MeshAsset = Mesh ? Mesh->GetSkeletalMeshAsset() : nullptr;
    if (!MeshAsset || MeshAsset->Bones.empty())
    {
        SetUsePhysicsAssetPose(false);
        return false;
    }

    TArray<FTransform> BoneWorldTransforms;
    if (!Instance->PullPhysicsPose(BoneWorldTransforms) ||
        BoneWorldTransforms.size() < MeshAsset->Bones.size())
    {
        SetUsePhysicsAssetPose(false);
        return false;
    }

    TArray<FMatrix> ComponentSpaceGlobalMatrices;
    ComponentSpaceGlobalMatrices.resize(MeshAsset->Bones.size());

    const FMatrix& ComponentWorldInverse = GetWorldInverseMatrix();
    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
    {
        ComponentSpaceGlobalMatrices[BoneIndex] =
            BoneWorldTransforms[BoneIndex].ToMatrix() * ComponentWorldInverse;
    }

    TArray<FTransform> LocalPose;
    LocalPose.resize(MeshAsset->Bones.size());
    for (int32 BoneIndex = 0; BoneIndex < static_cast<int32>(MeshAsset->Bones.size()); ++BoneIndex)
    {
        const int32 ParentIndex = MeshAsset->Bones[BoneIndex].ParentIndex;
        const FMatrix LocalMatrix = (ParentIndex >= 0)
            ? ComponentSpaceGlobalMatrices[BoneIndex] * GetAffineInverseForPoseSync(ComponentSpaceGlobalMatrices[ParentIndex])
            : ComponentSpaceGlobalMatrices[BoneIndex];
        LocalPose[BoneIndex] = FTransform(LocalMatrix);
    }

    // Full ragdoll is the final pose owner in this first sync pass; blending comes later.
    SetBoneLocalTransforms(LocalPose);
    return true;
}

void USkeletalMeshComponent::PlayAnimation(UAnimSequenceBase* NewAnimToPlay, bool bLooping)
{
    SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SetAnimation(NewAnimToPlay);
    SetLooping(bLooping);
    SetPlaying(NewAnimToPlay != nullptr);
}

void USkeletalMeshComponent::StopAnimation()
{
    SetAnimation(nullptr);
    SetPlaying(false);

    if (UAnimSingleNodeInstance* SingleNode = (IsValid(AnimInstance) ? Cast<UAnimSingleNodeInstance>(AnimInstance) : nullptr))
    {
        SingleNode->SetCurrentTime(0.0f);
    }
}

// ──────────────────────────────────────────────
// Animation API
// ──────────────────────────────────────────────
void USkeletalMeshComponent::SetAnimationMode(EAnimationMode InMode)
{
    if (AnimationMode == InMode) return;
    AnimationMode = InMode;
    InitializeAnimation();
}

bool USkeletalMeshComponent::CanUseAnimation(UAnimSequenceBase* InAsset) const
{
    if (!InAsset)
    {
        return true;
    }

    const USkeletalMesh* Mesh = GetSkeletalMesh();
    if (!Mesh)
    {
        return false;
    }

    if (const UAnimSequence* Sequence = Cast<UAnimSequence>(InAsset))
    {
        FSkeletonCompatibilityReport Report;
        const bool bCompatible = FAssetRegistry::CheckAnimationForMesh(Sequence, Mesh, &Report);
        if (!bCompatible)
        {
            UE_LOG("SetAnimation rejected: skeleton mismatch. Anim=%s Mesh=%s Reason=%s",
                Sequence->GetName().c_str(),
                Mesh->GetName().c_str(),
                Report.Reason.c_str());
        }
        return bCompatible;
    }

    return true;
}

void USkeletalMeshComponent::SetAnimation(UAnimSequenceBase* InAsset)
{
    if (!CanUseAnimation(InAsset))
    {
        return;
    }

    AnimationData.AnimToPlay = InAsset;

    if (UAnimSequence* Sequence = Cast<UAnimSequence>(InAsset))
    {
        AnimationData.AnimToPlayPath = Sequence->GetAssetPathFileName();
    }
    else if (!InAsset)
    {
        AnimationData.AnimToPlayPath = "None";
    }

    if (UAnimSingleNodeInstance* SingleNode = (IsValid(AnimInstance) ? Cast<UAnimSingleNodeInstance>(AnimInstance) : nullptr))
    {
        SingleNode->SetAnimationAsset(InAsset);
    }
}

bool USkeletalMeshComponent::SetAnimationByPath(const FString& AnimationPath)
{
    if (AnimationPath.empty() || AnimationPath == "None")
    {
        SetAnimation(nullptr);
        return true;
    }

    UAnimSequence* LoadedAnimation = FAnimationManager::Get().LoadAnimation(AnimationPath);
    if (!LoadedAnimation || !CanUseAnimation(LoadedAnimation))
    {
        return false;
    }

    SetAnimation(LoadedAnimation);
    return true;
}

bool USkeletalMeshComponent::PlayAnimationByPath(const FString& AnimationPath, bool bLooping)
{
    if (!SetAnimationByPath(AnimationPath))
    {
        return false;
    }

    SetAnimationMode(EAnimationMode::AnimationSingleNode);
    SetLooping(bLooping);
    SetPlaying(AnimationData.AnimToPlay != nullptr);
    return AnimationData.AnimToPlay != nullptr;
}

void USkeletalMeshComponent::SetPlayRate(float InRate)
{
    AnimationData.PlayRate = InRate;
    if (UAnimSingleNodeInstance* SingleNode = (IsValid(AnimInstance) ? Cast<UAnimSingleNodeInstance>(AnimInstance) : nullptr))
    {
        SingleNode->SetPlayRate(InRate);
    }
}

void USkeletalMeshComponent::SetLooping(bool bInLoop)
{
    AnimationData.bLooping = bInLoop;
    if (UAnimSingleNodeInstance* SingleNode = (IsValid(AnimInstance) ? Cast<UAnimSingleNodeInstance>(AnimInstance) : nullptr))
    {
        SingleNode->SetLooping(bInLoop);
    }
}

void USkeletalMeshComponent::SetPlaying(bool bInPlay)
{
    AnimationData.bPlaying = bInPlay;
    if (UAnimSingleNodeInstance* SingleNode = (IsValid(AnimInstance) ? Cast<UAnimSingleNodeInstance>(AnimInstance) : nullptr))
    {
        SingleNode->SetPlaying(bInPlay);
    }
}

void USkeletalMeshComponent::SetAnimInstanceClass(UClass* InClass)
{
    if (AnimInstanceClass.Get() == InClass) return;
    AnimInstanceClass = InClass;   // TSubclassOf 가 IsA 가드로 검증 (잘못된 클래스 → nullptr).
    if (AnimationMode == EAnimationMode::AnimationCustom)
    {
        InitializeAnimation();
    }
}

void USkeletalMeshComponent::SetAnimInstance(UAnimInstance* InInstance)
{
    if (InInstance && !IsValid(InInstance)) return;
    if (AnimInstance == InInstance) return;
    ClearAnimInstance();
    AnimInstance = InInstance;
    if (IsValid(AnimInstance))
    {
        AnimInstance->SetOuter(this);
        AnimInstance->SetOwningComponent(this);
        AnimInstance->NativeInitializeAnimation();
    }
}

UAnimSingleNodeInstance* USkeletalMeshComponent::GetAnimNodeInstance(FName NodeName) const
{
    (void)NodeName;
    return IsValid(AnimInstance) ? Cast<UAnimSingleNodeInstance>(AnimInstance) : nullptr;
}

void USkeletalMeshComponent::LoadAnimationFromPath()
{
    AnimationData.AnimToPlay = nullptr;

    if (AnimationData.AnimToPlayPath.empty() || AnimationData.AnimToPlayPath == "None")
    {
        return;
    }

    UAnimSequence* LoadedAnimation = FAnimationManager::Get().LoadAnimation(AnimationData.AnimToPlayPath.ToString());
    if (LoadedAnimation && CanUseAnimation(LoadedAnimation))
    {
        AnimationData.AnimToPlay = LoadedAnimation;
    }
    else
    {
        AnimationData.AnimToPlay = nullptr;
    }
}

bool USkeletalMeshComponent::CanUsePhysicsAsset(UPhysicsAsset* InPhysicsAsset, FSkeletonCompatibilityReport* OutReport) const
{
    if (!InPhysicsAsset)
    {
        if (OutReport)
        {
            OutReport->Result = ESkeletonCompatibilityResult::Incompatible;
            OutReport->Reason = "null physics asset";
        }
        return false;
    }

    const USkeletalMesh* Mesh = GetSkeletalMesh();
    if (!Mesh)
    {
        if (OutReport)
        {
            OutReport->Result = ESkeletonCompatibilityResult::Incompatible;
            OutReport->Reason = "skeletal mesh is missing";
        }
        return false;
    }

    const FSkeletonCompatibilityReport Report =
        FSkeletonManager::CheckCompatibility(Mesh->GetSkeletonBinding(), InPhysicsAsset->GetSkeletonBinding());

    if (OutReport)
    {
        *OutReport = Report;
    }

    return Report.Result == ESkeletonCompatibilityResult::ExactSkeleton;
}

void USkeletalMeshComponent::InitializeAnimation()
{
    if (!GetSkeletalMesh())
    {
        ClearAnimInstance();
        return;
    }
    if (AnimationMode == EAnimationMode::None)
    {
        ClearAnimInstance();
        return;
    }

    if (AnimationMode == EAnimationMode::AnimationSingleNode &&
        !AnimationData.AnimToPlay &&
        !AnimationData.AnimToPlayPath.empty() &&
        AnimationData.AnimToPlayPath != "None")
    {
        LoadAnimationFromPath();
    }

    if (AnimationMode == EAnimationMode::AnimationSingleNode && !CanUseAnimation(AnimationData.AnimToPlay))
    {
        AnimationData.AnimToPlay = nullptr;
        AnimationData.AnimToPlayPath = "None";
    }

    switch (AnimationMode)
    {
    case EAnimationMode::AnimationSingleNode:
    {
        ClearAnimInstance();

        UAnimSingleNodeInstance* Single =
            UObjectManager::Get().CreateObject<UAnimSingleNodeInstance>(this);
        AnimInstance = Single;
        Single->SetOwningComponent(this);
        Single->SetAnimationAsset(AnimationData.AnimToPlay);
        Single->SetPlayRate(AnimationData.PlayRate);
        Single->SetLooping(AnimationData.bLooping);
        Single->SetPlaying(AnimationData.bPlaying && AnimationData.AnimToPlay != nullptr);
        Single->NativeInitializeAnimation();
        break;
    }
    case EAnimationMode::AnimationCustom:
    {
        UClass* DesiredClass = AnimInstanceClass.Get();
        if (!DesiredClass)
        {
            ClearAnimInstance();
            return;
        }

        if (IsValid(AnimInstance) && AnimInstance->GetClass() == DesiredClass)
        {
            AnimInstance->SetOuter(this);
            AnimInstance->SetOwningComponent(this);
            AnimInstance->NativeInitializeAnimation();
            break;
        }

        ClearAnimInstance();

        UObject* Obj = FObjectFactory::Get().Create(DesiredClass->GetName(), this);
        AnimInstance = Cast<UAnimInstance>(Obj);
		if (!AnimInstance)
        {
            // 클래스가 등록 안됐거나 캐스트 실패 — 무관한 객체가 생성됐을 수 있으니 정리.
            if (Obj) UObjectManager::Get().DestroyObject(Obj);
            return;
        }
        AnimInstance->SetOwningComponent(this);

        AnimInstance->NativeInitializeAnimation();
        break;
    }
    default:
        break;
    }
}

void USkeletalMeshComponent::ClearAnimInstance()
{
    UAnimInstance* InstanceToDestroy = IsValid(AnimInstance) ? AnimInstance : nullptr;
    AnimInstance = nullptr;
    if (InstanceToDestroy)
    {
        InstanceToDestroy->SetOwningComponent(nullptr);
        UObjectManager::Get().DestroyObject(InstanceToDestroy);
    }
}

void USkeletalMeshComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction& ThisTickFunction)
{
    if (!IsValid(this) || IsPendingKill())
    {
        return;
    }

    if (bUsePhysicsAssetPose && ApplyPhysicsAssetPose())
    {
        UMeshComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
        return;
    }

    if (EvaluateAnimInstance(DeltaTime))
    {
        UMeshComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);
        return;
    }

    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// ──────────────────────────────────────────────
// Editor / 직렬화 통합
// ──────────────────────────────────────────────
void USkeletalMeshComponent::GetEditableProperties(TArray<FPropertyValue>& OutProps)
{
    Super::GetEditableProperties(OutProps);

    // AnimInstance 자체 properties (Speed 등) 도 패널에 같이 노출 — 컴포넌트가 forward.
    // 자식이 자기 카테고리(예: "Animation|Character") 로 그룹화.
    if (IsValid(AnimInstance)) AnimInstance->GetEditableProperties(OutProps);
}

void USkeletalMeshComponent::PostEditProperty(const char* PropertyName)
{
    Super::PostEditProperty(PropertyName);
    if (!PropertyName) return;

    if (std::strcmp(PropertyName, "AnimationMode") == 0)
    {
        InitializeAnimation();
    }
    else if (std::strcmp(PropertyName, "AnimInstanceClass") == 0)
    {
        // 클래스 슬롯이 바뀌면 Custom 모드에서 인스턴스 재생성 필요. (ours — Phase 6)
        if (AnimationMode == EAnimationMode::AnimationCustom) InitializeAnimation();
    }
    else if (std::strcmp(PropertyName, "AnimationData") == 0)
    {
        LoadAnimationFromPath();

        if (IsValid(AnimInstance))
        {
            InitializeAnimation();
        }
    }
    else if (std::strcmp(PropertyName, "AnimToPlayPath") == 0)
    {
        // theirs (main): FAnimationManager 가 path 로 실제 UAnimSequence 로딩 — Phase 4 의 TODO 해소.
        // Mode 가 None 이면 SingleNode 로 자동 전환, AnimInstance 없으면 Initialize, 있으면 SingleNode setter 들 갱신.
        LoadAnimationFromPath();

        if (AnimationMode == EAnimationMode::None)
        {
            AnimationMode = EAnimationMode::AnimationSingleNode;
        }

        if (!IsValid(AnimInstance))
        {
            InitializeAnimation();
        }
        else if (UAnimSingleNodeInstance* SingleNode = (IsValid(AnimInstance) ? Cast<UAnimSingleNodeInstance>(AnimInstance) : nullptr))
        {
            if (!CanUseAnimation(AnimationData.AnimToPlay))
            {
                AnimationData.AnimToPlay = nullptr;
                AnimationData.AnimToPlayPath = "None";
            }
            SingleNode->SetAnimationAsset(AnimationData.AnimToPlay);
            SingleNode->SetPlayRate(AnimationData.PlayRate);
            SingleNode->SetLooping(AnimationData.bLooping);
            SingleNode->SetPlaying(AnimationData.bPlaying && AnimationData.AnimToPlay != nullptr);
        }
    }
    else if (std::strcmp(PropertyName, "PlayRate") == 0)
    {
        SetPlayRate(AnimationData.PlayRate);
    }
    else if (std::strcmp(PropertyName, "bLooping") == 0)
    {
        SetLooping(AnimationData.bLooping);
    }
    else if (std::strcmp(PropertyName, "bPlaying") == 0)
    {
        SetPlaying(AnimationData.bPlaying);
    }
    else if (std::strcmp(PropertyName, "PhysicsAssetOverridePath") == 0 ||
             std::strcmp(PropertyName, "Physics Asset Override") == 0)
    {
        if (!PhysicsAssetOverridePath.IsNull())
        {
            ResolvePhysicsAssetOverride();
        }
        else
        {
            PhysicsAssetOverride.Reset();
        }

        OnPhysicsAssetChanged();
    }

    // AnimInstance 자체 properties 는 자식이 자체 PostEdit 처리. 컴포넌트는 dispatch 만.
    // 컴포넌트가 인식한 이름과 겹치지 않는 한 무해 (자식이 모르는 이름은 no-op).
    if (IsValid(AnimInstance)) AnimInstance->PostEditProperty(PropertyName);
}

void USkeletalMeshComponent::Serialize(FArchive& Ar)
{
    Super::Serialize(Ar);

    uint8 ModeRaw = static_cast<uint8>(AnimationMode);
    Ar << ModeRaw;
    AnimationMode = static_cast<EAnimationMode>(ModeRaw);

    // AnimToPlay 의 path 만 라운드트립. 실제 포인터 복원은 InitializeAnimation() → LoadAnimationFromPath() 가 처리.
    FString AnimToPlayPath = Ar.IsSaving() ? AnimationData.AnimToPlayPath.ToString() : FString();
    Ar << AnimToPlayPath;
    if (Ar.IsLoading())
    {
        AnimationData.AnimToPlayPath.SetPath(AnimToPlayPath);
        PhysicsAssetOverride.Reset();
        ResetRagdollRuntimeState();
    }
    Ar << AnimationData.PlayRate;
    Ar << AnimationData.bLooping;
    Ar << AnimationData.bPlaying;

}

bool USkeletalMeshComponent::EvaluateAnimInstance(float DeltaTime)
{
    if (!IsValid(this) || IsPendingKill()) return false;
    if (!IsValid(AnimInstance))
    {
        AnimInstance = nullptr;
        return false;
    }

    USkeletalMesh* Mesh = GetSkeletalMesh();
    if (!Mesh) return false;
    FSkeletalMesh* Asset = Mesh->GetSkeletalMeshAsset();
    if (!Asset || Asset->Bones.empty()) return false;

    if (UAnimSingleNodeInstance* SingleNode = (IsValid(AnimInstance) ? Cast<UAnimSingleNodeInstance>(AnimInstance) : nullptr))
    {
        if (!CanUseAnimation(SingleNode->GetAnimationAsset()))
        {
            SingleNode->SetAnimationAsset(nullptr);
            return false;
        }
    }

    AnimInstance->UpdateAnimation(DeltaTime);
    if (!IsValid(AnimInstance))
    {
        AnimInstance = nullptr;
        return false;
    }

    // Root motion 적용은 UCharacterMovementComponent 가 책임.
    // CMC::TickComponent (TG_DuringPhysics) 가 매 frame 이 AnimInstance->ConsumeRootMotion 으로
    // 누적값을 가져가 capsule 이동 / 회전에 반영한다 (sweep / floor stick 통과).
    // Mesh 는 actor transform 을 직접 만지지 않는다 — UE 본가 패턴.
    //
    // 주의: CMC 가 없는 actor 에 root motion 켠 anim 을 붙이면 누적값이 anywhere 도
    // 소비되지 않아 in-place 로 보인다. ACharacter 외 케이스에서 root motion 이 필요해지면
    // 별도 소비 경로가 추가되어야 한다.

    FPoseContext Out;
    Out.SkeletalMesh = Mesh;
    Out.Pose.resize(Asset->Bones.size());
    Out.ResetToRefPose();
    if (!IsValid(AnimInstance))
    {
        AnimInstance = nullptr;
        return false;
    }
    AnimInstance->EvaluatePose(Out);

    SetAnimationPose(Out.Pose, Out.MorphWeights);
    return true;
}


void USkeletalMeshComponent::BeginDestroy()
{
    DestroyPhysicsAssetInstance();
    ClearAnimInstance();
    Super::BeginDestroy();
}
