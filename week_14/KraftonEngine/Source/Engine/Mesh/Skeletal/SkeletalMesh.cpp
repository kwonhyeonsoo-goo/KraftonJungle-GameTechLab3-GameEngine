#include "SkeletalMesh.h"
#include "Object/Reflection/ObjectFactory.h"
#include "Serialization/Archive.h"
#include "Animation/Skeleton/Skeleton.h"
#include "Animation/Skeleton/SkeletonManager.h"
#include "Core/Logging/Log.h"
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsAssetManager.h"
#include "Engine/Profiling/Stats/MemoryStats.h"

USkeletalMesh::~USkeletalMesh()
{
    ReleaseTrackedMemory();
}

void USkeletalMesh::Serialize(FArchive& Ar)
{
    if (Ar.IsLoading())
    {
        ReleaseTrackedMemory();
        if (!SkeletalMeshAsset)
        {
            SkeletalMeshAsset = std::make_unique<FSkeletalMesh>();
        }
    }
    else if (!SkeletalMeshAsset)
    {
        SkeletalMeshAsset = std::make_unique<FSkeletalMesh>();
    }

    if (Ar.IsSaving())
    {
        SyncSkeletonBindingToAsset();
    }

    Ar << SkeletalMeshAsset->PathFileName;
    Ar << SkeletalMeshAsset->SkeletonPath;
    Ar << SkeletalMeshAsset->SkeletonAssetGuid;
    Ar << SkeletalMeshAsset->SkeletonCompatibilitySignature;
    Ar << SkeletalMeshAsset->Vertices;
    Ar << SkeletalMeshAsset->Indices;
    Ar << SkeletalMeshAsset->Sections;
    Ar << SkeletalMeshAsset->MeshRanges;
    Ar << SkeletalMeshAsset->Bones;
    Ar << SkeletalMaterials;
    Ar << SkeletalMeshAsset->MorphTargets;
    Ar << PhysicsAssetPath;

    if (Ar.IsLoading())
    {
        SkeletalMeshAsset->NormalizeBonePoseData();
        SyncSkeletonBindingFromAsset();
        PhysicsAsset.Reset();
        CacheSectionMaterialIndices();
        SkeletalMeshAsset->bBoundsValid = false;
    }
}

void USkeletalMesh::SetSkeletalMeshAsset(FSkeletalMesh* InMesh)
{
    if (SkeletalMeshAsset.get() == InMesh)
    {
        CacheSectionMaterialIndices();
        return;
    }

    SetSkeletalMeshAsset(std::unique_ptr<FSkeletalMesh>(InMesh));
}

void USkeletalMesh::SetSkeletalMeshAsset(std::unique_ptr<FSkeletalMesh> InMesh)
{
    if (SkeletalMeshAsset.get() == InMesh.get())
    {
        InMesh.release();
        CacheSectionMaterialIndices();
        return;
    }

    ReleaseTrackedMemory();
    SkeletalMeshAsset = std::move(InMesh);

    if (SkeletalMeshAsset)
    {
        SkeletalMeshAsset->NormalizeBonePoseData();
        SkeletalMeshAsset->bBoundsValid = false;
    }
    SyncSkeletonBindingFromAsset();
    if (PhysicsAsset.Get())
    {
        FSkeletonCompatibilityReport Report;
        if (!CanUsePhysicsAsset(PhysicsAsset.Get(), &Report))
        {
            UE_LOG("SkeletalMesh default PhysicsAsset cleared after mesh asset change. Mesh=%s PhysicsAsset=%s Reason=%s",
                GetName().c_str(),
                PhysicsAsset->GetName().c_str(),
                Report.Reason.c_str());
            ClearPhysicsAsset();
        }
    }
    CacheSectionMaterialIndices();
}

FSkeletalMesh* USkeletalMesh::GetSkeletalMeshAsset() const
{
    return SkeletalMeshAsset.get();
}

void USkeletalMesh::SetSkeletalMaterials(TArray<FSkeletalMaterial>&& InMaterials)
{
    SkeletalMaterials = std::move(InMaterials);
    CacheSectionMaterialIndices();
}

const TArray<FSkeletalMaterial>& USkeletalMesh::GetSkeletalMaterials() const
{
    return SkeletalMaterials;
}

void USkeletalMesh::InitResources(ID3D11Device* InDevice)
{
    if (!InDevice || !SkeletalMeshAsset) return;

    if (!bMemoryTracked)
    {
        TrackedMemorySize = CalculateTrackedMemorySize();
        MemoryStats::AddSkeletalMeshCPUMemory(TrackedMemorySize);
        bMemoryTracked = true;
    }

    TMeshData<FVertexPNCTBW> RenderMeshData;
    RenderMeshData.Vertices.reserve(SkeletalMeshAsset->Vertices.size());

    for (const FVertexPNCTBW& RawVert : SkeletalMeshAsset->Vertices)
    {
        FVertexPNCTBW RenderVert;
        RenderVert.Position = RawVert.Position;
        RenderVert.Normal   = RawVert.Normal;
        RenderVert.Color    = RawVert.Color;
        RenderVert.UV       = RawVert.UV;
        RenderVert.Tangent  = RawVert.Tangent;
        std::copy(std::begin(RawVert.BoneIndices), std::end(RawVert.BoneIndices), std::begin(RenderVert.BoneIndices));
        std::copy(std::begin(RawVert.BoneWeights), std::end(RawVert.BoneWeights), std::begin(RenderVert.BoneWeights));
        RenderMeshData.Vertices.push_back(RenderVert);
    }
    RenderMeshData.Indices = SkeletalMeshAsset->Indices;

    SkeletalMeshAsset->RenderBuffer = std::make_unique<FMeshBuffer>();
    SkeletalMeshAsset->RenderBuffer->Create(InDevice, RenderMeshData);
}

void USkeletalMesh::SetSkeleton(USkeleton* InSkeleton)
{
    Skeleton = InSkeleton;

    USkeleton* ValidSkeleton = Skeleton.Get();
    if (ValidSkeleton)
    {
        SetSkeletonBinding(ValidSkeleton->GetSkeletonBinding());
    }
    else
    {
        FSkeletonBinding EmptyBinding;
        SetSkeletonBinding(EmptyBinding);
    }
}

USkeleton* USkeletalMesh::GetSkeleton() const
{
    return Skeleton.Get();
}

void USkeletalMesh::SetSkeletonBinding(const FSkeletonBinding& InBinding)
{
    SkeletonBinding = InBinding;
    if (SkeletonBinding.SkeletonPath.empty())
    {
        SkeletonBinding.SkeletonPath = "None";
    }
    SyncSkeletonBindingToAsset();

    if (PhysicsAsset.Get())
    {
        FSkeletonCompatibilityReport Report;
        if (!CanUsePhysicsAsset(PhysicsAsset.Get(), &Report))
        {
            UE_LOG("SkeletalMesh default PhysicsAsset cleared after skeleton binding change. Mesh=%s PhysicsAsset=%s Reason=%s",
                GetName().c_str(),
                PhysicsAsset->GetName().c_str(),
                Report.Reason.c_str());
            ClearPhysicsAsset();
        }
    }
}

void USkeletalMesh::SetPhysicsAsset(UPhysicsAsset* InPhysicsAsset)
{
    if (!InPhysicsAsset)
    {
        ClearPhysicsAsset();
        return;
    }

    FSkeletonCompatibilityReport Report;
    if (!CanUsePhysicsAsset(InPhysicsAsset, &Report))
    {
        UE_LOG("SetPhysicsAsset rejected: skeleton mismatch. Mesh=%s PhysicsAsset=%s Reason=%s",
            GetName().c_str(),
            InPhysicsAsset->GetName().c_str(),
            Report.Reason.c_str());
        ClearPhysicsAsset();
        return;
    }

    PhysicsAsset     = InPhysicsAsset;
    PhysicsAssetPath = InPhysicsAsset->GetAssetPathFileName().empty()
            ? FString("None")
            : InPhysicsAsset->GetAssetPathFileName();
}

UPhysicsAsset* USkeletalMesh::GetPhysicsAsset() const
{
    if (PhysicsAsset.Get())
    {
        FSkeletonCompatibilityReport Report;
        if (CanUsePhysicsAsset(PhysicsAsset.Get(), &Report))
        {
            return PhysicsAsset.Get();
        }

        UE_LOG("GetPhysicsAsset cleared incompatible cached PhysicsAsset. Mesh=%s PhysicsAsset=%s Reason=%s",
            GetName().c_str(),
            PhysicsAsset->GetName().c_str(),
            Report.Reason.c_str());
        const_cast<USkeletalMesh*>(this)->ClearPhysicsAsset();
        return nullptr;
    }

    if (PhysicsAssetPath.empty() || PhysicsAssetPath == "None")
    {
        return nullptr;
    }

    USkeletalMesh* MutableThis = const_cast<USkeletalMesh*>(this);
    return MutableThis->ResolvePhysicsAsset() ? PhysicsAsset.Get() : nullptr;
}

void USkeletalMesh::SetPhysicsAssetPath(const FString& InPath)
{
    PhysicsAssetPath = InPath.empty() ? FString("None") : InPath;
    PhysicsAsset.Reset();

    if (PhysicsAssetPath == "None")
    {
        return;
    }
}

bool USkeletalMesh::ResolvePhysicsAsset()
{
    if (PhysicsAsset.Get())
    {
        FSkeletonCompatibilityReport Report;
        if (CanUsePhysicsAsset(PhysicsAsset.Get(), &Report))
        {
            return true;
        }

        UE_LOG("ResolvePhysicsAsset cleared incompatible cached PhysicsAsset. Mesh=%s PhysicsAsset=%s Reason=%s",
            GetName().c_str(),
            PhysicsAsset->GetName().c_str(),
            Report.Reason.c_str());
        ClearPhysicsAsset();
        return false;
    }

    if (PhysicsAssetPath.empty() || PhysicsAssetPath == "None")
    {
        PhysicsAsset.Reset();
        return false;
    }

    UPhysicsAsset* LoadedPhysicsAsset = FPhysicsAssetManager::Get().LoadPhysicsAsset(PhysicsAssetPath);
    if (!LoadedPhysicsAsset)
    {
        PhysicsAsset.Reset();
        return false;
    }

    FSkeletonCompatibilityReport Report;
    if (!CanUsePhysicsAsset(LoadedPhysicsAsset, &Report))
    {
        UE_LOG("ResolvePhysicsAsset rejected loaded PhysicsAsset: skeleton mismatch. Mesh=%s PhysicsAsset=%s Reason=%s",
            GetName().c_str(),
            LoadedPhysicsAsset->GetName().c_str(),
            Report.Reason.c_str());
        ClearPhysicsAsset();
        return false;
    }

    PhysicsAsset = LoadedPhysicsAsset;
    return true;
}

void USkeletalMesh::ClearPhysicsAsset()
{
    PhysicsAsset.Reset();
    PhysicsAssetPath = "None";
}

uint32 USkeletalMesh::CalculateTrackedMemorySize() const
{
    if (!SkeletalMeshAsset)
    {
        return 0;
    }

    return
            static_cast<uint32>(SkeletalMeshAsset->Vertices.size() * sizeof(FVertexPNCTBW)) +
            static_cast<uint32>(SkeletalMeshAsset->Indices.size() * sizeof(uint32));
}

void USkeletalMesh::ReleaseTrackedMemory()
{
    if (!bMemoryTracked)
    {
        return;
    }

    MemoryStats::SubSkeletalMeshCPUMemory(TrackedMemorySize);
    bMemoryTracked = false;
    TrackedMemorySize = 0;
}

void USkeletalMesh::CacheSectionMaterialIndices()
{
    if (!SkeletalMeshAsset)
    {
        return;
    }

    for (FSkeletalMeshSection& Section : SkeletalMeshAsset->Sections)
    {
        Section.MaterialIndex = -1;
        for (int32 i = 0; i < static_cast<int32>(SkeletalMaterials.size()); ++i)
        {
            if (SkeletalMaterials[i].MaterialSlotName == Section.MaterialSlotName)
            {
                Section.MaterialIndex = i;
                break;
            }
        }
    }
}

void USkeletalMesh::SyncSkeletonBindingToAsset()
{
    if (!SkeletalMeshAsset)
    {
        return;
    }

    SkeletalMeshAsset->SkeletonPath                   = SkeletonBinding.SkeletonPath.empty() ? FString("None") : SkeletonBinding.SkeletonPath;
    SkeletalMeshAsset->SkeletonAssetGuid              = SkeletonBinding.SkeletonAssetGuid;
    SkeletalMeshAsset->SkeletonCompatibilitySignature = SkeletonBinding.CompatibilitySignature;
}

void USkeletalMesh::SyncSkeletonBindingFromAsset()
{
    if (!SkeletalMeshAsset)
    {
        return;
    }

    SkeletonBinding.SkeletonPath           = SkeletalMeshAsset->SkeletonPath.empty() ? FString("None") : SkeletalMeshAsset->SkeletonPath;
    SkeletonBinding.SkeletonAssetGuid      = SkeletalMeshAsset->SkeletonAssetGuid;
    SkeletonBinding.CompatibilitySignature = SkeletalMeshAsset->SkeletonCompatibilitySignature;
}

bool USkeletalMesh::CanUsePhysicsAsset(UPhysicsAsset* InPhysicsAsset, FSkeletonCompatibilityReport* OutReport) const
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

    const FSkeletonCompatibilityReport Report =
            FSkeletonManager::CheckCompatibility(GetSkeletonBinding(), InPhysicsAsset->GetSkeletonBinding());

    if (OutReport)
    {
        *OutReport = Report;
    }

    return Report.Result == ESkeletonCompatibilityResult::ExactSkeleton;
}
