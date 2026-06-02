#include "Cloth/SkeletalClothRuntime.h"

#include "Cloth/NvClothBackend.h"
#include "Core/Logging/Log.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Render/Types/VertexTypes.h"

#include <NvCloth/Cloth.h>
#include <NvCloth/Fabric.h>
#include <NvCloth/Factory.h>
#include <NvCloth/PhaseConfig.h>
#include <NvCloth/Range.h>
#include <NvCloth/Solver.h>
#include <NvClothExt/ClothFabricCooker.h>
#include <NvClothExt/ClothMeshDesc.h>
#include <foundation/PxVec3.h>
#include <foundation/PxVec4.h>

#include <algorithm>
#include <cmath>

namespace
{
    constexpr float ClothDefaultInvMass = 1.0f;
    constexpr float ClothMinDeltaTime = 1.0e-5f;
    constexpr float ClothFixedStep = 1.0f / 60.0f;
    constexpr float ClothMaxFrameDelta = 1.0f / 15.0f;
    constexpr int32 ClothMaxSubsteps = 4;
    constexpr float ClothScaleTolerance = 1.0e-4f;

    float Clamp01(float Value)
    {
        return std::max(0.0f, std::min(1.0f, Value));
    }

    physx::PxVec3 ToPxVec3(const FVector& Value)
    {
        return physx::PxVec3(Value.X, Value.Y, Value.Z);
    }

    physx::PxVec4 ToPxParticle(const FVector& Position, float InvMass)
    {
        return physx::PxVec4(Position.X, Position.Y, Position.Z, InvMass);
    }

    FVector ToFVector(const physx::PxVec4& Value)
    {
        return FVector(Value.x, Value.y, Value.z);
    }

    bool IsFinite(float Value)
    {
        return std::isfinite(Value);
    }

    bool IsFiniteVector(const FVector& Value)
    {
        return IsFinite(Value.X) && IsFinite(Value.Y) && IsFinite(Value.Z);
    }

    bool IsNearlyEqual(float A, float B, float Tolerance = ClothScaleTolerance)
    {
        return std::fabs(A - B) <= Tolerance;
    }

    bool IsUniformScale(const FVector& Scale)
    {
        return IsNearlyEqual(Scale.X, Scale.Y) && IsNearlyEqual(Scale.Y, Scale.Z);
    }

    bool IsValidClothScale(const FVector& Scale)
    {
        return IsFiniteVector(Scale) &&
            Scale.X > ClothScaleTolerance &&
            Scale.Y > ClothScaleTolerance &&
            Scale.Z > ClothScaleTolerance &&
            IsUniformScale(Scale);
    }

    bool HasScaleChanged(const FVector& A, const FVector& B)
    {
        return !IsNearlyEqual(A.X, B.X) ||
            !IsNearlyEqual(A.Y, B.Y) ||
            !IsNearlyEqual(A.Z, B.Z);
    }

    bool HasPositivePaintRadius(const FSkeletalClothData& ClothData)
    {
        for (float MaxDistance : ClothData.Paint.MaxDistanceValues)
        {
            if (MaxDistance > 0.0f)
            {
                return true;
            }
        }
        return false;
    }

    float GetPaintMaxDistance(const FSkeletalClothData& ClothData, uint32 ParticleIndex)
    {
        if (ParticleIndex < ClothData.Paint.MaxDistanceValues.size())
        {
            return std::max(0.0f, ClothData.Paint.MaxDistanceValues[ParticleIndex]);
        }
        return 0.0f;
    }
}

struct FSkeletalClothRuntime::FImpl
{
    struct FSectionRuntime
    {
        const FSkeletalClothData* SourceData = nullptr;
        uint32 LODIndex = 0;
        int32 SectionIndex = -1;
        uint32 SourceVertexCount = 0;
        uint32 SourceIndexCount = 0;
        nv::cloth::Fabric* Fabric = nullptr;
        nv::cloth::Cloth* Cloth = nullptr;
        TArray<physx::PxVec3> CookedPositions;
        TArray<float> InvMasses;
        TArray<uint32_t> CookedIndices;

        void Destroy(nv::cloth::Solver* Solver)
        {
            if (Solver && Cloth)
            {
                Solver->removeCloth(Cloth);
            }

            delete Cloth;
            Cloth = nullptr;

            if (Fabric)
            {
                Fabric->decRefCount();
                Fabric = nullptr;
            }

            SourceData = nullptr;
            CookedPositions.clear();
            InvMasses.clear();
            CookedIndices.clear();
        }
    };

    FNvClothCpuBackend Backend;
    std::unique_ptr<IClothSolver> SolverOwner;
    TArray<FSectionRuntime> Sections;
    const FSkeletalMesh* BuiltAsset = nullptr;
    float AccumulatedSimulationTime = 0.0f;
    FMatrix PreviousComponentWorldMatrix = FMatrix::Identity;
    FMatrix CurrentComponentWorldMatrix = FMatrix::Identity;
    FVector LastComponentScale = FVector::OneVector;
    bool bHasComponentTransformHistory = false;
    bool bLoggedGpuSkip = false;

    ~FImpl()
    {
        Reset();
    }

    nv::cloth::Solver* GetNativeSolver() const
    {
        FNvClothSolver* Solver = dynamic_cast<FNvClothSolver*>(SolverOwner.get());
        return Solver ? Solver->GetNativeSolver() : nullptr;
    }

    void Reset()
    {
        nv::cloth::Solver* Solver = GetNativeSolver();
        for (FSectionRuntime& Section : Sections)
        {
            Section.Destroy(Solver);
        }
        Sections.clear();
        SolverOwner.reset();
        Backend.Shutdown();
        BuiltAsset = nullptr;
        AccumulatedSimulationTime = 0.0f;
        PreviousComponentWorldMatrix = FMatrix::Identity;
        CurrentComponentWorldMatrix = FMatrix::Identity;
        LastComponentScale = FVector::OneVector;
        bHasComponentTransformHistory = false;
        bLoggedGpuSkip = false;
    }

    bool PrepareComponentTransformHistory(const FMatrix& ComponentWorldMatrix)
    {
        const FVector ComponentScale = ComponentWorldMatrix.GetScale();
        if (!IsValidClothScale(ComponentScale))
        {
            Reset();
            return false;
        }

        if (!bHasComponentTransformHistory)
        {
            PreviousComponentWorldMatrix = ComponentWorldMatrix;
            CurrentComponentWorldMatrix = ComponentWorldMatrix;
            LastComponentScale = ComponentScale;
            bHasComponentTransformHistory = true;
            AccumulatedSimulationTime = 0.0f;
            return true;
        }

        if (HasScaleChanged(ComponentScale, LastComponentScale))
        {
            Reset();
            PreviousComponentWorldMatrix = ComponentWorldMatrix;
            CurrentComponentWorldMatrix = ComponentWorldMatrix;
            LastComponentScale = ComponentScale;
            bHasComponentTransformHistory = true;
            AccumulatedSimulationTime = 0.0f;
            return true;
        }

        PreviousComponentWorldMatrix = CurrentComponentWorldMatrix;
        CurrentComponentWorldMatrix = ComponentWorldMatrix;
        LastComponentScale = ComponentScale;
        return true;
    }

    FVector TransformWorldVectorToComponentLocal(const FVector& WorldVector) const
    {
        return CurrentComponentWorldMatrix.GetInverse().TransformVector(WorldVector);
    }

    void UpdateWorldForces(const FClothWorldForceContext& ForceContext)
    {
        for (FSectionRuntime& Section : Sections)
        {
            if (!Section.Cloth || !Section.SourceData)
            {
                continue;
            }

            const FSkeletalClothData& ClothData = *Section.SourceData;
            const FVector LocalGravity = TransformWorldVectorToComponentLocal(ForceContext.WorldGravity);
            Section.Cloth->setGravity(ToPxVec3(LocalGravity * ClothData.Config.GravityScale));
        }
    }

    bool HasEnabledCloth(const FSkeletalMesh& Asset) const
    {
        for (const FSkeletalClothLODData& LODData : Asset.ClothPayload.LODs)
        {
            for (const FSkeletalClothData& ClothData : LODData.Cloths)
            {
                if (ClothData.bEnabled)
                {
                    return true;
                }
            }
        }
        return false;
    }

    bool MatchesBuiltPayload(const FSkeletalMesh& Asset) const
    {
        if (BuiltAsset != &Asset)
        {
            return false;
        }

        uint32 EnabledCount = 0;
        for (const FSkeletalClothLODData& LODData : Asset.ClothPayload.LODs)
        {
            for (const FSkeletalClothData& ClothData : LODData.Cloths)
            {
                if (!ClothData.bEnabled)
                {
                    continue;
                }

                if (EnabledCount >= Sections.size() || Sections[EnabledCount].SourceData != &ClothData)
                {
                    return false;
                }
                ++EnabledCount;
            }
        }

        return EnabledCount == Sections.size();
    }

    bool EnsureBackend()
    {
        if (!Backend.Initialize())
        {
            return false;
        }

        if (!SolverOwner)
        {
            SolverOwner = Backend.CreateSolver();
        }

        return SolverOwner && SolverOwner->IsValid() && GetNativeSolver();
    }

    bool Build(const FSkeletalMesh& Asset, const TArray<FVertexPNCTT>& SkinnedVertices)
    {
        Reset();

        if (!HasEnabledCloth(Asset))
        {
            BuiltAsset = &Asset;
            return true;
        }

        if (!EnsureBackend())
        {
            UE_LOG("[NvCloth] Failed to initialize CPU cloth runtime");
            return false;
        }

        nv::cloth::Factory* Factory = Backend.GetNativeFactory();
        nv::cloth::Solver* Solver = GetNativeSolver();
        if (!Factory || !Solver)
        {
            return false;
        }

        for (const FSkeletalClothLODData& LODData : Asset.ClothPayload.LODs)
        {
            for (const FSkeletalClothData& ClothData : LODData.Cloths)
            {
                if (!ClothData.bEnabled)
                {
                    continue;
                }

                FString Error;
                if (!Asset.ValidateClothData(ClothData, &Error))
                {
                    UE_LOG("[NvCloth] Skipping invalid cloth '%s': %s",
                        ClothData.Name.c_str(),
                        Error.c_str());
                    continue;
                }

                if (ClothData.ParticleToRenderVertex.empty() ||
                    ClothData.ClothLocalIndices.empty() ||
                    ClothData.ClothLocalIndices.size() % 3 != 0)
                {
                    continue;
                }

                if (!HasPositivePaintRadius(ClothData))
                {
                    continue;
                }

                FSectionRuntime Runtime;
                Runtime.SourceData = &ClothData;
                Runtime.LODIndex = ClothData.Binding.LODIndex;
                Runtime.SectionIndex = ClothData.Binding.SectionIndex;
                Runtime.SourceVertexCount = ClothData.Binding.SourceVertexCount;
                Runtime.SourceIndexCount = ClothData.Binding.SourceIndexCount;

				Runtime.CookedPositions.resize(ClothData.ParticleToRenderVertex.size());
				Runtime.InvMasses.resize(ClothData.ParticleToRenderVertex.size());

                bool bValidMapping = true;
				for (uint32 ParticleIndex = 0; ParticleIndex < ClothData.ParticleToRenderVertex.size(); ++ParticleIndex)
				{
					const uint32 RenderVertexIndex = ClothData.ParticleToRenderVertex[ParticleIndex];
					if (RenderVertexIndex >= SkinnedVertices.size())
					{
                        bValidMapping = false;
                        break;
					}

					Runtime.CookedPositions[ParticleIndex] = ToPxVec3(SkinnedVertices[RenderVertexIndex].Position);
					Runtime.InvMasses[ParticleIndex] = GetPaintMaxDistance(ClothData, ParticleIndex) > 0.0f
						? ClothDefaultInvMass
						: 0.0f;
				}
                if (!bValidMapping)
                {
                    UE_LOG("[NvCloth] Skipping cloth '%s': particle/render vertex mapping is out of range",
                        ClothData.Name.c_str());
                    continue;
                }

                Runtime.CookedIndices.reserve(ClothData.ClothLocalIndices.size());
                for (uint32 LocalIndex : ClothData.ClothLocalIndices)
                {
                    Runtime.CookedIndices.push_back(static_cast<uint32_t>(LocalIndex));
                }

                nv::cloth::ClothMeshDesc MeshDesc;
                MeshDesc.points.data = Runtime.CookedPositions.data();
                MeshDesc.points.count = static_cast<physx::PxU32>(Runtime.CookedPositions.size());
                MeshDesc.points.stride = sizeof(physx::PxVec3);
                MeshDesc.invMasses.data = Runtime.InvMasses.data();
                MeshDesc.invMasses.count = static_cast<physx::PxU32>(Runtime.InvMasses.size());
                MeshDesc.invMasses.stride = sizeof(float);
                MeshDesc.triangles.data = Runtime.CookedIndices.data();
                MeshDesc.triangles.count = static_cast<physx::PxU32>(Runtime.CookedIndices.size() / 3);
                MeshDesc.triangles.stride = sizeof(uint32_t) * 3;

                Runtime.Fabric = NvClothCookFabricFromMesh(
                    Factory,
                    MeshDesc,
                    physx::PxVec3(0.0f, 0.0f, -1.0f),
                    nullptr,
                    false
                );

                if (!Runtime.Fabric)
                {
                    UE_LOG("[NvCloth] Failed to cook fabric for cloth '%s'", ClothData.Name.c_str());
                    Runtime.Destroy(Solver);
                    continue;
                }

                TArray<physx::PxVec4> InitialParticles;
                InitialParticles.resize(Runtime.CookedPositions.size());
                for (uint32 ParticleIndex = 0; ParticleIndex < InitialParticles.size(); ++ParticleIndex)
                {
                    const physx::PxVec3& Position = Runtime.CookedPositions[ParticleIndex];
                    InitialParticles[ParticleIndex] = physx::PxVec4(
                        Position.x,
                        Position.y,
                        Position.z,
                        Runtime.InvMasses[ParticleIndex]
                    );
                }

                Runtime.Cloth = Factory->createCloth(
                    nv::cloth::Range<const physx::PxVec4>(InitialParticles.data(), InitialParticles.data() + InitialParticles.size()),
                    *Runtime.Fabric
                );

                if (!Runtime.Cloth)
                {
                    UE_LOG("[NvCloth] Failed to create cloth '%s'", ClothData.Name.c_str());
                    Runtime.Destroy(Solver);
                    continue;
                }

                Runtime.Cloth->setGravity(physx::PxVec3(
                    0.0f,
                    0.0f,
                    -9.81f * ClothData.Config.GravityScale
                ));
                Runtime.Cloth->setDamping(physx::PxVec3(
                    Clamp01(ClothData.Config.Damping),
                    Clamp01(ClothData.Config.Damping),
                    Clamp01(ClothData.Config.Damping)
                ));
                Runtime.Cloth->setSolverFrequency(std::max(1.0f, ClothData.Config.SolverFrequency));

                TArray<nv::cloth::PhaseConfig> PhaseConfigs;
                PhaseConfigs.reserve(Runtime.Fabric->getNumPhases());
                for (uint32 PhaseIndex = 0; PhaseIndex < Runtime.Fabric->getNumPhases(); ++PhaseIndex)
                {
                    nv::cloth::PhaseConfig Phase(static_cast<uint16_t>(PhaseIndex));
                    Phase.mStiffness = Clamp01(ClothData.Config.Stiffness);
                    PhaseConfigs.push_back(Phase);
                }
                if (!PhaseConfigs.empty())
                {
                    Runtime.Cloth->setPhaseConfig(
                        nv::cloth::Range<const nv::cloth::PhaseConfig>(PhaseConfigs.data(), PhaseConfigs.data() + PhaseConfigs.size())
                    );
                }

                Solver->addCloth(Runtime.Cloth);
                Sections.push_back(std::move(Runtime));
            }
        }

        BuiltAsset = &Asset;
        return true;
    }

    void UpdateAnimatedPinsAndConstraints(FSectionRuntime& Section, const TArray<FVertexPNCTT>& SkinnedVertices)
    {
        const FSkeletalClothData& ClothData = *Section.SourceData;
        nv::cloth::Range<physx::PxVec4> CurrentParticles = Section.Cloth->getCurrentParticles();
        nv::cloth::Range<physx::PxVec4> PreviousParticles = Section.Cloth->getPreviousParticles();
        nv::cloth::Range<physx::PxVec4> MotionConstraints = Section.Cloth->getMotionConstraints();

        const uint32 ParticleCount = static_cast<uint32>(ClothData.ParticleToRenderVertex.size());
        for (uint32 ParticleIndex = 0; ParticleIndex < ParticleCount; ++ParticleIndex)
        {
            const uint32 RenderVertexIndex = ClothData.ParticleToRenderVertex[ParticleIndex];
            if (RenderVertexIndex >= SkinnedVertices.size())
            {
                continue;
            }

            const FVector& AnimatedPosition = SkinnedVertices[RenderVertexIndex].Position;
            const float MaxDistance = GetPaintMaxDistance(ClothData, ParticleIndex);

            if (ParticleIndex < CurrentParticles.size())
            {
                CurrentParticles[ParticleIndex].w = MaxDistance > 0.0f ? ClothDefaultInvMass : 0.0f;
                if (MaxDistance <= 0.0f)
                {
                    CurrentParticles[ParticleIndex] = ToPxParticle(AnimatedPosition, 0.0f);
                }
            }

            if (ParticleIndex < PreviousParticles.size() && MaxDistance <= 0.0f)
            {
                PreviousParticles[ParticleIndex] = ToPxParticle(AnimatedPosition, 0.0f);
            }

            if (ParticleIndex < MotionConstraints.size())
            {
                MotionConstraints[ParticleIndex] = ToPxParticle(AnimatedPosition, MaxDistance);
            }
        }
    }

    bool WriteBack(FSectionRuntime& Section, TArray<FVertexPNCTT>& InOutSkinnedVertices)
    {
        const FSkeletalClothData& ClothData = *Section.SourceData;
        nv::cloth::Range<const physx::PxVec4> CurrentParticles =
            static_cast<const nv::cloth::Cloth*>(Section.Cloth)->getCurrentParticles();

        bool bModified = false;
        for (uint32 ParticleIndex = 0; ParticleIndex < ClothData.ParticleToRenderVertex.size(); ++ParticleIndex)
        {
            const uint32 RenderVertexIndex = ClothData.ParticleToRenderVertex[ParticleIndex];
            if (ParticleIndex >= CurrentParticles.size() || RenderVertexIndex >= InOutSkinnedVertices.size())
            {
                continue;
            }

            InOutSkinnedVertices[RenderVertexIndex].Position = ToFVector(CurrentParticles[ParticleIndex]);
            bModified = true;
        }

        if (bModified)
        {
            RecomputeNormalsAndTangents(ClothData, InOutSkinnedVertices);
        }

        return bModified;
    }

    void RecomputeNormalsAndTangents(const FSkeletalClothData& ClothData, TArray<FVertexPNCTT>& InOutSkinnedVertices)
    {
        const uint32 ParticleCount = static_cast<uint32>(ClothData.ParticleToRenderVertex.size());
        TArray<FVector> NormalSums(ParticleCount, FVector::ZeroVector);
        TArray<FVector> TangentSums(ParticleCount, FVector::ZeroVector);

        for (uint32 Index = 0; Index + 2 < ClothData.ClothLocalIndices.size(); Index += 3)
        {
            const uint32 I0 = ClothData.ClothLocalIndices[Index + 0];
            const uint32 I1 = ClothData.ClothLocalIndices[Index + 1];
            const uint32 I2 = ClothData.ClothLocalIndices[Index + 2];
            if (I0 >= ParticleCount || I1 >= ParticleCount || I2 >= ParticleCount)
            {
                continue;
            }

            const uint32 V0 = ClothData.ParticleToRenderVertex[I0];
            const uint32 V1 = ClothData.ParticleToRenderVertex[I1];
            const uint32 V2 = ClothData.ParticleToRenderVertex[I2];
            if (V0 >= InOutSkinnedVertices.size() || V1 >= InOutSkinnedVertices.size() || V2 >= InOutSkinnedVertices.size())
            {
                continue;
            }

            const FVector& P0 = InOutSkinnedVertices[V0].Position;
            const FVector& P1 = InOutSkinnedVertices[V1].Position;
            const FVector& P2 = InOutSkinnedVertices[V2].Position;
            FVector FaceNormal = (P1 - P0).Cross(P2 - P0);
            if (!FaceNormal.IsNearlyZero())
            {
                FaceNormal.Normalize();
                NormalSums[I0] += FaceNormal;
                NormalSums[I1] += FaceNormal;
                NormalSums[I2] += FaceNormal;
            }

            const FVector2& UV0 = InOutSkinnedVertices[V0].UV;
            const FVector2& UV1 = InOutSkinnedVertices[V1].UV;
            const FVector2& UV2 = InOutSkinnedVertices[V2].UV;
            const float DU1 = UV1.X - UV0.X;
            const float DV1 = UV1.Y - UV0.Y;
            const float DU2 = UV2.X - UV0.X;
            const float DV2 = UV2.Y - UV0.Y;
            const float Denom = DU1 * DV2 - DV1 * DU2;
            if (std::fabs(Denom) > 1.0e-6f)
            {
                FVector Tangent = ((P1 - P0) * DV2 - (P2 - P0) * DV1) / Denom;
                if (!Tangent.IsNearlyZero())
                {
                    Tangent.Normalize();
                    TangentSums[I0] += Tangent;
                    TangentSums[I1] += Tangent;
                    TangentSums[I2] += Tangent;
                }
            }
        }

        for (uint32 ParticleIndex = 0; ParticleIndex < ParticleCount; ++ParticleIndex)
        {
            const uint32 RenderVertexIndex = ClothData.ParticleToRenderVertex[ParticleIndex];
            if (RenderVertexIndex >= InOutSkinnedVertices.size())
            {
                continue;
            }

            FVertexPNCTT& Vertex = InOutSkinnedVertices[RenderVertexIndex];
            if (!NormalSums[ParticleIndex].IsNearlyZero())
            {
                NormalSums[ParticleIndex].Normalize();
                Vertex.Normal = NormalSums[ParticleIndex];
            }

            if (!TangentSums[ParticleIndex].IsNearlyZero())
            {
                TangentSums[ParticleIndex].Normalize();
                Vertex.Tangent = FVector4(TangentSums[ParticleIndex], Vertex.Tangent.W);
            }
        }
    }

    bool Tick(
        const FSkeletalMesh& Asset,
        float DeltaTime,
        TArray<FVertexPNCTT>& InOutSkinnedVertices,
        const FMatrix& ComponentWorldMatrix,
        const FClothWorldForceContext& ForceContext)
    {
        if (!PrepareComponentTransformHistory(ComponentWorldMatrix))
        {
            return false;
        }

        if (!MatchesBuiltPayload(Asset) && !Build(Asset, InOutSkinnedVertices))
        {
            return false;
        }

        nv::cloth::Solver* Solver = GetNativeSolver();
        if (!Solver || Sections.empty() || DeltaTime <= ClothMinDeltaTime)
        {
            return false;
        }

        bool bHasSimulatedParticles = false;
        for (FSectionRuntime& Section : Sections)
        {
            if (!Section.Cloth || !Section.SourceData)
            {
                continue;
            }

            UpdateAnimatedPinsAndConstraints(Section, InOutSkinnedVertices);
            bHasSimulatedParticles = bHasSimulatedParticles || HasPositivePaintRadius(*Section.SourceData);
        }

        if (bHasSimulatedParticles)
        {
            UpdateWorldForces(ForceContext);
            AccumulatedSimulationTime += std::min(DeltaTime, ClothMaxFrameDelta);
            int32 SubstepCount = 0;
            while (AccumulatedSimulationTime >= ClothFixedStep && SubstepCount < ClothMaxSubsteps)
            {
                if (Solver->beginSimulation(ClothFixedStep))
                {
                    const int ChunkCount = Solver->getSimulationChunkCount();
                    for (int ChunkIndex = 0; ChunkIndex < ChunkCount; ++ChunkIndex)
                    {
                        Solver->simulateChunk(ChunkIndex);
                    }
                    Solver->endSimulation();
                }
                AccumulatedSimulationTime -= ClothFixedStep;
                ++SubstepCount;
            }

            if (SubstepCount >= ClothMaxSubsteps && AccumulatedSimulationTime >= ClothFixedStep)
            {
                AccumulatedSimulationTime = 0.0f;
            }
        }

        bool bModified = false;
        for (FSectionRuntime& Section : Sections)
        {
            if (Section.Cloth && Section.SourceData)
            {
                bModified = WriteBack(Section, InOutSkinnedVertices) || bModified;
            }
        }

        return bModified;
    }
};

FSkeletalClothRuntime::FSkeletalClothRuntime()
    : Impl(std::make_unique<FImpl>())
{
}

FSkeletalClothRuntime::~FSkeletalClothRuntime() = default;

void FSkeletalClothRuntime::Reset()
{
    Impl->Reset();
}

bool FSkeletalClothRuntime::Tick(
    const FSkeletalMesh& Asset,
    float DeltaTime,
    TArray<FVertexPNCTT>& InOutSkinnedVertices,
    const FMatrix& ComponentWorldMatrix,
    const FClothWorldForceContext& ForceContext)
{
    return Impl->Tick(Asset, DeltaTime, InOutSkinnedVertices, ComponentWorldMatrix, ForceContext);
}

bool FSkeletalClothRuntime::IsActive() const
{
    return Impl && !Impl->Sections.empty();
}
