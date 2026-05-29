#include "Animation/Skeleton/Skeleton.h"

void USkeleton::Serialize(FArchive& Ar)
{
    SerializeCurrentPayload(Ar);
}

void USkeleton::SerializeLegacyPayload(FArchive& Ar)
{
    UObject::Serialize(Ar);

    Ar << AssetPathFileName;
    Ar << SkeletonAssetGuid;
    Ar << CompatibilitySignature;
    Ar << ReferenceSkeleton;

    if (Ar.IsLoading())
    {
        Sockets.clear();
        RebuildReferenceSkeletonDerivedData();
        RebuildBoneNameCache();
    }
}

void USkeleton::SerializeCurrentPayload(FArchive& Ar)
{
    UObject::Serialize(Ar);

    Ar << AssetPathFileName;
    Ar << SkeletonAssetGuid;
    Ar << CompatibilitySignature;
    Ar << ReferenceSkeleton;
	
    uint32 SocketCount = static_cast<uint32>(Sockets.size());
    Ar << SocketCount;
    if (Ar.IsLoading())
    {
	    Sockets.resize(SocketCount);
    }
    for (FSkeletalMeshSocket& Socket : Sockets)
    {
	    Ar << Socket;
    }

    if (Ar.IsLoading())
    {
        RebuildReferenceSkeletonDerivedData();
        RebuildBoneNameCache();
    }
}

void USkeleton::RebuildBoneNameCache()
{
    BoneNameToIndex.clear();

    for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNumBones(); ++BoneIndex)
    {
        BoneNameToIndex[ReferenceSkeleton.Bones[BoneIndex].Name] = BoneIndex;
    }
}

int32 USkeleton::FindBoneIndex(const FString& BoneName) const
{
    auto It = BoneNameToIndex.find(BoneName);
    if (It != BoneNameToIndex.end())
    {
        return It->second;
    }

    return ReferenceSkeleton.FindBoneIndex(BoneName);
}

const FSkeletalMeshSocket* USkeleton::FindSocket(const FName& Name) const
{
    const int32 SocketIndex = FindSocketIndex(Name);
    return SocketIndex >= 0 ? &Sockets[SocketIndex] : nullptr;
}

bool USkeleton::HasSocket(const FName& Name) const
{
    return FindSocketIndex(Name) >= 0;
}

int32 USkeleton::FindSocketIndex(const FName& Name) const
{
    if (Name == FName::None || !Name.IsValid())
    {
        return -1;
    }

    for (int32 SocketIndex = 0; SocketIndex < static_cast<int32>(Sockets.size()); ++SocketIndex)
    {
        if (Sockets[SocketIndex].Name == Name)
        {
            return SocketIndex;
        }
    }

    return -1;
}

bool USkeleton::ResolveSocketBoneIndex(const FSkeletalMeshSocket& Socket, int32& OutBoneIndex) const
{
    OutBoneIndex = FindBoneIndex(Socket.BoneName.ToString());
    return OutBoneIndex >= 0;
}

void USkeleton::RebuildReferenceSkeletonDerivedData()
{
    for (int32 BoneIndex = 0; BoneIndex < ReferenceSkeleton.GetNumBones(); ++BoneIndex)
    {
        FReferenceBone& Bone = ReferenceSkeleton.Bones[BoneIndex];
        const int32 ParentIndex = Bone.ParentIndex;
        Bone.GlobalBindPose = (ParentIndex >= 0 && ParentIndex < BoneIndex)
            ? Bone.LocalBindPose * ReferenceSkeleton.Bones[ParentIndex].GlobalBindPose
            : Bone.LocalBindPose;
        Bone.InverseBindPose = Bone.GlobalBindPose.GetInverse();
    }
}
