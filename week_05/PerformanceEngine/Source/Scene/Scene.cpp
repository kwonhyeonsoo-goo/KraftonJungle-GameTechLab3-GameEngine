#include "Scene.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <string>

#include "Core/Core.h"
#include "Graphics/D3D11/D3D11RHI.h"
#include "Math/MathUtility.h"
#include "StaticMesh/StaticMesh.h"
#include "Scene/SceneGraph.h"
#include "FileSystem/FileSystem.h"
#include "Visibility/VisibilitySystem.h"

namespace
{
	struct FPendingPrimitive
	{
		int32 Id = 0;
		FString Type;
		FString MeshAssetPath;
		FVector Location = FVector::ZeroVector;
		FVector Rotation = FVector::ZeroVector;
		FVector Scale = FVector::OneVector;
	};

	std::string Trim(const std::string& InValue)
	{
		const auto Begin = std::find_if_not(InValue.begin(), InValue.end(), [](unsigned char Ch) { return std::isspace(Ch) != 0; });
		const auto End = std::find_if_not(InValue.rbegin(), InValue.rend(), [](unsigned char Ch) { return std::isspace(Ch) != 0; }).base();
		if (Begin >= End)
		{
			return {};
		}

		return std::string(Begin, End);
	}

	bool TryParseFloatArray1(const std::string& InLine, float& OutValue)
	{
		const size_t OpenBracket = InLine.find('[');
		const size_t CloseBracket = InLine.find(']');
		if (OpenBracket == std::string::npos || CloseBracket == std::string::npos || CloseBracket <= OpenBracket)
		{
			return false;
		}

		return ::sscanf_s(InLine.c_str() + OpenBracket + 1, "%f", &OutValue) == 1;
	}

	bool TryParseFloatArray3(const std::string& InLine, FVector& OutValue)
	{
		const size_t OpenBracket = InLine.find('[');
		const size_t CloseBracket = InLine.find(']');
		if (OpenBracket == std::string::npos || CloseBracket == std::string::npos || CloseBracket <= OpenBracket)
		{
			return false;
		}

		return ::sscanf_s(
			InLine.c_str() + OpenBracket + 1,
			"%f, %f, %f",
			&OutValue.X,
			&OutValue.Y,
			&OutValue.Z) == 3;
	}

	bool TryParseQuotedStringValue(const std::string& InLine, FString& OutValue)
	{
		const size_t Colon = InLine.find(':');
		if (Colon == std::string::npos)
		{
			return false;
		}

		const size_t FirstQuote = InLine.find('"', Colon);
		if (FirstQuote == std::string::npos)
		{
			return false;
		}

		const size_t SecondQuote = InLine.find('"', FirstQuote + 1);
		if (SecondQuote == std::string::npos || SecondQuote <= FirstQuote)
		{
			return false;
		}

		OutValue = InLine.substr(FirstQuote + 1, SecondQuote - FirstQuote - 1);
		return true;
	}

	bool TryParsePrimitiveId(const std::string& InLine, int32& OutId)
	{
		int ParsedId = 0;
		if (::sscanf_s(InLine.c_str(), "\"%d\" : {", &ParsedId) != 1)
		{
			return false;
		}

		OutId = ParsedId;
		return true;
	}

	bool LooksLikeRadians(const FVector& InRotation)
	{
		const float MaxComponent = std::max({ std::fabs(InRotation.X), std::fabs(InRotation.Y), std::fabs(InRotation.Z) });
		return MaxComponent <= 6.5f;
	}

	FVector ConvertRadiansToDegrees(const FVector& InRadians)
	{
		return FVector(
			FMath::RadiansToDegrees(InRadians.X),
			FMath::RadiansToDegrees(InRadians.Y),
			FMath::RadiansToDegrees(InRadians.Z));
	}

	FQuat MakeSceneRotation(const FVector& InRotation)
	{
		const FVector EulerDegrees = LooksLikeRadians(InRotation) ? ConvertRadiansToDegrees(InRotation) : InRotation;
		return FQuat::MakeFromEuler(EulerDegrees);
	}

	FTransform BuildSceneTransform(const FVector& InLocation, const FVector& InRotation, const FVector& InScale)
	{
		return FTransform(MakeSceneRotation(InRotation), InLocation, InScale);
	}

	void ExpandBounds(FVector& InOutMin, FVector& InOutMax, bool& bInOutHasBounds, const FVector& InPoint)
	{
		if (!bInOutHasBounds)
		{
			InOutMin = InPoint;
			InOutMax = InPoint;
			bInOutHasBounds = true;
			return;
		}

		InOutMin = FVector::Min(InOutMin, InPoint);
		InOutMax = FVector::Max(InOutMax, InPoint);
	}

	void ExpandBoundsWithTransformedAabb(
		const FVector& InLocalMin,
		const FVector& InLocalMax,
		const FTransform& InTransform,
		FVector& InOutMin,
		FVector& InOutMax,
		bool& bInOutHasBounds)
	{
		const FVector Corners[8] =
		{
			FVector(InLocalMin.X, InLocalMin.Y, InLocalMin.Z),
			FVector(InLocalMax.X, InLocalMin.Y, InLocalMin.Z),
			FVector(InLocalMin.X, InLocalMax.Y, InLocalMin.Z),
			FVector(InLocalMax.X, InLocalMax.Y, InLocalMin.Z),
			FVector(InLocalMin.X, InLocalMin.Y, InLocalMax.Z),
			FVector(InLocalMax.X, InLocalMin.Y, InLocalMax.Z),
			FVector(InLocalMin.X, InLocalMax.Y, InLocalMax.Z),
			FVector(InLocalMax.X, InLocalMax.Y, InLocalMax.Z),
		};

		for (const FVector& Corner : Corners)
		{
			ExpandBounds(InOutMin, InOutMax, bInOutHasBounds, InTransform.TransformPosition(Corner));
		}
	}

	std::wstring ResolveAssetPath(const std::wstring& InScenePath, const FString& InRelativeAssetPath)
	{
		const char* RelativeAssetPathStr = InRelativeAssetPath.c_str();
		int size_needed = MultiByteToWideChar(CP_UTF8, 0, RelativeAssetPathStr, -1, NULL, 0);
		std::wstring RelativeAssetPathW(size_needed, 0);
		MultiByteToWideChar(CP_UTF8, 0, RelativeAssetPathStr, -1, &RelativeAssetPathW[0], 256);
		if (!RelativeAssetPathW.empty() && RelativeAssetPathW.back() == L'\0') RelativeAssetPathW.pop_back();

		const std::wstring AssetPath = RelativeAssetPathW;
		if ( FFileSystem::IsAbsolutePath(AssetPath) && FFileSystem::SafeExists(AssetPath))
		{
			return AssetPath;
		}

		std::wstring Cursor = FFileSystem::GetParentPath(InScenePath);
		for (int Depth = 0; Depth < 6; ++Depth)
		{
			std::wstring Combined = FFileSystem::JoinPath(Cursor, AssetPath);
			std::wstring Candidate = FFileSystem::NormalizePath(Combined);

			if (FFileSystem::SafeExists(Candidate))
			{
				return Candidate;
			}

			if (!FFileSystem::HasParentPath( Cursor))
			{
				break;
			}

			Cursor = FFileSystem::GetParentPath(Cursor);
		}

		return FFileSystem::JoinPath(InScenePath, AssetPath);
	}

}

bool FScene::LoadFromFile(ID3D11Device* InDevice, ID3D11DeviceContext* InDeviceContext, const std::wstring& InSceneFilePath)
{
	Release();

	if (InDevice == nullptr || InDeviceContext == nullptr || InSceneFilePath.empty())
	{
		return false;
	}

	const std::wstring SceneFilePath = FFileSystem::GetAbsolutePath(InSceneFilePath);
	FILE* SceneFile = nullptr;
	_wfopen_s(&SceneFile, SceneFilePath.c_str(), L"rb"); // 읽기 모드

	if (!SceneFile)
	{
		return false;
	}

	bool bInCameraBlock = false;
	bool bInPrimitivesBlock = false;
	bool bInPrimitiveBlock = false;
	bool bHasSceneBounds = false;

	FPendingPrimitive PendingPrimitive;
	std::string Line;
	while (FFileSystem::GetLineFromFile(SceneFile, Line))
	{
		const std::string TrimmedLine = Trim(Line);
		if (TrimmedLine.empty())
		{
			continue;
		}

		if (!bInPrimitiveBlock && TrimmedLine.starts_with("\"PerspectiveCamera\""))
		{
			bInCameraBlock = true;
			continue;
		}

		if (!bInPrimitiveBlock && TrimmedLine.starts_with("\"Primitives\""))
		{
			bInPrimitivesBlock = true;
			continue;
		}

		if (bInCameraBlock)
		{
			if (TrimmedLine == "}," || TrimmedLine == "}")
			{
				bInCameraBlock = false;
				continue;
			}

			float ScalarValue = 0.0f;
			FVector VectorValue = FVector::ZeroVector;
			if (TrimmedLine.starts_with("\"FOV\"") && TryParseFloatArray1(TrimmedLine, ScalarValue))
			{
				InitialCamera.FovDegrees = ScalarValue;
			}
			else if (TrimmedLine.starts_with("\"NearClip\"") && TryParseFloatArray1(TrimmedLine, ScalarValue))
			{
				InitialCamera.NearClip = ScalarValue;
			}
			else if (TrimmedLine.starts_with("\"FarClip\"") && TryParseFloatArray1(TrimmedLine, ScalarValue))
			{
				InitialCamera.FarClip = ScalarValue;
			}
			else if (TrimmedLine.starts_with("\"Location\"") && TryParseFloatArray3(TrimmedLine, VectorValue))
			{
				RawCameraLocation = VectorValue;
			}
			else if (TrimmedLine.starts_with("\"Rotation\"") && TryParseFloatArray3(TrimmedLine, VectorValue))
			{
				RawCameraRotation = VectorValue;
			}

			continue;
		}

		if (bInPrimitiveBlock)
		{
			if (TrimmedLine == "}," || TrimmedLine == "}")
			{
				bInPrimitiveBlock = false;

				if (PendingPrimitive.Type != "StaticMeshComp" || PendingPrimitive.MeshAssetPath.empty())
				{
					continue;
				}

				const std::wstring MeshPath = ResolveAssetPath(SceneFilePath, PendingPrimitive.MeshAssetPath);
				const FString MeshCacheKey = FStaticMeshManager::BuildAssetKey(MeshPath);

				std::shared_ptr<FStaticMesh> SharedMesh = MeshManager.LoadStaticMesh(InDevice, InDeviceContext, MeshPath);

				if (!SharedMesh || !SharedMesh->IsValid())
				{
					continue;
				}

				const FTransform WorldTransform = BuildSceneTransform(PendingPrimitive.Location, PendingPrimitive.Rotation, PendingPrimitive.Scale);

				FScenePrimitiveColdData ColdData;
				ColdData.MeshAssetPath = MeshCacheKey;
				ColdData.StaticMeshOwner = SharedMesh;

				FScenePrimitiveRuntimeData RuntimeData;
				RuntimeData.PrimitiveId = PendingPrimitive.Id;
				RuntimeData.WorldMatrix = WorldTransform.ToMatrixWithScale();
				RuntimeData.InverseWorldMatrix = WorldTransform.ToInverseMatrixWithScale();
				RuntimeData.StaticMesh = SharedMesh.get();

				RuntimeData.SetWorldMatrix(WorldTransform.ToMatrixWithScale());
				RuntimeData.GetComponentToWorld();
		
				ExpandBounds(SceneBoundsMin, SceneBoundsMax, bHasSceneBounds, RuntimeData.WorldBounds.Min);
				ExpandBounds(SceneBoundsMin, SceneBoundsMax, bHasSceneBounds, RuntimeData.WorldBounds.Max);

				PrimitiveColdData.push_back(std::move(ColdData));
				PrimitiveRuntimeData.push_back(std::move(RuntimeData));
				continue;
			}

			FVector VectorValue = FVector::ZeroVector;
			FString StringValue;
			if (TrimmedLine.starts_with("\"Location\"") && TryParseFloatArray3(TrimmedLine, VectorValue))
			{
				PendingPrimitive.Location = VectorValue;
			}
			else if (TrimmedLine.starts_with("\"Rotation\"") && TryParseFloatArray3(TrimmedLine, VectorValue))
			{
				PendingPrimitive.Rotation = VectorValue;
			}
			else if (TrimmedLine.starts_with("\"Scale\"") && TryParseFloatArray3(TrimmedLine, VectorValue))
			{
				PendingPrimitive.Scale = VectorValue;
			}
			else if (TrimmedLine.starts_with("\"ObjStaticMeshAsset\"") && TryParseQuotedStringValue(TrimmedLine, StringValue))
			{
				PendingPrimitive.MeshAssetPath = StringValue;
			}
			else if (TrimmedLine.starts_with("\"Type\"") && TryParseQuotedStringValue(TrimmedLine, StringValue))
			{
				PendingPrimitive.Type = StringValue;
			}

			continue;
		}

		if (bInPrimitivesBlock)
		{
			if (TrimmedLine == "}" || TrimmedLine == "},")
			{
				bInPrimitivesBlock = false;
				continue;
			}

			int32 PrimitiveId = 0;
			if (TryParsePrimitiveId(TrimmedLine, PrimitiveId))
			{
				PendingPrimitive = {};
				PendingPrimitive.Id = PrimitiveId;
				bInPrimitiveBlock = true;
			}
		}
	}

	if (PrimitiveRuntimeData.empty())
	{
		Release();
		return false;
	}

	InitialCamera.Transform = FTransform(MakeSceneRotation(RawCameraRotation), RawCameraLocation, FVector::OneVector);
	if (!bHasSceneBounds)
	{
		SceneBoundsMin = FVector::ZeroVector;
		SceneBoundsMax = FVector::ZeroVector;
	}

	return true;
}

void FScene::Release()
{
	PrimitiveRuntimeData.clear();
	PrimitiveColdData.clear();
	MeshManager.Release();
	InitialCamera = FSceneCameraInitData();
	RawCameraLocation = FVector::ZeroVector;
	RawCameraRotation = FVector::ZeroVector;
	SceneBoundsMin = FVector::ZeroVector;
	SceneBoundsMax = FVector::ZeroVector;
}

void FScene::Tick()
{
	for (auto& RuntimeData : PrimitiveRuntimeData)
	{
		RuntimeData.Tick();
	}
}

void FScene::Spawn(FCore* InCore)
{
	ID3D11Device* Device = InCore->GetRHI()->GetDevice();
	ID3D11DeviceContext* DeviceContext = InCore->GetRHI()->GetDeviceContext();

	const FTransform WorldTransform = BuildSceneTransform(FVector::ZeroVector, FVector::ZeroVector, FVector::OneVector);

	FScenePrimitiveRuntimeData RuntimeData;
	RuntimeData.PrimitiveId = PrimitiveRuntimeData.size();
	RuntimeData.WorldMatrix = FMatrix::Identity;
	RuntimeData.InverseWorldMatrix = RuntimeData.WorldMatrix.GetInverse();

	RuntimeData.SetWorldMatrix(WorldTransform.ToMatrixWithScale());

	bool bHasSceneBounds = false;

	ExpandBounds(SceneBoundsMin, SceneBoundsMax, bHasSceneBounds, RuntimeData.WorldBounds.Min);
	ExpandBounds(SceneBoundsMin, SceneBoundsMax, bHasSceneBounds, RuntimeData.WorldBounds.Max);

	const std::wstring SceneFilePath = FFileSystem::GetAbsolutePath(L"Data/Scene/Clear.scene");
	const std::wstring MeshPath = ResolveAssetPath(SceneFilePath, "Data/apple_mid.obj");

	std::shared_ptr<FStaticMesh> SharedMesh = MeshManager.LoadStaticMesh(Device, DeviceContext, MeshPath);
	RuntimeData.StaticMesh = SharedMesh.get();

	PrimitiveRuntimeData.push_back(std::move(RuntimeData));

	InCore->GetSceneGraph()->Build(*this);
	InCore->GetVisibilitySystem()->BuildBVH(*this);
}

const FScenePrimitiveRuntimeData* FScene::GetPrimitiveRuntimeDataById(int32 PrimitiveId) const
{
	const FScenePrimitiveRuntimeData* Data = &PrimitiveRuntimeData[PrimitiveId];

	if (Data)
	{
		return Data;
	}
	return nullptr;
}
