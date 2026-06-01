#include "MeshEditorWidget.h"

#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

#include "Mesh/Skeletal/SkeletalMesh.h"
#include "Mesh/Skeletal/SkeletalMeshAsset.h"
#include "Mesh/MeshManager.h"
#include "Runtime/Engine.h"
#include "Component/Debug/PhysicsAssetPreviewComponent.h"
#include "Component/Primitive/SkeletalMeshComponent.h"
#include "Component/Light/DirectionalLightComponent.h"
#include "Collision/Ray/RayUtils.h"
#include "Viewport/Viewport.h"
#include "GameFramework/World.h"
#include "GameFramework/Light/DirectionalLightActor.h"
#include "GameFramework/Actor/StaticMeshActor.h"
#include "Settings/EditorSettings.h"
#include "UI/Toolbar/ViewportToolbar.h"
#include "Slate/SlateApplication.h"
#include "Render/Shader/ShaderManager.h"
#include "Animation/Sequence/AnimSequence.h"
#include "Animation/Montage/AnimMontage.h"
#include "Animation/AnimInstance.h"
#include "Animation/Instance/AnimSingleNodeInstance.h"
#include "Animation/AnimationManager.h"
#include "Animation/Sequence/AnimDataModel.h"
#include "Animation/Skeleton/Skeleton.h"
#include "Animation/Skeleton/SkeletonManager.h"
#include "Asset/AssetPackage.h"
#include "Asset/AssetRegistry.h"
#include "Editor/Subsystem/AssetFactory.h"
#include "Physics/PhysicsAsset.h"
#include "Physics/PhysicsAssetManager.h"
#include "UI/Asset/Animation/AnimationTransportBar.h"
#include "UI/Asset/Animation/AnimationTimelinePanel.h"
#include "UI/Asset/Animation/AnimSequencePropertyPanel.h"
#include "UI/Asset/Animation/AnimMontagePropertyPanel.h"
#include "UI/Util/EditorFileUtils.h"
#include "Editor/UI/Util/EditorTextureManager.h"
#include "Editor/EditorEngine.h"
#include "Render/Types/MinimalViewInfo.h"
#include "Render/Types/ViewTypes.h"

#include "Platform/Paths.h"
#include "Object/Object.h"

#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <utility>

// Paths.h가 끌어오는 Windows.h는 GetCurrentTime을 GetTickCount로 치환한다.
#ifdef GetCurrentTime
#undef GetCurrentTime
#endif

namespace
{
	constexpr float MeshEditorTreeIndentSpacing = 10.0f;

	bool ProjectWorldToViewport(
		const FVector& WorldPosition,
		const FMinimalViewInfo& POV,
		const ImVec2& ViewportPos,
		const ImVec2& ViewportSize,
		ImVec2& OutScreenPosition)
	{
		if (ViewportSize.x <= 1.0f || ViewportSize.y <= 1.0f)
		{
			return false;
		}

		FMatrix ViewProj = POV.CalculateViewProjectionMatrix();
		const float ClipX = WorldPosition.X * ViewProj.M[0][0] + WorldPosition.Y * ViewProj.M[1][0] + WorldPosition.Z * ViewProj.M[2][0] + ViewProj.M[3][0];
		const float ClipY = WorldPosition.X * ViewProj.M[0][1] + WorldPosition.Y * ViewProj.M[1][1] + WorldPosition.Z * ViewProj.M[2][1] + ViewProj.M[3][1];
		const float ClipW = WorldPosition.X * ViewProj.M[0][3] + WorldPosition.Y * ViewProj.M[1][3] + WorldPosition.Z * ViewProj.M[2][3] + ViewProj.M[3][3];
		if (std::abs(ClipW) <= 1.0e-4f)
		{
			return false;
		}

		const float NdcX = ClipX / ClipW;
		const float NdcY = ClipY / ClipW;
		if (NdcX < -1.05f || NdcX > 1.05f || NdcY < -1.05f || NdcY > 1.05f)
		{
			return false;
		}

		OutScreenPosition.x = ViewportPos.x + (NdcX * 0.5f + 0.5f) * ViewportSize.x;
		OutScreenPosition.y = ViewportPos.y + (0.5f - NdcY * 0.5f) * ViewportSize.y;
		return true;
	}

	float ProjectWorldRadiusToPixels(
		const FVector& WorldPosition,
		float WorldRadius,
		const FMinimalViewInfo& POV,
		const ImVec2& ViewportPos,
		const ImVec2& ViewportSize)
	{
		if (WorldRadius <= 0.0f)
		{
			return 0.0f;
		}

		ImVec2 Center;
		if (!ProjectWorldToViewport(WorldPosition, POV, ViewportPos, ViewportSize, Center))
		{
			return 0.0f;
		}

		const FVector Axes[3] =
		{
			FVector(WorldRadius, 0.0f, 0.0f),
			FVector(0.0f, WorldRadius, 0.0f),
			FVector(0.0f, 0.0f, WorldRadius),
		};

		float PixelRadius = 0.0f;
		for (const FVector& Axis : Axes)
		{
			ImVec2 Projected;
			if (ProjectWorldToViewport(WorldPosition + Axis, POV, ViewportPos, ViewportSize, Projected))
			{
				const float DX = Projected.x - Center.x;
				const float DY = Projected.y - Center.y;
				PixelRadius = std::max(PixelRadius, std::sqrt(DX * DX + DY * DY));
			}
		}
		return PixelRadius;
	}

	ID3D11ShaderResourceView* LoadTabIcon(const wchar_t* FileName)
	{
		const FString Path = FPaths::ToUtf8(
			FPaths::Combine(FPaths::AssetDir(), L"Editor/ToolIcons/", FileName));
		return FEditorTextureManager::Get().GetOrLoadIcon(Path);
	}

	FString FormatMeshStatCount(size_t Value)
	{
		FString Result = std::to_string(Value);
		for (int32 InsertPos = static_cast<int32>(Result.length()) - 3; InsertPos > 0; InsertPos -= 3)
		{
			Result.insert(static_cast<size_t>(InsertPos), ",");
		}
		return Result;
	}

	FString FormatMeshStatSeconds(double Seconds)
	{
		char Buffer[64] = {};
		std::snprintf(Buffer, sizeof(Buffer), "%.3f sec", Seconds);
		return FString(Buffer);
	}

	bool IsSameSkeletonBindingForAnimationList(const FSkeletonBinding& A, const FSkeletonBinding& B)
	{
		return A.SkeletonPath == B.SkeletonPath
			&& A.SkeletonAssetGuid == B.SkeletonAssetGuid
			&& A.CompatibilitySignature == B.CompatibilitySignature;
	}

	TMap<FString, double> GMeshImportDurationsByAssetPath;

	double GetRecordedImportDurationSeconds(const USkeletalMesh* Mesh)
	{
		if (!Mesh)
		{
			return -1.0;
		}

		const FString& AssetPath = Mesh->GetAssetPathFileName();
		if (AssetPath.empty() || AssetPath == "None")
		{
			return -1.0;
		}

		auto It = GMeshImportDurationsByAssetPath.find(AssetPath);
		return It != GMeshImportDurationsByAssetPath.end() ? It->second : -1.0;
	}

	USkeleton* GetEditingSkeleton(USkeletalMesh* Mesh)
	{
		if (!Mesh)
		{
			return nullptr;
		}

		if (USkeleton* Skeleton = Mesh->GetSkeleton())
		{
			return Skeleton;
		}

		const FSkeletonBinding& Binding = Mesh->GetSkeletonBinding();
		if (!Binding.HasSkeletonPath())
		{
			return nullptr;
		}

		USkeleton* Skeleton = FSkeletonManager::Get().LoadSkeleton(Binding.SkeletonPath);
		if (Skeleton)
		{
			Mesh->SetSkeleton(Skeleton);
		}
		return Skeleton;
	}

	USkeletalMesh* FindPreviewMeshForPhysicsAsset(UPhysicsAsset* PhysicsAsset, ID3D11Device* Device)
	{
		if (!PhysicsAsset || !Device)
		{
			return nullptr;
		}

		const FString PhysicsAssetPath = PhysicsAsset->GetAssetPathFileName();
		if (!PhysicsAssetPath.empty() && PhysicsAssetPath != "None")
		{
			FAssetImportMetadata Metadata;
			if (FAssetPackage::ReadMetadata(PhysicsAssetPath, EAssetPackageType::PhysicsAsset, Metadata) &&
				!Metadata.SourcePath.empty() &&
				FMeshManager::IsSkeletalMeshPackage(Metadata.SourcePath))
			{
				if (USkeletalMesh* Mesh = FMeshManager::LoadSkeletalMesh(Metadata.SourcePath, Device))
				{
					return Mesh;
				}
			}
		}

		const TArray<FAssetListItem> Meshes = FAssetRegistry::ListMeshesForSkeleton(PhysicsAsset->GetSkeletonBinding(), true);
		for (const FAssetListItem& Item : Meshes)
		{
			if (USkeletalMesh* Mesh = FMeshManager::LoadSkeletalMesh(Item.FullPath, Device))
			{
				return Mesh;
			}
		}

		return nullptr;
	}

	FString MakePhysicsAssetNameForMesh(const USkeletalMesh* SkeletalMesh)
	{
		if (!SkeletalMesh)
		{
			return "NewPhysicsAsset";
		}

		const FString MeshPath = SkeletalMesh->GetAssetPathFileName();
		if (MeshPath.empty() || MeshPath == "None")
		{
			return "NewPhysicsAsset";
		}

		std::filesystem::path Path(FPaths::ToWide(MeshPath));
		return FPaths::ToUtf8(Path.stem().wstring()) + "_PhysicsAsset";
	}

	FString MakePhysicsAssetDirectoryForMesh(const USkeletalMesh* SkeletalMesh)
	{
		if (!SkeletalMesh)
		{
			return FPaths::ToUtf8((std::filesystem::path(FPaths::RootDir()) / L"Content").wstring());
		}

		const FString MeshPath = SkeletalMesh->GetAssetPathFileName();
		if (MeshPath.empty() || MeshPath == "None")
		{
			return FPaths::ToUtf8((std::filesystem::path(FPaths::RootDir()) / L"Content").wstring());
		}

		std::filesystem::path Path(FPaths::ToWide(MeshPath));
		if (!Path.is_absolute())
		{
			Path = std::filesystem::path(FPaths::RootDir()) / Path;
		}
		return FPaths::ToUtf8(Path.parent_path().wstring());
	}

	void CopyStringToBuffer(const FString& Source, char* Buffer, size_t BufferSize)
	{
		if (!Buffer || BufferSize == 0)
		{
			return;
		}

		std::snprintf(Buffer, BufferSize, "%s", Source.c_str());
	}
	
	FString GenerateUniqueSocketName(USkeleton* Skeleton, const char* Base)
	{
		const FString Prefix = (Base && Base[0] != '\0') ? FString(Base) : FString("Socket");
		if (!Skeleton->HasSocket(FName(Prefix)))
		{
			return Prefix;
		}

		for (int32 Suffix = 1; Suffix < 10000; ++Suffix)
		{
			const FString Candidate = Prefix + std::to_string(Suffix);
			if (!Skeleton->HasSocket(FName(Candidate)))
			{
				return Candidate;
			}
		}

		return Prefix + std::to_string(static_cast<int32>(Skeleton->GetSockets().size()));
	}

	bool IsSocketNameValidForSave(const USkeleton* Skeleton, const FName& Name, int32 CurrentSocketIndex)
	{
		if (!Skeleton || !Name.IsValid() || Name == FName::None)
		{
			return false;
		}

		const int32 ExistingIndex = Skeleton->FindSocketIndex(Name);
		return ExistingIndex < 0 || ExistingIndex == CurrentSocketIndex;
	}

	FMorphTargetCurve& FindOrAddMorphCurve(UAnimSequence* Seq, const FString& MorphTargetName)
	{
		TArray<FMorphTargetCurve>& Curves = Seq->GetMutableMorphTargetCurves();
		for (FMorphTargetCurve& Curve : Curves)
		{
			if (Curve.MorphTargetName == MorphTargetName)
			{
				return Curve;
			}
		}
		FMorphTargetCurve NewCurve;
		NewCurve.MorphTargetName = MorphTargetName;
		Curves.push_back(std::move(NewCurve));
		return Curves.back();
	}

	void AddOrUpdateMorphCurveKey(FMorphTargetCurve& Curve, float TimeSeconds, float Value)
	{
		constexpr float TimeTolerance = 1.0e-4f;
		for (FRawFloatCurveKey& Key : Curve.Curve.Keys)
		{
			if (std::fabs(Key.TimeSeconds - TimeSeconds) <= TimeTolerance)
			{
				Key.Value = Value;
				return;
			}
		}
		FRawFloatCurveKey NewKey;
		NewKey.TimeSeconds   = TimeSeconds;
		NewKey.Value         = Value;
		NewKey.Interpolation = 2;
		Curve.Curve.Keys.push_back(NewKey);
		std::sort(
			Curve.Curve.Keys.begin(),
			Curve.Curve.Keys.end(),
			[](const FRawFloatCurveKey& A, const FRawFloatCurveKey& B)
			{
				return A.TimeSeconds < B.TimeSeconds;
			}
		);
	}

	EUberLitDefines::ELightingModel GetLightingModelForViewMode(EViewMode ViewMode)
	{
		switch (ViewMode)
		{
		case EViewMode::Unlit:       return EUberLitDefines::ELightingModel::Unlit;
		case EViewMode::Lit_Gouraud: return EUberLitDefines::ELightingModel::Gouraud;
		case EViewMode::Lit_Lambert: return EUberLitDefines::ELightingModel::Lambert;
		case EViewMode::Lit_Phong:
		case EViewMode::LightCulling:
		default:                     return EUberLitDefines::ELightingModel::Phong;
		}
	}
}

static uint32 GNextMeshEditorInstanceId = 0;

void FMeshEditorWidget::RecordImportDurationForAsset(const FString& AssetPath, double Seconds)
{
	if (AssetPath.empty() || AssetPath == "None" || Seconds < 0.0)
	{
		return;
	}

	GMeshImportDurationsByAssetPath[AssetPath] = Seconds;
}

void FMeshEditorWidget::ClearImportDurationForAsset(const FString& AssetPath)
{
	if (AssetPath.empty() || AssetPath == "None")
	{
		return;
	}

	GMeshImportDurationsByAssetPath.erase(AssetPath);
}

FMeshEditorWidget::FMeshEditorWidget()
	: InstanceId(GNextMeshEditorInstanceId++)
{
	const FString Id = std::to_string(InstanceId);
	PreviewWorldHandle = FName("MeshEditorPreview_" + Id);
	WindowIdSuffix = "###MeshEditor_" + Id;
}

bool FMeshEditorWidget::CanEdit(UObject* Object) const
{
	return Object && (Object->IsA<USkeletalMesh>() || Object->IsA<UPhysicsAsset>());
}

bool FMeshEditorWidget::IsEditingObject(UObject* Object) const
{
	if (FAssetEditorWidget::IsEditingObject(Object))
	{
		return true;
	}

	const USkeletalMesh* CurrentMesh = Cast<USkeletalMesh>(EditedObject);
	if (!IsOpen() || !CurrentMesh)
	{
		return false;
	}

	if (const USkeletalMesh* RequestedMesh = Cast<USkeletalMesh>(Object))
	{
		const FString& CurrentPath = CurrentMesh->GetAssetPathFileName();
		return !CurrentPath.empty()
			&& CurrentPath != "None"
			&& CurrentPath == RequestedMesh->GetAssetPathFileName();
	}

	if (const UPhysicsAsset* RequestedPhysicsAsset = Cast<UPhysicsAsset>(Object))
	{
		const FString& RequestedPath = RequestedPhysicsAsset->GetAssetPathFileName();
		const FString& CurrentPhysicsPath = CurrentMesh->GetPhysicsAssetPath();
		return !RequestedPath.empty()
			&& RequestedPath != "None"
			&& RequestedPath == CurrentPhysicsPath;
	}

	return false;
}

void FMeshEditorWidget::Open(UObject* Object)
{
	USkeletalMesh* MeshToOpen = Cast<USkeletalMesh>(Object);
	UPhysicsAsset* RequestedPhysicsAsset = Cast<UPhysicsAsset>(Object);

	if (!MeshToOpen && RequestedPhysicsAsset)
	{
		ID3D11Device* Device = EditorEngine ? EditorEngine->GetRenderer().GetFD3DDevice().GetDevice() : nullptr;
		MeshToOpen = FindPreviewMeshForPhysicsAsset(RequestedPhysicsAsset, Device);
		if (!MeshToOpen)
		{
			return;
		}
	}

	if (!MeshToOpen)
	{
		return;
	}

	FAssetEditorWidget::Open(MeshToOpen);

	FWorldContext& WorldContext = GEngine->CreateWorldContext(EWorldType::EditorPreview, PreviewWorldHandle);
	WorldContext.World->SetWorldType(EWorldType::EditorPreview);
	WorldContext.World->InitWorld();

	AActor* Actor = WorldContext.World->SpawnActor<AActor>();
	if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(EditedObject))
	{
		USkeletalMeshComponent* Comp = Actor->AddComponent<USkeletalMeshComponent>();
		Comp->SetSkeletalMesh(Mesh);
		Actor->SetRootComponent(Comp);
	}
	Actor->SetActorLocation(FVector(0.0f, 0.0f, 0.0f));

	ADirectionalLightActor* LightActor = WorldContext.World->SpawnActor<ADirectionalLightActor>();
	LightActor->InitDefaultComponents();
	LightActor->SetActorRotation(FVector(0.0f, 45.0f, -45.0f));
	UDirectionalLightComponent* LightComp = LightActor->GetComponentByClass<UDirectionalLightComponent>();
	LightComp->SetShadowBias(0.0f);
	LightComp->PushToScene();

	AStaticMeshActor* FloorActor = WorldContext.World->SpawnActor<AStaticMeshActor>();
	FloorActor->InitDefaultComponents("Content/Data/BasicShape/Cube.OBJ");
	FloorActor->SetActorLocation(FVector(0.0f, 0.0f, -0.05f));
	FloorActor->SetActorScale(FVector(10.0f, 10.0f, 0.02f));

	ImVec2 ViewportSize = ImGui::GetContentRegionAvail();

	ViewportClient.Initialize(GEngine->GetRenderer().GetFD3DDevice().GetDevice(), static_cast<uint32>(ViewportSize.x), static_cast<uint32>(ViewportSize.y));
	ViewportClient.SetPreviewWorld(WorldContext.World);
	ViewportClient.SetPreviewActor(Actor);
	ViewportClient.SetPreviewMeshComponent(Actor->GetComponentByClass<USkeletalMeshComponent>());

	ViewportClient.CreatePreviewGizmo();
	ViewportClient.CreateBoneDebugComponent();
	ViewportClient.CreatePhysicsAssetPreviewComponent();
	ViewportClient.ResetCameraToPreviousBounds();

	WorldContext.World->SetEditorPOVProvider(&ViewportClient);

	ViewportClient.SetSelectedBone(Cast<USkeletalMesh>(EditedObject), -1);

	FSlateApplication::Get().RegisterViewport(&ViewportClient);

	// 디스크의 기존 AnimSequence .uasset 들을 목록에 채워 둔다(런타임 Load/Save 만으론 안 잡힘).
	FAnimationManager::Get().RefreshAvailableAnimations();

	ActiveTab         = RequestedPhysicsAsset ? EMeshEditorTab::Physics : EMeshEditorTab::Skeleton;
	AnimTabState      = FAnimationTabState {};
	SelectedBoneIndex = -1;
	SelectedSocketIndex = -1;
	SelectedClothLODIndex = 0;
	SelectedClothIndex = -1;
	SelectedClothSectionIndex = 0;
	ClothBrushValue = 50.0f;
	ClothBrushRadius = 25.0f;
	ClothBrushSmoothStrength = 0.5f;
	bClothPaintBrushEnabled = false;
	bMeshDirty = false;
	bSkeletonDirty = false;
	BufferedSocketIndex = -2;
	SocketNameBuffer[0] = '\0';
	SocketBoneNameBuffer[0] = '\0';
	bPhysicsAssetListDirty = true;
	SelectedPhysicsAssetIndex = -1;

	if (RequestedPhysicsAsset)
	{
		AssignPhysicsAssetToCurrentMesh(RequestedPhysicsAsset);
		PhysicsAssetEditor.OpenEmbedded(RequestedPhysicsAsset);
	}
	else
	{
		PhysicsAssetEditor.Close();
	}
}

void FMeshEditorWidget::Close()
{
	PhysicsAssetEditor.Close();
	FAssetEditorWidget::Close();

	if (UWorld* PreviewWorld = ViewportClient.GetPreviewWorld())
	{
		FScene& PreviewScene = PreviewWorld->GetScene();
		GEngine->GetRenderer().GetResources().ReleaseShadowResourcesForScene(&PreviewScene);

		if (PreviewWorldHandle.IsValid())
		{
			GEngine->DestroyWorldContext(PreviewWorldHandle);
		}
	}

	FSlateApplication::Get().UnregisterViewport(&ViewportClient);

	ViewportClient.Release();
}

void FMeshEditorWidget::Tick(float DeltaTime)
{
	if (ViewportClient.IsRenderable())
	{
		ViewportClient.Tick(DeltaTime);
		if (ViewportClient.ConsumeSocketGizmoModified())
		{
			bSkeletonDirty = true;
		}
		if (ActiveTab == EMeshEditorTab::Physics && ViewportClient.ConsumePhysicsAssetGizmoModified())
		{
			PhysicsAssetEditor.NotifyViewportGizmoModified();
		}

		int32 PickedPhysicsBodyIndex = -1;
		int32 PickedPhysicsShapeIndex = -1;
		if (ActiveTab == EMeshEditorTab::Physics &&
			ViewportClient.ConsumePhysicsAssetViewportPick(PickedPhysicsBodyIndex, PickedPhysicsShapeIndex))
		{
			PhysicsAssetEditor.SelectPhysicsShapeFromViewport(
				GetCurrentPhysicsAsset(),
				PickedPhysicsBodyIndex,
				PickedPhysicsShapeIndex);
		}

		if (ActiveTab == EMeshEditorTab::Mesh)
		{
			TickClothPaintBrush();
			if (USkeletalMeshComponent* Comp = ViewportClient.GetPreviewMeshComponent())
			{
				Comp->TickClothSimulationForEditorPreview(DeltaTime);
			}
		}

	}

	if (ActiveTab == EMeshEditorTab::Animation)
	{
		USkeletalMeshComponent* Comp = ViewportClient.GetPreviewMeshComponent();
		if (!Comp) return;
		UAnimSingleNodeInstance* NodeInst = Comp->GetAnimNodeInstance(FName::None);
		if (!NodeInst) return;

		NodeInst->UpdateAnimation(DeltaTime);

		USkeletalMesh* Mesh = Comp->GetSkeletalMesh();
		if (!Mesh) return;
		FSkeletalMesh* Asset = Mesh->GetSkeletalMeshAsset();
		if (!Asset || Asset->Bones.empty()) return;

		FPoseContext Out;
		Out.SkeletalMesh = Mesh;
		Out.Pose.resize(Asset->Bones.size());
		Out.ResetToRefPose();

		NodeInst->EvaluatePose(Out);
		ApplyMorphPreviewOverrides(Out.MorphWeights);

		Comp->SetAnimationPose(Out.Pose, Out.MorphWeights);
		Comp->TickClothSimulationForEditorPreview(DeltaTime);
	}
}

void FMeshEditorWidget::CollectPreviewViewports(TArray<IEditorPreviewViewportClient*>& OutClients) const
{
	if (IsOpen())
	{
		if (ActiveTab == EMeshEditorTab::Physics)
		{
			FMeshEditorWidget* MutableThis = const_cast<FMeshEditorWidget*>(this);
			UPhysicsAsset* PhysicsAsset = MutableThis->GetCurrentPhysicsAsset();
			USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
			MutableThis->PhysicsAssetEditor.RenderPhysicsPreview(
				PhysicsAsset,
				SkeletalMesh,
				ViewportClient.GetPreviewWorld(),
				ViewportClient.GetPreviewMeshComponent(),
				ViewportClient.GetPhysicsAssetPreviewComponent(),
				ViewportClient.GetRenderDevice());
		}
		else if (ViewportClient.GetPhysicsAssetPreviewComponent())
		{
			ViewportClient.GetPhysicsAssetPreviewComponent()->ClearPreview(ViewportClient.GetRenderDevice());
		}
		OutClients.push_back(const_cast<FMeshEditorViewportClient*>(&ViewportClient));
	}
}

void FMeshEditorWidget::AddReferencedObjects(FReferenceCollector& Collector)
{
	FAssetEditorWidget::AddReferencedObjects(Collector);
	PhysicsAssetEditor.AddReferencedObjects(Collector);
}

// ─────────────────────────────────────────────────────────────────────────────
// Render entry point
// ─────────────────────────────────────────────────────────────────────────────

void FMeshEditorWidget::Render(float DeltaTime)
{
	// 1프레임 지연 close (SRV lifetime issue)
	if (bPendingClose)
	{
		Close();
		bPendingClose = false;
		return;
	}
	if (!IsOpen() || !EditedObject)
	{
		return;
	}

	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);

	bool bWindowOpen = true;
	FString VisibleTitle = "Mesh Editor";
	const FString AssetPath = SkeletalMesh ? SkeletalMesh->GetAssetPathFileName() : FString();
	if (!AssetPath.empty())
	{
		VisibleTitle += " - ";
		VisibleTitle += AssetPath;
	}
	if (IsDirty())
	{
		VisibleTitle += " *";
	}

	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_None;
	if (ViewportClient.IsMouseOverViewport())
	{
		WindowFlags |= ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;
	}

	FString WindowTitle = VisibleTitle + WindowIdSuffix;
	if (ConsumeFocusRequest())
	{
		ImGui::SetNextWindowFocus();
	}

	if (!ImGui::Begin(WindowTitle.c_str(), &bWindowOpen, WindowFlags))
	{
		// 접힌 동안엔 hover 를 보고하지 않음
		ImGui::End();
		if (!bWindowOpen)
		{
			Close();
		}
		return;
	}

	RenderDocument(DeltaTime);

	ImGui::End();

	if (!bWindowOpen)
	{
		bPendingClose = true;
	}
}

void FMeshEditorWidget::RenderDocument(float DeltaTime)
{
	(void)DeltaTime;

	if (bPendingClose)
	{
		Close();
		bPendingClose = false;
		return;
	}
	if (!IsOpen() || !EditedObject)
	{
		return;
	}

	if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows))
	{
		FSlateApplication::Get().BringViewportToFront(&ViewportClient);

		const ImGuiIO& IO = ImGui::GetIO();
		if (IO.KeyCtrl && !IO.KeyAlt && ImGui::IsKeyPressed(ImGuiKey_S, false))
		{
			SaveAllDirtyAssets();
		}
	}

	RenderTabBar();
	ImGui::Separator();

	const float AvailableHeight = ImGui::GetContentRegionAvail().y;

	switch (ActiveTab)
	{
	case EMeshEditorTab::Skeleton:
		RenderSkeletonLayout();
		break;
	case EMeshEditorTab::Mesh:
		RenderMeshLayout();
		break;
	case EMeshEditorTab::Animation:
		RenderAnimationLayout(AvailableHeight);
		break;
	case EMeshEditorTab::Physics:
		RenderPhysicsLayout(AvailableHeight);
		break;
	}
}

FString FMeshEditorWidget::GetDocumentTitle() const
{
	const USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	const FString AssetPath = SkeletalMesh ? SkeletalMesh->GetAssetPathFileName() : FString();
	return AssetPath.empty() ? FString("Skeletal Mesh") : FString("Skeletal Mesh - " + AssetPath);
}

FString FMeshEditorWidget::GetDocumentPayloadId() const
{
	const USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	const FString AssetPath = SkeletalMesh ? SkeletalMesh->GetAssetPathFileName() : FString();
	return AssetPath.empty() ? FAssetEditorWidget::GetDocumentPayloadId() : AssetPath;
}

// ─────────────────────────────────────────────────────────────────────────────
// Tab bar
// ─────────────────────────────────────────────────────────────────────────────

void FMeshEditorWidget::RenderTabBar()
{
	// 언리얼 Persona 모드 툴바: 평평한 버튼 + 선택 시 액센트 밑줄.
	constexpr float BarHeight = 30.0f;
	ImDrawList*     DrawList  = ImGui::GetWindowDrawList();
	const ImVec2    BarPos    = ImGui::GetCursorScreenPos();
	const float     BarWidth  = ImGui::GetContentRegionAvail().x;
	DrawList->AddRectFilled(BarPos, ImVec2(BarPos.x + BarWidth, BarPos.y + BarHeight),
	                        IM_COL32(38, 38, 38, 255));

	const bool bCanSaveSkeleton = bSkeletonDirty;
	const bool bCanSaveMesh = bMeshDirty;
	const bool bCanSaveAnimation = IsCurrentAnimationDirty();
	const bool bCanSavePhysics = IsCurrentPhysicsAssetDirty();
	const bool bCanSave = bCanSaveSkeleton || bCanSaveMesh || bCanSaveAnimation || bCanSavePhysics;
	const bool bShowDirtySave = bCanSave;
	if (!bCanSave)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.45f);
	}
	if (ImGui::Button(bShowDirtySave ? "Save*" : "Save", ImVec2(68.0f, BarHeight)))
	{
		SaveAllDirtyAssets();
	}
	if (!bCanSave)
	{
		ImGui::PopStyleVar();
	}
	ImGui::SameLine(0.0f, 8.0f);

	auto TabButton = [&](const char* Label, const wchar_t* IconFile, EMeshEditorTab Tab)
	{
		const bool      bActive = (ActiveTab == Tab);
		constexpr float IconSz  = 18.0f;
		constexpr float PadX    = 14.0f;
		constexpr float Gap     = 8.0f;

		const ImVec2 Pos    = ImGui::GetCursorScreenPos();
		const ImVec2 TextSz = ImGui::CalcTextSize(Label);
		const float  Width  = PadX + IconSz + Gap + TextSz.x + PadX;

		ImGui::InvisibleButton(Label, ImVec2(Width, BarHeight));
		const bool bHovered = ImGui::IsItemHovered();
		if (ImGui::IsItemClicked())
		{
			const EMeshEditorTab PreviousTab = ActiveTab;
			ActiveTab = Tab;
			if (PreviousTab == EMeshEditorTab::Physics && ActiveTab != EMeshEditorTab::Physics)
			{
				ViewportClient.ClearPhysicsAssetGizmoTarget();
			}
			if (PreviousTab != ActiveTab && ActiveTab == EMeshEditorTab::Skeleton)
			{
				if (USkeletalMeshComponent* Comp = ViewportClient.GetPreviewMeshComponent())
				{
					Comp->ApplyBoneEditBasePose();
				}
			}
		}

		if (bActive || bHovered)
		{
			DrawList->AddRectFilled(Pos, ImVec2(Pos.x + Width, Pos.y + BarHeight),
				bActive ? IM_COL32(41, 41, 41, 255) : IM_COL32(255, 255, 255, 20));
		}

		const float IconY = Pos.y + (BarHeight - IconSz) * 0.5f;
		if (ID3D11ShaderResourceView* Icon = LoadTabIcon(IconFile))
		{
			DrawList->AddImage(reinterpret_cast<ImTextureID>(Icon),
			                   ImVec2(Pos.x + PadX, IconY),
			                   ImVec2(Pos.x + PadX + IconSz, IconY + IconSz));
		}

		DrawList->AddText(ImVec2(Pos.x + PadX + IconSz + Gap, Pos.y + (BarHeight - TextSz.y) * 0.5f),
		                  bActive ? IM_COL32(255, 255, 255, 255) : IM_COL32(190, 190, 190, 255),
		                  Label);

		if (bActive)
		{
			DrawList->AddRectFilled(ImVec2(Pos.x, Pos.y + BarHeight - 2.0f),
			                        ImVec2(Pos.x + Width, Pos.y + BarHeight),
			                        IM_COL32(64, 132, 224, 255));
		}
		ImGui::SameLine(0.0f, 0.0f);
	};

	TabButton("Skeleton", L"Skeleton.png", EMeshEditorTab::Skeleton);
	TabButton("Mesh", L"SkeletalMesh.png", EMeshEditorTab::Mesh);
	TabButton("Animation", L"Animation.png", EMeshEditorTab::Animation);
	TabButton("Physics", L"PhysicsAsset.png", EMeshEditorTab::Physics);

	ImGui::NewLine();
}

// ─────────────────────────────────────────────────────────────────────────────
// Shared: viewport panel
// ─────────────────────────────────────────────────────────────────────────────

void FMeshEditorWidget::RenderViewportPanel(ImVec2 Size)
{
	ImVec2 ViewportPos = ImGui::GetCursorScreenPos();
	ViewportClient.SetViewportRect(ViewportPos.x, ViewportPos.y, Size.x, Size.y);

	FViewport* VP = ViewportClient.GetViewport();
	if (!VP || Size.x <= 0 || Size.y <= 0)
	{
		ImGui::Dummy(Size);
		return;
	}

	VP->RequestResize(static_cast<uint32>(Size.x), static_cast<uint32>(Size.y));

	if (VP->GetSRV())
	{
		ImGui::Image((ImTextureID)VP->GetSRV(), Size);
	}
	else
	{
		ImGui::Dummy(Size);
	}

	// ImGui가 계산한 hover(다른 창에 가려지면 false)를 입력 소유권 중재에 보고.
	FSlateApplication::Get().SetViewportImGuiHovered(&ViewportClient, ImGui::IsItemHovered());

	constexpr float ToolbarHeight = 28.0f;
	ImDrawList*     DrawList      = ImGui::GetWindowDrawList();
	RenderTemporaryClothPaintValueOverlay(DrawList, ViewportPos, Size);
	RenderClothBrushRadiusOverlay(DrawList, ViewportPos, Size);
	DrawList->AddRectFilled(ViewportPos, ImVec2(ViewportPos.x + Size.x, ViewportPos.y + ToolbarHeight), IM_COL32(40, 40, 40, 255));

	FViewportToolbarContext Context;
	Context.Renderer              = &GEngine->GetRenderer();
	Context.Gizmo                 = ViewportClient.GetGizmo();
	Context.Settings              = &FEditorSettings::Get().MeshEditorViewportSettings;
	Context.RenderOptions         = &ViewportClient.GetRenderOptions();
	Context.ToolbarLeft           = ViewportPos.x;
	Context.ToolbarTop            = ViewportPos.y;
	Context.ToolbarWidth          = Size.x;
	Context.bReservePlayStopSpace = false;
	Context.bShowAddActor         = false;
	Context.OnCoordSystemToggled  = [&]()
	{
		FGizmoToolSettings& Settings = FEditorSettings::Get().MeshEditorViewportSettings.Gizmo;
		Settings.CoordSystem         = (Settings.CoordSystem == EEditorCoordSystem::World) ? EEditorCoordSystem::Local : EEditorCoordSystem::World;
		ViewportClient.ApplyTransformSettingsToGizmo();
	};
	Context.OnSettingsChanged = [&]()
	{
		ViewportClient.ApplyTransformSettingsToGizmo();
	};
	Context.OnRenderViewModeExtras = [&]()
	{
		const EBoneDebugDrawMode CurrentBoneDrawMode = ViewportClient.GetBoneDebugDrawMode();
		int32                    BoneDrawMode        = static_cast<int32>(CurrentBoneDrawMode);
		ImGui::Text("Bone Display");
		ImGui::RadioButton("Selected Bone", &BoneDrawMode, static_cast<int32>(EBoneDebugDrawMode::SelectedOnly));
		ImGui::RadioButton("All Bones", &BoneDrawMode, static_cast<int32>(EBoneDebugDrawMode::AllBones));
		if (BoneDrawMode != static_cast<int32>(CurrentBoneDrawMode))
		{
			ViewportClient.SetBoneDebugDrawMode(static_cast<EBoneDebugDrawMode>(BoneDrawMode));
		}

		FViewportRenderOptions& RenderOptions = ViewportClient.GetRenderOptions();
		bool bWeightBoneHeatMap = RenderOptions.bWeightBoneHeatMap;
		if (ImGui::Checkbox("Weight Bone HeatMap", &bWeightBoneHeatMap))
		{
			RenderOptions.bWeightBoneHeatMap = bWeightBoneHeatMap;
			RenderOptions.WeightBoneHeatMapBoneIndex = SelectedBoneIndex;
			if (bWeightBoneHeatMap)
			{
				FShaderManager::Get().GetOrCreateUberLitPermutation(
					GetLightingModelForViewMode(RenderOptions.ViewMode),
					EUberLitDefines::EVertexFactory::SkeletalMesh,
					EShaderErrorMode::Notification,
					true);
			}
		}

		if (ActiveTab == EMeshEditorTab::Physics)
		{
			PhysicsAssetEditor.RenderViewportDebugOptions();
		}
	};

	FViewportToolbar::Render(Context);
	RenderMeshStatsOverlay(DrawList, ViewportPos);
}

// ─────────────────────────────────────────────────────────────────────────────
// Skeleton tab
// ─────────────────────────────────────────────────────────────────────────────

void FMeshEditorWidget::SaveCurrentSkeleton()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	USkeleton* Skeleton = GetEditingSkeleton(SkeletalMesh);
	if (!SkeletalMesh || !Skeleton)
	{
		return;
	}

	Skeleton->RebuildReferenceSkeletonDerivedData();
	Skeleton->RebuildBoneNameCache();
	const FString& SkeletonPath = Skeleton->GetAssetPathFileName();
	if (SkeletonPath.empty() || SkeletonPath == "None")
	{
		return;
	}

	FAssetImportMetadata Metadata;
	FAssetPackage::ReadMetadata(SkeletonPath, EAssetPackageType::Skeleton, Metadata);
	if (FSkeletonManager::Get().SaveSkeleton(Skeleton, SkeletonPath, Metadata.SourcePath))
	{
		SkeletalMesh->SetSkeleton(Skeleton);
		if (USkeletalMeshComponent* PreviewMeshComponent = ViewportClient.GetPreviewMeshComponent())
		{
			PreviewMeshComponent->ApplyBoneEditBasePose();
		}
		bSkeletonDirty = false;
	}
}

void FMeshEditorWidget::SaveCurrentAnimationAsset()
{
	if (AnimTabState.bMontageSelected)
	{
		UAnimMontage* Montage = AnimTabState.CurrentMontage;
		if (Montage && FAnimationManager::Get().SaveMontagePreservingMetadata(Montage))
		{
			AnimTabState.DirtyMontages.erase(Montage);
			FAnimationManager::Get().RefreshAvailableMontages();
			MarkAnimationListDirty();
		}
		return;
	}

	UAnimSequence* Sequence = AnimTabState.CurrentSequence;
	if (Sequence && FAnimationManager::Get().SaveAnimationPreservingMetadata(Sequence))
	{
		AnimTabState.DirtySequences.erase(Sequence);
		FAnimationManager::Get().RefreshAvailableAnimations();
		MarkAnimationListDirty();
	}
}


bool FMeshEditorWidget::SaveCurrentMeshAsset()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	if (SkeletalMesh && FMeshManager::SaveSkeletalMeshPreservingMetadata(SkeletalMesh))
	{
		bMeshDirty = false;
		return true;
	}
	return false;
}

bool FMeshEditorWidget::SaveCurrentPhysicsAsset()
{
	if (!IsCurrentPhysicsAssetDirty())
	{
		return false;
	}

	if (!PhysicsAssetEditor.SaveEditedPhysicsAsset())
	{
		return false;
	}

	// A PhysicsAsset edit is only useful after restart if the owning SkeletalMesh
	// still persists the assignment path as well. Create/Assign already saves the
	// mesh, but saving it again here makes Ctrl+S / Save robust when the dirty
	// PhysicsAsset came from an embedded editor or a direct PhysicsAsset open.
	SaveCurrentMeshAsset();
	return true;
}

void FMeshEditorWidget::SaveAllDirtyAssets()
{
	if (bSkeletonDirty)
	{
		SaveCurrentSkeleton();
	}

	if (bMeshDirty)
	{
		SaveCurrentMeshAsset();
	}

	if (IsCurrentAnimationDirty())
	{
		SaveCurrentAnimationAsset();
	}

	if (IsCurrentPhysicsAssetDirty())
	{
		SaveCurrentPhysicsAsset();
	}
}

void FMeshEditorWidget::MarkCurrentMeshDirty()
{
	bMeshDirty = true;
	if (USkeletalMeshComponent* PreviewMeshComponent = ViewportClient.GetPreviewMeshComponent())
	{
		if (USkeletalMesh* Mesh = Cast<USkeletalMesh>(EditedObject))
		{
			PreviewMeshComponent->SetSkeletalMesh(Mesh);
		}
	}
}

bool FMeshEditorWidget::IsCurrentAnimationDirty() const
{
	if (AnimTabState.bMontageSelected)
	{
		return AnimTabState.CurrentMontage && AnimTabState.DirtyMontages.count(AnimTabState.CurrentMontage) > 0;
	}

	return AnimTabState.CurrentSequence && AnimTabState.DirtySequences.count(AnimTabState.CurrentSequence) > 0;
}

void FMeshEditorWidget::MarkCurrentAnimationDirty()
{
	if (AnimTabState.bMontageSelected)
	{
		if (AnimTabState.CurrentMontage)
		{
			AnimTabState.DirtyMontages.insert(AnimTabState.CurrentMontage);
		}
		return;
	}

	if (AnimTabState.CurrentSequence)
	{
		AnimTabState.DirtySequences.insert(AnimTabState.CurrentSequence);
	}
}

void FMeshEditorWidget::RefreshSelectedSocketEditBuffers(USkeleton* Skeleton)
{
	if (!Skeleton ||
		SelectedSocketIndex < 0 ||
		SelectedSocketIndex >= static_cast<int32>(Skeleton->GetSockets().size()))
	{
		BufferedSocketIndex = -2;
		SocketNameBuffer[0] = '\0';
		SocketBoneNameBuffer[0] = '\0';
		return;
	}

	if (BufferedSocketIndex == SelectedSocketIndex)
	{
		return;
	}

	const FSkeletalMeshSocket& Socket = Skeleton->GetSockets()[SelectedSocketIndex];
	CopyStringToBuffer(Socket.Name.ToString(), SocketNameBuffer, sizeof(SocketNameBuffer));
	CopyStringToBuffer(Socket.BoneName.ToString(), SocketBoneNameBuffer, sizeof(SocketBoneNameBuffer));
	BufferedSocketIndex = SelectedSocketIndex;
}

void FMeshEditorWidget::RenderSkeletonLayout()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	USkeleton* Skeleton = GetEditingSkeleton(SkeletalMesh);

	// Left: bone hierarchy
	ImGui::BeginChild("BoneHierarchy", ImVec2(HierarchyWidth, 0), true);
	ImGui::Text("Bone Hierarchy");
	ImGui::Separator();
	ImGui::PushStyleVar(ImGuiStyleVar_IndentSpacing, MeshEditorTreeIndentSpacing);
	if (Skeleton)
	{
		const FReferenceSkeleton& RefSkeleton = Skeleton->GetReferenceSkeleton();
		for (int32 i = 0; i < RefSkeleton.GetNumBones(); ++i)
		{
			if (RefSkeleton.Bones[i].ParentIndex == -1)
			{
				RenderBoneTree(Skeleton, RefSkeleton, i);
			}
		}
	}
	ImGui::PopStyleVar();
	ImGui::EndChild();

	ImGui::SameLine();

	// Splitter
	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::Button("##skelSplitter", ImVec2(4.0f, -1.0f));
	if (ImGui::IsItemActive())
	{
		HierarchyWidth += ImGui::GetIO().MouseDelta.x;
		HierarchyWidth = std::max(100.0f, std::min(HierarchyWidth, ImGui::GetWindowWidth() - DetailsWidth - 100.0f));
	}
	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	// Center: viewport
	ImGui::BeginGroup();
	{
		float  ViewportWidth = ImGui::GetContentRegionAvail().x - DetailsWidth - ImGui::GetStyle().ItemSpacing.x;
		ImVec2 Size          = ImVec2(ViewportWidth, ImGui::GetContentRegionAvail().y);
		RenderViewportPanel(Size);
	}
	ImGui::EndGroup();

	ImGui::SameLine();

	// Right: bone details
	ImGui::BeginChild("BoneDetails", ImVec2(DetailsWidth, 0), true);
	ImGui::Text("Bone Details");
	ImGui::Separator();

	if (Skeleton && SelectedSocketIndex >= 0 && SelectedSocketIndex < static_cast<int32>(Skeleton->GetMutableSockets().size()))
	{
		RefreshSelectedSocketEditBuffers(Skeleton);
		TArray<FSkeletalMeshSocket>& Sockets = Skeleton->GetMutableSockets();
		FSkeletalMeshSocket& Socket = Sockets[SelectedSocketIndex];
		bool bSocketChanged = false;

		ImGui::Text("Socket Details");
		ImGui::Text("Index: %d", SelectedSocketIndex);
		ImGui::Dummy(ImVec2(0, 10));

		if (ImGui::InputText("Name", SocketNameBuffer, sizeof(SocketNameBuffer)))
		{
			FName NewName(SocketNameBuffer);
			if (IsSocketNameValidForSave(Skeleton, NewName, SelectedSocketIndex))
			{
				Socket.Name = NewName;
				bSkeletonDirty = true;
				bSocketChanged = true;
			}
		}

		if (ImGui::InputText("BoneName", SocketBoneNameBuffer, sizeof(SocketBoneNameBuffer)))
		{
			FName NewBoneName(SocketBoneNameBuffer);
			if (Skeleton->FindBoneIndex(NewBoneName.ToString()) >= 0)
			{
				Socket.BoneName = NewBoneName;
				bSkeletonDirty = true;
				bSocketChanged = true;
			}
		}

		if (ImGui::DragFloat3("Location", &Socket.RelativeLocation.X, 0.1f))
		{
			bSkeletonDirty = true;
			bSocketChanged = true;
		}

		FVector Rotation = Socket.RelativeRotation.ToVector();
		if (ImGui::DragFloat3("Rotation", &Rotation.X, 0.1f))
		{
			Socket.RelativeRotation = FRotator(Rotation);
			bSkeletonDirty = true;
			bSocketChanged = true;
		}

		if (ImGui::DragFloat3("Scale", &Socket.RelativeScale.X, 0.1f, 0.01f))
		{
			Socket.RelativeScale.X = std::max(Socket.RelativeScale.X, 0.01f);
			Socket.RelativeScale.Y = std::max(Socket.RelativeScale.Y, 0.01f);
			Socket.RelativeScale.Z = std::max(Socket.RelativeScale.Z, 0.01f);
			bSkeletonDirty = true;
			bSocketChanged = true;
		}

		if (bSocketChanged)
		{
			ViewportClient.SetSelectedSocket(Cast<USkeletalMesh>(EditedObject), Skeleton, SelectedSocketIndex);
		}
	}
	else if (Skeleton && SelectedBoneIndex != -1)
	{
		FReferenceSkeleton& RefSkeleton = Skeleton->GetMutableReferenceSkeleton();
		if (SelectedBoneIndex < 0 || SelectedBoneIndex >= RefSkeleton.GetNumBones())
		{
			ImGui::TextDisabled("Select a bone to edit.");
			ImGui::EndChild();
			return;
		}

		FReferenceBone& Bone = RefSkeleton.Bones[SelectedBoneIndex];

		ImGui::Text("Name: %s", Bone.Name.c_str());
		ImGui::Text("Index: %d", SelectedBoneIndex);
		ImGui::Dummy(ImVec2(0, 10));

		USkeletalMeshComponent* PreviewMeshComponent = ViewportClient.GetPreviewMeshComponent();
		FTransform LocalTransform = PreviewMeshComponent
			? PreviewMeshComponent->GetBoneEditBaseLocalTransformByIndex(SelectedBoneIndex)
			: FTransform(Bone.LocalBindPose);

		FVector Location = LocalTransform.Location;
		if (ImGui::DragFloat3("Location", &Location.X, 0.1f))
		{
			LocalTransform.Location = Location;
			if (PreviewMeshComponent)
				PreviewMeshComponent->SetBoneEditBaseLocalTransformByIndex(SelectedBoneIndex, LocalTransform);
			else
			{
				Bone.LocalBindPose = LocalTransform.ToMatrix();
			}
			Bone.LocalBindPose = LocalTransform.ToMatrix();
			Skeleton->RebuildReferenceSkeletonDerivedData();
			bSkeletonDirty = true;
		}

		FVector Rotation = LocalTransform.GetRotator().ToVector();
		if (ImGui::DragFloat3("Rotation", &Rotation.X, 0.1f))
		{
			LocalTransform.SetRotation(FRotator(Rotation));
			if (PreviewMeshComponent)
				PreviewMeshComponent->SetBoneEditBaseLocalTransformByIndex(SelectedBoneIndex, LocalTransform);
			else
			{
				Bone.LocalBindPose = LocalTransform.ToMatrix();
			}
			Bone.LocalBindPose = LocalTransform.ToMatrix();
			Skeleton->RebuildReferenceSkeletonDerivedData();
			bSkeletonDirty = true;
		}

		FVector Scale = LocalTransform.Scale;
		if (ImGui::DragFloat3("Scale", &Scale.X, 0.1f, 0.01f))
		{
			LocalTransform.Scale = Scale;
			if (PreviewMeshComponent)
				PreviewMeshComponent->SetBoneEditBaseLocalTransformByIndex(SelectedBoneIndex, LocalTransform);
			else
			{
				Bone.LocalBindPose = LocalTransform.ToMatrix();
			}
			Bone.LocalBindPose = LocalTransform.ToMatrix();
			Skeleton->RebuildReferenceSkeletonDerivedData();
			bSkeletonDirty = true;
		}
	}
	else
	{
		ImGui::TextDisabled("Select a bone to edit.");
	}

	ImGui::EndChild();
}

// ─────────────────────────────────────────────────────────────────────────────
// Physics tab
// ─────────────────────────────────────────────────────────────────────────────

UPhysicsAsset* FMeshEditorWidget::GetCurrentPhysicsAsset()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	return SkeletalMesh ? SkeletalMesh->GetPhysicsAsset() : nullptr;
}

bool FMeshEditorWidget::AssignPhysicsAssetToCurrentMesh(UPhysicsAsset* PhysicsAsset)
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	if (!SkeletalMesh || !PhysicsAsset)
	{
		return false;
	}

	SkeletalMesh->SetPhysicsAsset(PhysicsAsset);
	if (SkeletalMesh->GetPhysicsAsset() != PhysicsAsset)
	{
		return false;
	}

	PhysicsAssetEditor.OpenEmbedded(PhysicsAsset);
	bPhysicsAssetListDirty = true;
	return SaveCurrentMeshAsset();
}

UPhysicsAsset* FMeshEditorWidget::CreateAndAssignPhysicsAssetForCurrentMesh()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	if (!SkeletalMesh)
	{
		return nullptr;
	}

	FString CreatedPath;
	if (!FAssetFactory::CreatePhysicsAssetForSkeletalMesh(
			MakePhysicsAssetDirectoryForMesh(SkeletalMesh),
			MakePhysicsAssetNameForMesh(SkeletalMesh),
			SkeletalMesh,
			CreatedPath))
	{
		return nullptr;
	}

	UPhysicsAsset* PhysicsAsset = FPhysicsAssetManager::Get().LoadPhysicsAsset(CreatedPath);
	if (!PhysicsAsset)
	{
		return nullptr;
	}

	AssignPhysicsAssetToCurrentMesh(PhysicsAsset);
	FPhysicsAssetManager::Get().ScanPhysicsAssets();
	bPhysicsAssetListDirty = true;
	return PhysicsAsset;
}

void FMeshEditorWidget::RefreshPhysicsAssetList()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	CachedPhysicsAssetFiles.clear();
	SelectedPhysicsAssetIndex = -1;
	if (!SkeletalMesh)
	{
		bPhysicsAssetListDirty = false;
		return;
	}

	CachedPhysicsAssetFiles = FAssetRegistry::ListPhysicsAssetsForSkeleton(SkeletalMesh->GetSkeletonBinding(), true);
	bPhysicsAssetListDirty = false;
}

bool FMeshEditorWidget::IsCurrentPhysicsAssetDirty() const
{
	return PhysicsAssetEditor.HasUnsavedChanges();
}

void FMeshEditorWidget::RenderMissingPhysicsAssetPanel(USkeletalMesh* SkeletalMesh)
{
	ImGui::TextUnformatted("Physics Asset");
	ImGui::Separator();
	ImGui::TextDisabled("No Physics Asset is assigned to this Skeletal Mesh.");
	ImGui::Spacing();

	if (!SkeletalMesh)
	{
		ImGui::TextDisabled("Open a Skeletal Mesh first.");
		return;
	}

	if (ImGui::Button("Create Physics Asset", ImVec2(180.0f, 0.0f)))
	{
		CreateAndAssignPhysicsAssetForCurrentMesh();
	}

	ImGui::Spacing();
	ImGui::Separator();
	ImGui::TextUnformatted("Assign Existing Physics Asset");
	if (bPhysicsAssetListDirty)
	{
		RefreshPhysicsAssetList();
	}

	if (CachedPhysicsAssetFiles.empty())
	{
		ImGui::TextDisabled("No compatible Physics Asset found.");
		return;
	}

	const char* Preview = (SelectedPhysicsAssetIndex >= 0 && SelectedPhysicsAssetIndex < static_cast<int32>(CachedPhysicsAssetFiles.size()))
		? CachedPhysicsAssetFiles[SelectedPhysicsAssetIndex].DisplayName.c_str()
		: "Select Physics Asset";
	if (ImGui::BeginCombo("Physics Asset", Preview))
	{
		for (int32 Index = 0; Index < static_cast<int32>(CachedPhysicsAssetFiles.size()); ++Index)
		{
			const bool bSelected = SelectedPhysicsAssetIndex == Index;
			if (ImGui::Selectable(CachedPhysicsAssetFiles[Index].DisplayName.c_str(), bSelected))
			{
				SelectedPhysicsAssetIndex = Index;
			}
			if (bSelected)
			{
				ImGui::SetItemDefaultFocus();
			}
		}
		ImGui::EndCombo();
	}

	const bool bCanAssign = SelectedPhysicsAssetIndex >= 0 && SelectedPhysicsAssetIndex < static_cast<int32>(CachedPhysicsAssetFiles.size());
	if (!bCanAssign) ImGui::BeginDisabled();
	if (ImGui::Button("Assign Selected", ImVec2(180.0f, 0.0f)))
	{
		if (UPhysicsAsset* PhysicsAsset = FPhysicsAssetManager::Get().LoadPhysicsAsset(CachedPhysicsAssetFiles[SelectedPhysicsAssetIndex].FullPath))
		{
			AssignPhysicsAssetToCurrentMesh(PhysicsAsset);
		}
	}
	if (!bCanAssign) ImGui::EndDisabled();
}

void FMeshEditorWidget::RenderPhysicsLayout(float TotalHeight)
{
	(void)TotalHeight;
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	UPhysicsAsset* PhysicsAsset = GetCurrentPhysicsAsset();

	if (PhysicsAsset)
	{
		PhysicsAssetEditor.RenderEmbeddedToolbar(PhysicsAsset, SkeletalMesh, 0.0f);
	}
	else
	{
		ImGui::TextDisabled("No Physics Asset selected.");
	}

	const float Spacing = ImGui::GetStyle().ItemSpacing.x;
	const float SplitterWidth = 4.0f;
	const float TotalWidth = ImGui::GetContentRegionAvail().x;
	constexpr float MinLeftWidth = 320.0f;
	constexpr float MinViewportWidth = 260.0f;
	constexpr float MinDetailsWidth = 300.0f;
	const float WidthForPanels = std::max(0.0f, TotalWidth - SplitterWidth * 2.0f - Spacing * 2.0f);

	PhysicsPanelWidth = std::max(MinLeftWidth, std::min(PhysicsPanelWidth, WidthForPanels - MinViewportWidth - MinDetailsWidth));
	PhysicsDetailsWidth = std::max(MinDetailsWidth, std::min(PhysicsDetailsWidth, WidthForPanels - PhysicsPanelWidth - MinViewportWidth));
	float ViewportWidth = WidthForPanels - PhysicsPanelWidth - PhysicsDetailsWidth;
	if (ViewportWidth < MinViewportWidth)
	{
		const float Deficit = MinViewportWidth - ViewportWidth;
		const float DetailsShrink = std::min(Deficit, std::max(0.0f, PhysicsDetailsWidth - MinDetailsWidth));
		PhysicsDetailsWidth -= DetailsShrink;
		ViewportWidth = WidthForPanels - PhysicsPanelWidth - PhysicsDetailsWidth;
	}
	ViewportWidth = std::max(MinViewportWidth, ViewportWidth);

	ImGui::BeginChild("PhysicsAssetTreeAndGraphPanel", ImVec2(PhysicsPanelWidth, 0), true);
	if (PhysicsAsset)
	{
		PhysicsAssetEditor.RenderEmbeddedTreeAndGraph(PhysicsAsset, SkeletalMesh, 0.0f);
	}
	else
	{
		RenderMissingPhysicsAssetPanel(SkeletalMesh);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::Button("##physicsTreeViewportSplitter", ImVec2(SplitterWidth, -1.0f));
	if (ImGui::IsItemActive())
	{
		PhysicsPanelWidth += ImGui::GetIO().MouseDelta.x;
		PhysicsPanelWidth = std::max(MinLeftWidth, std::min(PhysicsPanelWidth, WidthForPanels - MinViewportWidth - PhysicsDetailsWidth));
	}
	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	ImGui::BeginChild("PhysicsViewportPanel", ImVec2(ViewportWidth, 0.0f), false);
	{
		const ImVec2 Size = ImGui::GetContentRegionAvail();
		RenderViewportPanel(Size);
	}
	ImGui::EndChild();

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
	ImGui::Button("##physicsViewportDetailsSplitter", ImVec2(SplitterWidth, -1.0f));
	if (ImGui::IsItemActive())
	{
		PhysicsDetailsWidth -= ImGui::GetIO().MouseDelta.x;
		PhysicsDetailsWidth = std::max(MinDetailsWidth, std::min(PhysicsDetailsWidth, WidthForPanels - PhysicsPanelWidth - MinViewportWidth));
	}
	if (ImGui::IsItemHovered() || ImGui::IsItemActive())
	{
		ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
	}
	ImGui::PopStyleColor(3);

	ImGui::SameLine();

	ImGui::BeginChild("PhysicsAssetDetailsPanel", ImVec2(PhysicsDetailsWidth, 0.0f), true);
	if (PhysicsAsset)
	{
		PhysicsAssetEditor.RenderEmbeddedDetails(PhysicsAsset, SkeletalMesh, 0.0f);
	}
	else
	{
		ImGui::TextDisabled("No Physics Asset selected.");
	}
	ImGui::EndChild();

	if (PhysicsAsset)
	{
		ViewportClient.SetSelectedPhysicsAssetElement(
			PhysicsAsset,
			PhysicsAssetEditor.GetSelectedBodyIndex(),
			PhysicsAssetEditor.GetSelectedShapeIndex(),
			PhysicsAssetEditor.GetSelectedConstraintIndex(),
			PhysicsAssetEditor.GetSelectedConstraintGizmoFrame());
	}
	else
	{
		ViewportClient.ClearPhysicsAssetGizmoTarget();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Mesh tab
// ─────────────────────────────────────────────────────────────────────────────

void FMeshEditorWidget::RenderMeshLayout()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);

	// Left: mesh info
	const float StatsWidth = 220.0f;
	ImGui::BeginChild("MeshInfo", ImVec2(StatsWidth, 0), true);
	ImGui::Text("Mesh Info");
	ImGui::Separator();
	if (SkeletalMesh)
	{
			FSkeletalMesh* Asset = SkeletalMesh->GetSkeletalMeshAsset();
		if (Asset)
		{
			ImGui::Text("Vertices:  %s", FormatMeshStatCount(Asset->Vertices.size()).c_str());
			ImGui::Text("Triangles: %s", FormatMeshStatCount(Asset->Indices.size() / 3).c_str());
			ImGui::Text("Bones:     %zu", Asset->Bones.size());
			ImGui::Text("Morphs:    %zu", Asset->MorphTargets.size());
			ImGui::Dummy(ImVec2(0, 8));
			ImGui::Separator();
			RenderClothAuthoringPanel(SkeletalMesh, Asset);
			USkeletalMeshComponent* PreviewMeshComponent = ViewportClient.GetPreviewMeshComponent();
			if (!Asset->MorphTargets.empty() && PreviewMeshComponent)
			{
				ImGui::Dummy(ImVec2(0, 8));
				ImGui::Separator();
				ImGui::TextUnformatted("Morph Preview");
				if (ImGui::SmallButton("Reset Morphs"))
				{
					PreviewMeshComponent->ClearMorphTargetWeights();
				}
				for (int32 MorphIndex = 0; MorphIndex < static_cast<int32>(Asset->MorphTargets.size()); ++MorphIndex)
				{
					const FMorphTarget& MorphTarget = Asset->MorphTargets[MorphIndex];
					float               Weight      = PreviewMeshComponent->GetMorphTargetWeightByIndex(MorphIndex);
					ImGui::PushID(MorphIndex);
					const char* Label = MorphTarget.Name.empty() ? "Unnamed" : MorphTarget.Name.c_str();
					if (ImGui::SliderFloat(Label, &Weight, -1.0f, 1.0f, "%.3f"))
					{
						PreviewMeshComponent->SetMorphTargetWeightByIndex(MorphIndex, Weight);
					}
					if (ImGui::IsItemHovered())
					{
						ImGui::SetTooltip("%zu vertex deltas", MorphTarget.Deltas.size());
					}
					ImGui::PopID();
				}
			}
			ImGui::Dummy(ImVec2(0, 8));
			const FString& Path = SkeletalMesh->GetAssetPathFileName();
			if (!Path.empty() && Path != "None")
			{
				ImGui::TextWrapped("Path:\n%s", Path.c_str());
			}
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// Center: viewport (full remaining width)
	ImGui::BeginGroup();
	{
		ImVec2 Size = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y);
		RenderViewportPanel(Size);
	}
	ImGui::EndGroup();
}

void FMeshEditorWidget::RenderClothAuthoringPanel(USkeletalMesh* SkeletalMesh, FSkeletalMesh* Asset)
{
	if (!SkeletalMesh || !Asset)
	{
		return;
	}

	ImGui::TextUnformatted("Cloth");

	int32 LODIndex = SelectedClothLODIndex;
	if (ImGui::InputInt("LOD", &LODIndex))
	{
		SelectedClothLODIndex = std::max(0, LODIndex);
		SelectedClothIndex = -1;
	}

	if (!Asset->Sections.empty())
	{
		SelectedClothSectionIndex = std::clamp(
			SelectedClothSectionIndex,
			0,
			static_cast<int32>(Asset->Sections.size()) - 1
		);
		const FSkeletalMeshSection& CurrentSection = Asset->Sections[SelectedClothSectionIndex];
		const FString SectionLabel = CurrentSection.MaterialSlotName.empty()
			? ("Section " + std::to_string(SelectedClothSectionIndex))
			: (std::to_string(SelectedClothSectionIndex) + ": " + CurrentSection.MaterialSlotName);

		if (ImGui::BeginCombo("Section", SectionLabel.c_str()))
		{
			for (int32 SectionIndex = 0; SectionIndex < static_cast<int32>(Asset->Sections.size()); ++SectionIndex)
			{
				const FSkeletalMeshSection& Section = Asset->Sections[SectionIndex];
				const FString ItemLabel = Section.MaterialSlotName.empty()
					? ("Section " + std::to_string(SectionIndex))
					: (std::to_string(SectionIndex) + ": " + Section.MaterialSlotName);
				if (ImGui::Selectable(ItemLabel.c_str(), SelectedClothSectionIndex == SectionIndex))
				{
					SelectedClothSectionIndex = SectionIndex;
				}
			}
			ImGui::EndCombo();
		}

		if (ImGui::Button("Create From Section", ImVec2(-1.0f, 0.0f)))
		{
			FSkeletalClothData NewCloth;
			FString Error;
			const FString ClothName = CurrentSection.MaterialSlotName.empty()
				? ("Cloth_Section_" + std::to_string(SelectedClothSectionIndex))
				: CurrentSection.MaterialSlotName;
			if (Asset->BuildClothDataFromSection(
					static_cast<uint32>(SelectedClothLODIndex),
					SelectedClothSectionIndex,
					ClothName,
					NewCloth,
					&Error))
			{
				NewCloth.bEnabled = true;
				Asset->AddOrReplaceClothData(std::move(NewCloth));
				FSkeletalClothLODData* LODData = Asset->FindClothLOD(static_cast<uint32>(SelectedClothLODIndex));
				SelectedClothIndex = LODData ? static_cast<int32>(LODData->Cloths.size()) - 1 : -1;
				MarkCurrentMeshDirty();
			}
			else
			{
				UE_LOG("Create cloth data failed: %s", Error.c_str());
			}
		}

		bool bHasClothForSelectedSection = false;
		if (const FSkeletalClothLODData* ExistingLODData = Asset->FindClothLOD(static_cast<uint32>(SelectedClothLODIndex)))
		{
			for (const FSkeletalClothData& Cloth : ExistingLODData->Cloths)
			{
				if (Cloth.Binding.LODIndex == static_cast<uint32>(SelectedClothLODIndex) &&
					Cloth.Binding.SectionIndex == SelectedClothSectionIndex)
				{
					bHasClothForSelectedSection = true;
					break;
				}
			}
		}

		if (!bHasClothForSelectedSection)
		{
			ImGui::BeginDisabled();
		}
		if (ImGui::Button("Delete Section Cloth", ImVec2(-1.0f, 0.0f)))
		{
			if (Asset->RemoveClothDataForSection(static_cast<uint32>(SelectedClothLODIndex), SelectedClothSectionIndex))
			{
				SelectedClothIndex = -1;
				MarkCurrentMeshDirty();
			}
		}
		if (!bHasClothForSelectedSection)
		{
			ImGui::EndDisabled();
		}
	}

	FSkeletalClothLODData* LODData = Asset->FindClothLOD(static_cast<uint32>(SelectedClothLODIndex));
	if (!LODData || LODData->Cloths.empty())
	{
		ImGui::TextDisabled("No cloth data on this LOD.");
		return;
	}

	SelectedClothIndex = std::clamp(SelectedClothIndex, 0, static_cast<int32>(LODData->Cloths.size()) - 1);
	FSkeletalClothData* SelectedCloth = &LODData->Cloths[SelectedClothIndex];

	if (ImGui::BeginCombo("Cloth", SelectedCloth->Name.empty() ? "Unnamed" : SelectedCloth->Name.c_str()))
	{
		for (int32 ClothIndex = 0; ClothIndex < static_cast<int32>(LODData->Cloths.size()); ++ClothIndex)
		{
			const FSkeletalClothData& Cloth = LODData->Cloths[ClothIndex];
			const char* Label = Cloth.Name.empty() ? "Unnamed" : Cloth.Name.c_str();
			if (ImGui::Selectable(Label, SelectedClothIndex == ClothIndex))
			{
				SelectedClothIndex = ClothIndex;
			}
		}
		ImGui::EndCombo();
		SelectedCloth = &LODData->Cloths[SelectedClothIndex];
	}

	if (ImGui::Checkbox("Enabled", &SelectedCloth->bEnabled))
	{
		MarkCurrentMeshDirty();
	}

	const bool bCpuSkinning = SkinningModeRuntime::Get() == ESkinningMode::CPU;
	ImGui::Text("Skinning: %s", bCpuSkinning ? "CPU" : "GPU");
	if (!bCpuSkinning)
	{
		ImGui::TextDisabled("Cloth preview runs in CPU skinning mode.");
		if (ImGui::Button("Use CPU Skinning", ImVec2(-1.0f, 0.0f)))
		{
			SkinningModeRuntime::Set(ESkinningMode::CPU);
			if (USkeletalMeshComponent* Comp = ViewportClient.GetPreviewMeshComponent())
			{
				Comp->TickClothSimulationForEditorPreview(0.0f);
			}
		}
	}

	bool bChanged = false;
	bChanged |= ImGui::DragFloat("Gravity", &SelectedCloth->Config.GravityScale, 0.01f, -10.0f, 10.0f, "%.2f");
	bChanged |= ImGui::DragFloat("Solver Hz", &SelectedCloth->Config.SolverFrequency, 1.0f, 1.0f, 1000.0f, "%.0f");
	bChanged |= ImGui::SliderFloat("Stiffness", &SelectedCloth->Config.Stiffness, 0.0f, 1.0f, "%.2f");
	bChanged |= ImGui::SliderFloat("Damping", &SelectedCloth->Config.Damping, 0.0f, 1.0f, "%.2f");
	bChanged |= ImGui::DragFloat("View Min", &SelectedCloth->Paint.ViewMin, 1.0f, 0.0f, 100000.0f, "%.1f");
	bChanged |= ImGui::DragFloat("View Max", &SelectedCloth->Paint.ViewMax, 1.0f, 0.0f, 100000.0f, "%.1f");
	if (SelectedCloth->Paint.ViewMax <= SelectedCloth->Paint.ViewMin)
	{
		SelectedCloth->Paint.ViewMax = SelectedCloth->Paint.ViewMin + 1.0f;
	}
	if (bChanged)
	{
		MarkCurrentMeshDirty();
	}

	ImGui::Checkbox("Paint Brush", &bClothPaintBrushEnabled);
	ImGui::Checkbox("Show Paint Values", &bShowClothPaintValues);
	ImGui::DragFloat("Brush Value", &ClothBrushValue, 0.01f, 0.0f, 1000.0f, "%.2f");
	ImGui::DragFloat("Brush Radius", &ClothBrushRadius, 0.01f, 0.0f, 500.0f, "%.2f");
	if (ImGui::Button("Fill MaxDistance", ImVec2(-1.0f, 0.0f)))
	{
		for (float& Value : SelectedCloth->Paint.MaxDistanceValues)
		{
			Value = ClothBrushValue;
		}
		MarkCurrentMeshDirty();
	}
	if (ImGui::Button("Test Pin Top / Free Rest", ImVec2(-1.0f, 0.0f)) &&
		!SelectedCloth->Paint.MaxDistanceValues.empty())
	{
		float MinZ = 0.0f;
		float MaxZ = 0.0f;
		bool bHasBounds = false;
		for (uint32 RenderVertexIndex : SelectedCloth->ParticleToRenderVertex)
		{
			if (RenderVertexIndex >= Asset->Vertices.size())
			{
				continue;
			}

			const float Z = Asset->Vertices[RenderVertexIndex].Position.Z;
			MinZ = bHasBounds ? std::min(MinZ, Z) : Z;
			MaxZ = bHasBounds ? std::max(MaxZ, Z) : Z;
			bHasBounds = true;
		}

		if (bHasBounds)
		{
			const float FreeValue = ClothBrushValue > 0.0f ? ClothBrushValue : 50.0f;
			const float PinThreshold = MinZ + (MaxZ - MinZ) * 0.8f;
			for (uint32 ParticleIndex = 0; ParticleIndex < SelectedCloth->ParticleToRenderVertex.size(); ++ParticleIndex)
			{
				if (ParticleIndex >= SelectedCloth->Paint.MaxDistanceValues.size())
				{
					break;
				}

				const uint32 RenderVertexIndex = SelectedCloth->ParticleToRenderVertex[ParticleIndex];
				if (RenderVertexIndex >= Asset->Vertices.size())
				{
					continue;
				}

				SelectedCloth->Paint.MaxDistanceValues[ParticleIndex] =
					Asset->Vertices[RenderVertexIndex].Position.Z >= PinThreshold ? 0.0f : FreeValue;
			}
			MarkCurrentMeshDirty();
		}
	}

	ImGui::SliderFloat("Smooth", &ClothBrushSmoothStrength, 0.0f, 1.0f, "%.2f");
	if (ImGui::Button("Smooth Paint", ImVec2(-1.0f, 0.0f)) &&
		!SelectedCloth->Paint.MaxDistanceValues.empty())
	{
		TArray<float> Smoothed = SelectedCloth->Paint.MaxDistanceValues;
		for (uint32 Index = 0; Index + 2 < SelectedCloth->ClothLocalIndices.size(); Index += 3)
		{
			const uint32 I0 = SelectedCloth->ClothLocalIndices[Index + 0];
			const uint32 I1 = SelectedCloth->ClothLocalIndices[Index + 1];
			const uint32 I2 = SelectedCloth->ClothLocalIndices[Index + 2];
			if (I0 >= Smoothed.size() || I1 >= Smoothed.size() || I2 >= Smoothed.size())
			{
				continue;
			}

			const float Average =
				(SelectedCloth->Paint.MaxDistanceValues[I0] +
				 SelectedCloth->Paint.MaxDistanceValues[I1] +
				 SelectedCloth->Paint.MaxDistanceValues[I2]) / 3.0f;
			Smoothed[I0] = Smoothed[I0] * (1.0f - ClothBrushSmoothStrength) + Average * ClothBrushSmoothStrength;
			Smoothed[I1] = Smoothed[I1] * (1.0f - ClothBrushSmoothStrength) + Average * ClothBrushSmoothStrength;
			Smoothed[I2] = Smoothed[I2] * (1.0f - ClothBrushSmoothStrength) + Average * ClothBrushSmoothStrength;
		}
		SelectedCloth->Paint.MaxDistanceValues = std::move(Smoothed);
		MarkCurrentMeshDirty();
	}

	if (!SelectedCloth->Paint.MaxDistanceValues.empty())
	{
		float MinValue = SelectedCloth->Paint.MaxDistanceValues[0];
		float MaxValue = SelectedCloth->Paint.MaxDistanceValues[0];
		for (float Value : SelectedCloth->Paint.MaxDistanceValues)
		{
			MinValue = std::min(MinValue, Value);
			MaxValue = std::max(MaxValue, Value);
		}
		ImGui::Text("Particles: %zu", SelectedCloth->Paint.MaxDistanceValues.size());
		ImGui::Text("Range: %.1f - %.1f", MinValue, MaxValue);

		const ImVec2 StripPos = ImGui::GetCursorScreenPos();
		const float StripWidth = ImGui::GetContentRegionAvail().x;
		const float StripHeight = 14.0f;
		ImDrawList* DrawList = ImGui::GetWindowDrawList();
		const uint32 SegmentCount = static_cast<uint32>(std::min<size_t>(SelectedCloth->Paint.MaxDistanceValues.size(), 96));
		const float Denom = std::max(1.0f, SelectedCloth->Paint.ViewMax - SelectedCloth->Paint.ViewMin);
		for (uint32 Segment = 0; Segment < SegmentCount; ++Segment)
		{
			const size_t SourceIndex = static_cast<size_t>(
				(static_cast<double>(Segment) / std::max(1u, SegmentCount - 1)) *
				(static_cast<double>(SelectedCloth->Paint.MaxDistanceValues.size() - 1))
			);
			const float T = std::clamp(
				(SelectedCloth->Paint.MaxDistanceValues[SourceIndex] - SelectedCloth->Paint.ViewMin) / Denom,
				0.0f,
				1.0f
			);
			const int Red = static_cast<int>(255.0f * T);
			const int Green = static_cast<int>(255.0f * (1.0f - std::fabs(T - 0.5f) * 2.0f));
			const int Blue = static_cast<int>(255.0f * (1.0f - T));
			const float X0 = StripPos.x + StripWidth * (static_cast<float>(Segment) / SegmentCount);
			const float X1 = StripPos.x + StripWidth * (static_cast<float>(Segment + 1) / SegmentCount);
			DrawList->AddRectFilled(ImVec2(X0, StripPos.y), ImVec2(X1, StripPos.y + StripHeight), IM_COL32(Red, Green, Blue, 255));
		}
		ImGui::Dummy(ImVec2(StripWidth, StripHeight + 4.0f));
	}
}

// Temporary ImGui overlay. Remove this whole function/call once cloth paint has
// a renderer-owned material/debug pass that can depth-test and cull correctly.
void FMeshEditorWidget::RenderTemporaryClothPaintValueOverlay(
	ImDrawList* DrawList,
	const ImVec2& ViewportPos,
	const ImVec2& ViewportSize) const
{
	if (!DrawList || ActiveTab != EMeshEditorTab::Mesh || !bShowClothPaintValues)
	{
		return;
	}

	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	FSkeletalMesh* Asset = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshAsset() : nullptr;
	if (!Asset)
	{
		return;
	}

	FSkeletalClothLODData* LODData = Asset->FindClothLOD(static_cast<uint32>(SelectedClothLODIndex));
	if (!LODData || SelectedClothIndex < 0 || SelectedClothIndex >= static_cast<int32>(LODData->Cloths.size()))
	{
		return;
	}

	const FSkeletalClothData& Cloth = LODData->Cloths[SelectedClothIndex];
	if (Cloth.Paint.MaxDistanceValues.empty() || Cloth.ParticleToRenderVertex.empty())
	{
		return;
	}

	USkeletalMeshComponent* PreviewMeshComponent = ViewportClient.GetPreviewMeshComponent();
	if (!PreviewMeshComponent)
	{
		return;
	}

	const TArray<FVertexPNCTT>& SkinnedVertices = PreviewMeshComponent->GetSkinnedVertices();
	if (SkinnedVertices.empty())
	{
		return;
	}

	FMinimalViewInfo POV;
	if (!ViewportClient.GetCameraView(POV))
	{
		return;
	}

	const FMatrix& WorldMatrix = PreviewMeshComponent->GetWorldMatrix();
	const float Denom = std::max(1.0f, Cloth.Paint.ViewMax - Cloth.Paint.ViewMin);
	const float MarkerRadius = 4.0f;

	for (uint32 ParticleIndex = 0; ParticleIndex < Cloth.ParticleToRenderVertex.size(); ++ParticleIndex)
	{
		if (ParticleIndex >= Cloth.Paint.MaxDistanceValues.size())
		{
			break;
		}

		const uint32 RenderVertexIndex = Cloth.ParticleToRenderVertex[ParticleIndex];
		if (RenderVertexIndex >= SkinnedVertices.size())
		{
			continue;
		}

		const float T = std::clamp(
			(Cloth.Paint.MaxDistanceValues[ParticleIndex] - Cloth.Paint.ViewMin) / Denom,
			0.0f,
			1.0f
		);
		const uint32 Red = static_cast<uint32>(255.0f * T);
		const uint32 Green = static_cast<uint32>(255.0f * (1.0f - std::fabs(T - 0.5f) * 2.0f));
		const uint32 Blue = static_cast<uint32>(255.0f * (1.0f - T));
		const ImU32 Color = IM_COL32(Red, Green, Blue, 235);
		const FVector WorldPos = WorldMatrix.TransformPositionWithW(SkinnedVertices[RenderVertexIndex].Position);

		ImVec2 ScreenPos;
		if (!ProjectWorldToViewport(WorldPos, POV, ViewportPos, ViewportSize, ScreenPos))
		{
			continue;
		}

		DrawList->AddCircleFilled(ScreenPos, MarkerRadius, Color, 10);
		DrawList->AddCircle(ScreenPos, MarkerRadius + 1.0f, IM_COL32(15, 15, 15, 220), 10, 1.0f);
		if (Cloth.Paint.MaxDistanceValues[ParticleIndex] <= 0.0f)
		{
			DrawList->AddCircle(ScreenPos, MarkerRadius + 2.0f, IM_COL32(255, 255, 255, 245), 10, 1.5f);
		}
	}
}

void FMeshEditorWidget::RenderClothBrushRadiusOverlay(
	ImDrawList* DrawList,
	const ImVec2& ViewportPos,
	const ImVec2& ViewportSize) const
{
	if (!DrawList ||
		ActiveTab != EMeshEditorTab::Mesh ||
		!bClothPaintBrushEnabled ||
		!ViewportClient.IsMouseOverViewport())
	{
		return;
	}

	USkeletalMeshComponent* PreviewMeshComponent = ViewportClient.GetPreviewMeshComponent();
	USkeletalMesh* SkeletalMesh = PreviewMeshComponent ? PreviewMeshComponent->GetSkeletalMesh() : nullptr;
	FSkeletalMesh* Asset = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshAsset() : nullptr;
	if (!PreviewMeshComponent || !Asset)
	{
		return;
	}

	const FSkeletalClothLODData* LODData = Asset->FindClothLOD(static_cast<uint32>(SelectedClothLODIndex));
	if (!LODData || SelectedClothIndex < 0 || SelectedClothIndex >= static_cast<int32>(LODData->Cloths.size()))
	{
		return;
	}

	const FSkeletalClothData& Cloth = LODData->Cloths[SelectedClothIndex];
	if (Cloth.Paint.MaxDistanceValues.empty() || Cloth.ParticleToRenderVertex.empty())
	{
		return;
	}

	FRay Ray;
	if (!ViewportClient.GetMouseRay(Ray))
	{
		return;
	}

	FHitResult Hit;
	if (!FRayUtils::RaycastComponent(PreviewMeshComponent, Ray, Hit) || Hit.FaceIndex < 0)
	{
		return;
	}

	const uint32 HitIndexOffset = static_cast<uint32>(Hit.FaceIndex);
	if (HitIndexOffset < Cloth.Binding.FirstIndex ||
		HitIndexOffset >= Cloth.Binding.FirstIndex + Cloth.Binding.IndexCount)
	{
		return;
	}

	FMinimalViewInfo POV;
	if (!ViewportClient.GetCameraView(POV))
	{
		return;
	}

	const FVector HitWorld = Ray.Origin + Ray.Direction * Hit.Distance;
	ImVec2 HitScreen;
	if (!ProjectWorldToViewport(HitWorld, POV, ViewportPos, ViewportSize, HitScreen))
	{
		return;
	}

	const float PixelRadius = ProjectWorldRadiusToPixels(
		HitWorld,
		std::max(0.0f, ClothBrushRadius),
		POV,
		ViewportPos,
		ViewportSize);

	const ImU32 RingColor = IM_COL32(255, 230, 90, 230);
	const ImU32 FillColor = IM_COL32(255, 230, 90, 28);
	const float DrawRadius = std::max(5.0f, PixelRadius);
	DrawList->AddCircleFilled(HitScreen, DrawRadius, FillColor, 48);
	DrawList->AddCircle(HitScreen, DrawRadius, RingColor, 48, 2.0f);
	DrawList->AddLine(
		ImVec2(HitScreen.x - 5.0f, HitScreen.y),
		ImVec2(HitScreen.x + 5.0f, HitScreen.y),
		RingColor,
		1.5f);
	DrawList->AddLine(
		ImVec2(HitScreen.x, HitScreen.y - 5.0f),
		ImVec2(HitScreen.x, HitScreen.y + 5.0f),
		RingColor,
		1.5f);
}

void FMeshEditorWidget::TickClothPaintBrush()
{
	if (!bClothPaintBrushEnabled || !ImGui::GetIO().MouseDown[0] || !ViewportClient.IsMouseOverViewport())
	{
		return;
	}

	USkeletalMeshComponent* PreviewMeshComponent = ViewportClient.GetPreviewMeshComponent();
	USkeletalMesh* SkeletalMesh = PreviewMeshComponent ? PreviewMeshComponent->GetSkeletalMesh() : nullptr;
	FSkeletalMesh* Asset = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshAsset() : nullptr;
	if (!PreviewMeshComponent || !Asset)
	{
		return;
	}

	FSkeletalClothLODData* LODData = Asset->FindClothLOD(static_cast<uint32>(SelectedClothLODIndex));
	if (!LODData || SelectedClothIndex < 0 || SelectedClothIndex >= static_cast<int32>(LODData->Cloths.size()))
	{
		return;
	}

	FSkeletalClothData& Cloth = LODData->Cloths[SelectedClothIndex];
	if (Cloth.Paint.MaxDistanceValues.empty() || Cloth.ParticleToRenderVertex.empty())
	{
		return;
	}

	FRay Ray;
	if (!ViewportClient.GetMouseRay(Ray))
	{
		return;
	}

	FHitResult Hit;
	if (!FRayUtils::RaycastComponent(PreviewMeshComponent, Ray, Hit) || Hit.FaceIndex < 0)
	{
		return;
	}

	const uint32 HitIndexOffset = static_cast<uint32>(Hit.FaceIndex);
	if (HitIndexOffset < Cloth.Binding.FirstIndex ||
		HitIndexOffset >= Cloth.Binding.FirstIndex + Cloth.Binding.IndexCount)
	{
		return;
	}

	bool bChanged = false;
	const float Radius = std::max(0.0f, ClothBrushRadius);
	if (Radius <= 0.0f)
	{
		for (uint32 Corner = 0; Corner < 3; ++Corner)
		{
			const uint32 RenderVertexIndex = Asset->Indices[HitIndexOffset + Corner];
			for (uint32 ParticleIndex = 0; ParticleIndex < Cloth.ParticleToRenderVertex.size(); ++ParticleIndex)
			{
				if (Cloth.ParticleToRenderVertex[ParticleIndex] == RenderVertexIndex &&
					ParticleIndex < Cloth.Paint.MaxDistanceValues.size())
				{
					Cloth.Paint.MaxDistanceValues[ParticleIndex] = ClothBrushValue;
					bChanged = true;
				}
			}
		}
	}
	else
	{
		const FVector HitWorld = Ray.Origin + Ray.Direction * Hit.Distance;
		const TArray<FVertexPNCTT>& SkinnedVertices = PreviewMeshComponent->GetSkinnedVertices();
		const FMatrix& WorldMatrix = PreviewMeshComponent->GetWorldMatrix();
		for (uint32 ParticleIndex = 0; ParticleIndex < Cloth.ParticleToRenderVertex.size(); ++ParticleIndex)
		{
			const uint32 RenderVertexIndex = Cloth.ParticleToRenderVertex[ParticleIndex];
			if (RenderVertexIndex >= SkinnedVertices.size() || ParticleIndex >= Cloth.Paint.MaxDistanceValues.size())
			{
				continue;
			}

			const FVector ParticleWorld = WorldMatrix.TransformPositionWithW(SkinnedVertices[RenderVertexIndex].Position);
			if (FVector::Distance(ParticleWorld, HitWorld) <= Radius)
			{
				Cloth.Paint.MaxDistanceValues[ParticleIndex] = ClothBrushValue;
				bChanged = true;
			}
		}
	}

	if (bChanged)
	{
		bMeshDirty = true;
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Animation tab
// ─────────────────────────────────────────────────────────────────────────────

void FMeshEditorWidget::ApplyAnimationToComponent()
{
	USkeletalMeshComponent* Comp = ViewportClient.GetPreviewMeshComponent();
	if (!Comp || !AnimTabState.CurrentSequence)
	{
		return;
	}
	Comp->PlayAnimation(AnimTabState.CurrentSequence, /*bLooping=*/true);
	Comp->SetPlaying(false);
	Comp->SetPlayRate(1.0f);
	ResetMorphPreviewOverrides();
}

void FMeshEditorWidget::EnsureMorphPreviewOverrideSize()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	FSkeletalMesh* MeshAsset    = SkeletalMesh ? SkeletalMesh->GetSkeletalMeshAsset() : nullptr;
	const size_t   MorphCount   = MeshAsset ? MeshAsset->MorphTargets.size() : 0;
	if (AnimTabState.MorphPreviewWeights.size() != MorphCount)
	{
		AnimTabState.MorphPreviewWeights.assign(MorphCount, 0.0f);
	}
	if (AnimTabState.MorphPreviewOverrideMask.size() != MorphCount)
	{
		AnimTabState.MorphPreviewOverrideMask.assign(MorphCount, 0);
	}
}

void FMeshEditorWidget::ResetMorphPreviewOverrides()
{
	AnimTabState.MorphPreviewWeights.clear();
	AnimTabState.MorphPreviewOverrideMask.clear();
	AnimTabState.bMorphPreviewOverrideEnabled = false;
}

void FMeshEditorWidget::ApplyMorphPreviewOverrides(TArray<float>& InOutMorphWeights) const
{
	if (!AnimTabState.bMorphPreviewOverrideEnabled)
	{
		return;
	}
	const size_t Count = AnimTabState.MorphPreviewWeights.size();
	if (Count == 0 || AnimTabState.MorphPreviewOverrideMask.size() != Count)
	{
		return;
	}
	if (InOutMorphWeights.size() < Count)
	{
		InOutMorphWeights.resize(Count, 0.0f);
	}
	for (size_t Index = 0; Index < Count; ++Index)
	{
		if (AnimTabState.MorphPreviewOverrideMask[Index] != 0)
		{
			InOutMorphWeights[Index] = AnimTabState.MorphPreviewWeights[Index];
		}
	}
}

void FMeshEditorWidget::RefreshAnimationPreviewPose()
{
	USkeletalMeshComponent* Comp = ViewportClient.GetPreviewMeshComponent();
	if (!Comp) return;
	UAnimSingleNodeInstance* NodeInst = Comp->GetAnimNodeInstance(FName::None);
	if (!NodeInst) return;
	USkeletalMesh* Mesh = Comp->GetSkeletalMesh();
	if (!Mesh) return;
	FSkeletalMesh* Asset = Mesh->GetSkeletalMeshAsset();
	if (!Asset || Asset->Bones.empty()) return;

	FPoseContext Out;
	Out.SkeletalMesh = Mesh;
	Out.Pose.resize(Asset->Bones.size());
	Out.ResetToRefPose();
	NodeInst->EvaluatePose(Out);
	ApplyMorphPreviewOverrides(Out.MorphWeights);
	Comp->SetAnimationPose(Out.Pose, Out.MorphWeights);
}

void FMeshEditorWidget::MarkAnimationListDirty()
{
	AnimTabState.bAnimationListDirty = true;
}

const TArray<FAssetListItem>& FMeshEditorWidget::GetCachedAnimationFilesForCurrentSkeleton()
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);
	FSkeletonBinding CurrentBinding;

	if (SkeletalMesh)
	{
		CurrentBinding = SkeletalMesh->GetSkeletonBinding();
	}
	else
	{
		CurrentBinding.Reset();
	}

	if (AnimTabState.bAnimationListDirty ||
		!IsSameSkeletonBindingForAnimationList(AnimTabState.CachedAnimationListBinding, CurrentBinding))
	{
		AnimTabState.CachedAnimationFiles.clear();
		AnimTabState.CachedMontageFiles.clear();
		AnimTabState.CachedAnimationListBinding = CurrentBinding;

		if (SkeletalMesh)
		{
			AnimTabState.CachedAnimationFiles = FAssetRegistry::ListAnimationsForSkeleton(CurrentBinding, false);
			AnimTabState.CachedMontageFiles   = FAssetRegistry::ListMontagesForSkeleton(CurrentBinding, false);
		}

		AnimTabState.bAnimationListDirty = false;
	}

	return AnimTabState.CachedAnimationFiles;
}

const TArray<FAssetListItem>& FMeshEditorWidget::GetCachedMontageFilesForCurrentSkeleton()
{
	// Animation 캐시와 같은 dirty/binding key 를 공유. 호출 순서는 animation → montage 가정.
	GetCachedAnimationFilesForCurrentSkeleton();
	return AnimTabState.CachedMontageFiles;
}

void FMeshEditorWidget::RenderAnimationLayout(float TotalHeight)
{
	USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject);

	constexpr float TimelineHeight = 210.0f;
	const float     ContentHeight  = TotalHeight - TimelineHeight - ImGui::GetStyle().ItemSpacing.y * 3.0f;

	// ─── Top: Asset Details | Viewport | Asset Browser (Persona 배치) ───

	// Left: 시퀀스 / 몽타주 디테일 패널 (선택 종류에 따라 분기)
	ImGui::BeginChild("AssetDetails", ImVec2(AnimTabState.AnimDetailsWidth, ContentHeight), true);
	if (AnimTabState.bMontageSelected && AnimTabState.CurrentMontage)
	{
		USkeletalMeshComponent* Comp = ViewportClient.GetPreviewMeshComponent();
		UAnimInstance* AnimInst = Comp ? Comp->GetAnimInstance() : nullptr;
		if (FAnimMontagePropertyPanel::Render(AnimTabState.CurrentMontage, Comp, AnimInst))
		{
			MarkCurrentAnimationDirty();
		}
	}
	else if (AnimTabState.CurrentSequence)
	{
		UAnimSequence* Seq = AnimTabState.CurrentSequence;
		// Notify entry 가 타임라인에서 선택되어 있으면 Notify 의 UPROPERTY 편집 UI 를 표시.
		// 아니면 기존 시퀀스 메타 + Root Motion 패널.
		const int32 NotifyCount = static_cast<int32>(Seq->GetNotifies().size());
		const bool bShowNotifyDetails =
			AnimTabState.SelectedNotifyIndex >= 0 &&
			AnimTabState.SelectedNotifyIndex < NotifyCount;
		const bool bShowMorphDetails = AnimTabState.SelectedMorphCurveIndex >= 0 && AnimTabState.SelectedMorphCurveIndex
		< static_cast<int32>(Seq->GetMorphTargetCurves().size());

		if (bShowNotifyDetails)
		{
			if (FAnimationTimelinePanel::RenderNotifyDetails(Seq, AnimTabState.SelectedNotifyIndex))
			{
				MarkCurrentAnimationDirty();
			}
		}
		else if (bShowMorphDetails)
		{
			if (FAnimationTimelinePanel::RenderMorphDetails(
				Seq,
				SkeletalMesh,
				AnimTabState.SelectedMorphCurveIndex,
				AnimTabState.SelectedMorphKeyIndex
			))
			{
				MarkCurrentAnimationDirty();
				RefreshAnimationPreviewPose();
			}
		}
		else
		{
			ImGui::TextUnformatted("Asset Details");
			ImGui::Separator();
			ImGui::Text("Name:   %s", Seq->GetName().c_str());
			ImGui::Text("Length: %.3f s", Seq->GetPlayLength());
			ImGui::Text("FPS:    %.1f", Seq->GetFrameRate());
			ImGui::Text("Frames: %d", Seq->GetNumberOfFrames());
			ImGui::Dummy(ImVec2(0, 6));
			const FString& Path = Seq->GetAssetPathFileName();
			if (!Path.empty() && Path != "None")
			{
				ImGui::TextWrapped("Path:\n%s", Path.c_str());
			}

			// AnimSequence property 패널 — root motion 등 편집 가능한 항목.
			ImGui::Dummy(ImVec2(0, 12));
			if (FAnimSequencePropertyPanel::Render(Seq))
			{
				MarkCurrentAnimationDirty();
				RefreshAnimationPreviewPose();
			}

			USkeletalMeshComponent* PreviewMeshComponent = ViewportClient.GetPreviewMeshComponent();
			USkeletalMesh* PreviewMesh = PreviewMeshComponent ? PreviewMeshComponent->GetSkeletalMesh() : SkeletalMesh;
			FSkeletalMesh* MeshAsset = PreviewMesh ? PreviewMesh->GetSkeletalMeshAsset() : nullptr;
			if (MeshAsset && !MeshAsset->MorphTargets.empty())
			{
				ImGui::Dummy(ImVec2(0, 12));
				ImGui::Separator();
				ImGui::TextUnformatted("Morph Preview / Keys");
				EnsureMorphPreviewOverrideSize();
				if (ImGui::SmallButton("Clear Morph Preview"))
				{
					ResetMorphPreviewOverrides();
					RefreshAnimationPreviewPose();
				}
				for (int32 MorphIndex = 0; MorphIndex < static_cast<int32>(MeshAsset->MorphTargets.size()); ++
				     MorphIndex)
				{
					const FMorphTarget& MorphTarget   = MeshAsset->MorphTargets[MorphIndex];
					float               CurrentWeight = 0.0f;
					if (MorphIndex < static_cast<int32>(AnimTabState.MorphPreviewWeights.size()) && AnimTabState.
						MorphPreviewOverrideMask[MorphIndex] != 0)
					{
						CurrentWeight = AnimTabState.MorphPreviewWeights[MorphIndex];
					}
					else if (PreviewMeshComponent)
					{
						CurrentWeight = PreviewMeshComponent->GetMorphTargetWeightByIndex(MorphIndex);
					}

					ImGui::PushID(MorphIndex);
					const char* Label = MorphTarget.Name.empty() ? "Unnamed" : MorphTarget.Name.c_str();
					if (ImGui::SliderFloat(Label, &CurrentWeight, -1.0f, 1.0f, "%.3f"))
					{
						AnimTabState.MorphPreviewWeights[MorphIndex]      = CurrentWeight;
						AnimTabState.MorphPreviewOverrideMask[MorphIndex] = 1;
						AnimTabState.bMorphPreviewOverrideEnabled         = true;
						RefreshAnimationPreviewPose();
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("Key"))
					{
						FMorphTargetCurve& Curve = FindOrAddMorphCurve(Seq, MorphTarget.Name);
						AddOrUpdateMorphCurveKey(
							Curve,
							PreviewMeshComponent && PreviewMeshComponent->GetAnimNodeInstance(FName::None)
							? PreviewMeshComponent->GetAnimNodeInstance(FName::None)->GetCurrentTime() : 0.0f,
							CurrentWeight
						);
						AnimTabState.MorphPreviewOverrideMask[MorphIndex] = 0;
						bool bAnyOverride                                 = false;
						for (uint8 Mask : AnimTabState.MorphPreviewOverrideMask)
						{
							if (Mask != 0)
							{
								bAnyOverride = true;
								break;
							}
						}
						AnimTabState.bMorphPreviewOverrideEnabled = bAnyOverride;
						MarkCurrentAnimationDirty();
						RefreshAnimationPreviewPose();
					}
					ImGui::PopID();
				}
			}
		}
	}
	else
	{
		ImGui::TextUnformatted("Asset Details");
		ImGui::Separator();
		ImGui::TextDisabled("No animation selected.");
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// Center: viewport
	ImGui::BeginGroup();
	{
		float  ViewportWidth = ImGui::GetContentRegionAvail().x - AnimTabState.AnimListWidth - ImGui::GetStyle().ItemSpacing.x;
		ImVec2 Size          = ImVec2(ViewportWidth, ContentHeight);
		RenderViewportPanel(Size);
	}
	ImGui::EndGroup();

	ImGui::SameLine();

	// Right: 에셋 브라우저 (애니메이션 목록)
	ImGui::BeginChild("AssetBrowser", ImVec2(AnimTabState.AnimListWidth, ContentHeight), true);
	ImGui::TextUnformatted("Asset Browser");
	ImGui::Separator();

	if (ImGui::Button("Load...", ImVec2(-1.0f, 0.0f)))
	{
		FEditorFileDialogOptions Opts;
		Opts.Filter                       = L"Animation Files (*.uasset)\0*.uasset\0All Files (*.*)\0*.*\0";
		Opts.Title                        = L"Load Animation";
		Opts.bReturnRelativeToProjectRoot = true;
		FString Path                      = FEditorFileUtils::OpenFileDialog(Opts);
		if (!Path.empty())
		{
			UAnimSequence* Seq = FAnimationManager::Get().LoadAnimation(Path);
			if (Seq && Seq->IsCompatibleWith(SkeletalMesh))
			{
				AnimTabState.CurrentSequence         = Seq;
				AnimTabState.SelectedAnimIndex       = -1;
				AnimTabState.SelectedNotifyIndex     = -1;
				AnimTabState.SelectedMorphCurveIndex = -1;
				AnimTabState.SelectedMorphKeyIndex   = -1;
				ApplyAnimationToComponent();
			}
		}
	}

	if (ImGui::Button("Import Animation FBX", ImVec2(-1.0f, 0.0f)))
	{
		FEditorFileDialogOptions Opts;
		Opts.Filter                       = L"FBX Files (*.fbx)\0*.fbx\0All Files (*.*)\0*.*\0";
		Opts.Title                        = L"Import Animation FBX";
		Opts.bReturnRelativeToProjectRoot = true;
		FString Path                      = FEditorFileUtils::OpenFileDialog(Opts);
		if (!Path.empty())
		{
			FFbxImportOptionsDialog::BeginAnimationImport(AnimTabState.AnimationImportDialog, Path);
		}
	}

	if (ImGui::Button("+ New Morph Animation", ImVec2(-1.0f, 0.0f)) && SkeletalMesh)
	{
		UAnimSequence*  Seq       = UObjectManager::Get().CreateObject<UAnimSequence>();
		UAnimDataModel* DataModel = UObjectManager::Get().CreateObject<UAnimDataModel>(Seq);
		DataModel->SetTiming(1.0f, 30.0f, 0);
		Seq->SetDataModel(DataModel);
		Seq->SetSkeletonBinding(SkeletalMesh->GetSkeletonBinding());
		Seq->SetFName(FName("MorphAnimation"));
		const FString AnimPath = FAnimationManager::GetAnimationPathForSkeleton(
			SkeletalMesh->GetAssetPathFileName(),
			"MorphAnimation",
			SkeletalMesh->GetSkeletonBinding().SkeletonPath
		);
		if (FAnimationManager::Get().SaveAnimation(Seq, AnimPath, SkeletalMesh->GetAssetPathFileName()))
		{
			AnimTabState.CurrentSequence         = Seq;
			AnimTabState.SelectedAnimIndex       = -1;
			AnimTabState.SelectedNotifyIndex     = -1;
			AnimTabState.SelectedMorphCurveIndex = -1;
			AnimTabState.SelectedMorphKeyIndex   = -1;
			ApplyAnimationToComponent();
			FAnimationManager::Get().RefreshAvailableAnimations();
			MarkAnimationListDirty();
		}
	}

	FAnimationImportRequest      AnimationImportRequest;
	const EFbxImportDialogResult AnimationImportDialogResult = FFbxImportOptionsDialog::RenderAnimationImportPopup(
		"Import Animation FBX Options",
		AnimTabState.AnimationImportDialog,
		SkeletalMesh ? SkeletalMesh->GetSkeletonBinding().SkeletonPath : FString("None"),
		AnimationImportRequest
	);

	if (AnimationImportDialogResult == EFbxImportDialogResult::Submitted)
	{
		TArray<UAnimSequence*> ImportedSequences;
		FAnimationManager::Get().ImportAnimationForSkeleton(AnimationImportRequest, &ImportedSequences);
		// 임포트 성공/스킵(이미 존재) 무관하게 디스크를 다시 스캔해 목록 갱신.
		FAnimationManager::Get().RefreshAvailableAnimations();
		MarkAnimationListDirty();
		if (!ImportedSequences.empty())
		{
			AnimTabState.CurrentSequence         = ImportedSequences[0];
			AnimTabState.SelectedAnimIndex       = -1;
			AnimTabState.SelectedNotifyIndex     = -1;
			AnimTabState.SelectedMorphCurveIndex = -1;
			AnimTabState.SelectedMorphKeyIndex   = -1;
			ApplyAnimationToComponent();
			FFbxImportOptionsDialog::RequestClose(AnimTabState.AnimationImportDialog);
		}
		else
		{
			AnimTabState.AnimationImportDialog.Error =
			"No animation was imported. Existing assets may have been skipped.";
		}
	}

	ImGui::Separator();

	if (ImGui::SmallButton("Refresh Animation List"))
	{
		FAnimationManager::Get().RefreshAvailableAnimations();
		FAnimationManager::Get().RefreshAvailableMontages();
		MarkAnimationListDirty();
	}

	// 디스크 스캔 — montage 목록 초기화 (최초 1회 + Refresh 시).
	static bool sMontagesScanned = false;
	if (!sMontagesScanned)
	{
		FAnimationManager::Get().RefreshAvailableMontages();
		sMontagesScanned = true;
	}

	const TArray<FAssetListItem>& AnimFiles     = GetCachedAnimationFilesForCurrentSkeleton();
	const TArray<FAssetListItem>& MontageFiles  = GetCachedMontageFilesForCurrentSkeleton();

	// asset 경로의 stem (확장자/디렉토리 제거) — 자동 montage 이름의 source 식별자.
	auto ExtractStem = [](const FString& Path) -> FString
	{
		const size_t LastSlash = Path.find_last_of("/\\");
		const size_t Start = (LastSlash == FString::npos) ? 0 : LastSlash + 1;
		const size_t LastDot = Path.find_last_of('.');
		const size_t End = (LastDot == FString::npos || LastDot < Start) ? Path.size() : LastDot;
		return Path.substr(Start, End - Start);
	};

	// + New Montage — 현재 선택된 sequence 가 있으면 source 로 새 montage 생성.
	// 이름은 sequence 의 asset path stem 사용 (UObject::GetName() 의 자동생성 ObjectName 회피).
	const bool bCanCreateMontage = (AnimTabState.CurrentSequence != nullptr) && !AnimTabState.bMontageSelected;
	if (!bCanCreateMontage) ImGui::BeginDisabled();
	if (ImGui::Button("+ New Montage (from selected sequence)", ImVec2(-1.0f, 0.0f)))
	{
		const FString Stem = ExtractStem(AnimTabState.CurrentSequence->GetAssetPathFileName());
		const FString MontageName = Stem + "_Montage";
		const FString PackagePath = FString("Content/Montages/") + MontageName + ".uasset";
		UAnimMontage* Montage = FAnimationManager::Get().CreateMontage(AnimTabState.CurrentSequence, MontageName);
		if (Montage)
		{
			FAnimationManager::Get().SaveMontage(Montage, PackagePath);
			FAnimationManager::Get().RefreshAvailableMontages();
			MarkAnimationListDirty();
			AnimTabState.CurrentMontage    = Montage;
			AnimTabState.bMontageSelected  = true;

			// 새 montage 의 인덱스 즉시 매핑 — list 의 hilight + 다음 클릭의 일관 동작 보장.
			// skeleton 필터링된 캐시 기준으로 인덱스를 잡아야 selectable 비교가 맞는다.
			const TArray<FAssetListItem>& Updated = GetCachedMontageFilesForCurrentSkeleton();
			AnimTabState.SelectedMontageIndex = -1;
			for (int32 j = 0; j < static_cast<int32>(Updated.size()); ++j)
			{
				if (Updated[j].FullPath == PackagePath)
				{
					AnimTabState.SelectedMontageIndex = j;
					break;
				}
			}
		}
	}
	if (!bCanCreateMontage) ImGui::EndDisabled();

	// 통합 리스트 — Sequence + Montage 한 selectable. 알파벳 정렬 (Walking_mixamo_com 옆에
	// Walking_mixamo_com_Montage 가 자연스럽게 인접). 시각 구분: Montage 는 노랑 + [M] prefix.
	struct FEntry
	{
		FString  DisplayName;
		FString  FullPath;
		bool     bIsMontage = false;
		int32    OriginalIndex = -1;   // AnimFiles 또는 MontageFiles 의 인덱스
	};
	TArray<FEntry> Entries;
	Entries.reserve(AnimFiles.size() + MontageFiles.size());
	for (int32 i = 0; i < static_cast<int32>(AnimFiles.size());    ++i) Entries.push_back({ AnimFiles[i].DisplayName,    AnimFiles[i].FullPath,    false, i });
	for (int32 i = 0; i < static_cast<int32>(MontageFiles.size()); ++i) Entries.push_back({ MontageFiles[i].DisplayName, MontageFiles[i].FullPath, true,  i });
	std::sort(Entries.begin(), Entries.end(),
		[](const FEntry& A, const FEntry& B) { return A.DisplayName < B.DisplayName; });

	ImGui::TextUnformatted("Animations & Montages");
	for (const FEntry& E : Entries)
	{
		const bool bSelected =
			E.bIsMontage
				? (AnimTabState.bMontageSelected && AnimTabState.SelectedMontageIndex == E.OriginalIndex)
				: (!AnimTabState.bMontageSelected && AnimTabState.SelectedAnimIndex == E.OriginalIndex);

		// 시각 구분 — Montage 는 노랑 톤. Sequence 는 기본 색.
		const ImU32 Color = E.bIsMontage ? IM_COL32(255, 200, 100, 255) : IM_COL32(255, 255, 255, 255);
		ImGui::PushStyleColor(ImGuiCol_Text, Color);

		const FString Label = (E.bIsMontage ? "[M] " : "      ") + E.DisplayName;
		if (ImGui::Selectable(Label.c_str(), bSelected))
		{
			if (E.bIsMontage)
			{
				AnimTabState.SelectedMontageIndex    = E.OriginalIndex;
				AnimTabState.bMontageSelected        = true;
				AnimTabState.SelectedNotifyIndex     = -1;
				AnimTabState.SelectedMorphCurveIndex = -1;
				AnimTabState.SelectedMorphKeyIndex   = -1;
				ResetMorphPreviewOverrides();
				if (UAnimMontage* M = FAnimationManager::Get().LoadMontage(E.FullPath))
				{
					AnimTabState.CurrentMontage = M;
				}
			}
			else
			{
				AnimTabState.SelectedAnimIndex       = E.OriginalIndex;
				AnimTabState.bMontageSelected        = false;
				AnimTabState.SelectedNotifyIndex     = -1;
				AnimTabState.SelectedMorphCurveIndex = -1;
				AnimTabState.SelectedMorphKeyIndex   = -1;
				if (UAnimSequence* Seq = FAnimationManager::Get().LoadAnimation(E.FullPath))
				{
					if (Seq->IsCompatibleWith(SkeletalMesh))
					{
						AnimTabState.CurrentSequence = Seq;
						ApplyAnimationToComponent();
					}
				}
			}
		}
		ImGui::PopStyleColor();

		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("%s\n%s", E.bIsMontage ? "Montage" : "Sequence", E.FullPath.c_str());
		}
	}
	ImGui::EndChild();

	// ─── Bottom: Unreal 시퀀서 패널 ───
	UAnimSingleNodeInstance* NodeInst = nullptr;
	USkeletalMeshComponent*  Comp     = ViewportClient.GetPreviewMeshComponent();
	if (Comp && AnimTabState.CurrentSequence)
	{
		NodeInst = Comp->GetAnimNodeInstance(FName::None);
	}

	// 스페이스바: 재생/정지 토글 (메시 에디터 창 포커스 + 텍스트 입력 중 아닐 때)
	if (Comp && ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows) &&
	    !ImGui::GetIO().WantTextInput &&
	    ImGui::IsKeyPressed(ImGuiKey_Space, false))
	{
		const bool bPlaying = NodeInst && NodeInst->IsPlaying();
		Comp->SetPlaying(!bPlaying);
	}

	if (FAnimationTimelinePanel::Render(NodeInst, Comp, AnimTabState.CurrentSequence, TimelineHeight,
		AnimTabState.SelectedNotifyIndex,
		AnimTabState.SelectedMorphCurveIndex,
		AnimTabState.SelectedMorphKeyIndex
	))
	{
		if (AnimTabState.CurrentSequence)
		{
			AnimTabState.DirtySequences.insert(AnimTabState.CurrentSequence);
		}
		RefreshAnimationPreviewPose();
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// Mesh stats overlay
// ─────────────────────────────────────────────────────────────────────────────

void FMeshEditorWidget::RenderMeshStatsOverlay(ImDrawList* DrawList, const ImVec2& ViewportPos) const
{
	if (!DrawList || !EditedObject)
	{
		return;
	}

	size_t VertexCount   = 0;
	size_t TriangleCount = 0;
	size_t IndexCount    = 0;
	double ImportSeconds = -1.0;

	if (const USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(EditedObject))
	{
		if (const FSkeletalMesh* Asset = SkeletalMesh->GetSkeletalMeshAsset())
		{
			VertexCount   = Asset->Vertices.size();
			IndexCount    = Asset->Indices.size();
			TriangleCount = Asset->Indices.size() / 3;
		}
		ImportSeconds = GetRecordedImportDurationSeconds(SkeletalMesh);
	}

	FString Text =
		"Triangles: " + FormatMeshStatCount(TriangleCount) + "\n" +
		"Vertices: " + FormatMeshStatCount(VertexCount) + "\n" +
		"Indices: " + FormatMeshStatCount(IndexCount);

	if (ImportSeconds >= 0.0)
	{
		Text += "\nImport Time: " + FormatMeshStatSeconds(ImportSeconds);
	}

	const ImVec2 TextPos(ViewportPos.x + 8.0f, ViewportPos.y + 36.0f);
	DrawList->AddText(ImVec2(TextPos.x + 1.0f, TextPos.y + 1.0f), IM_COL32(0, 0, 0, 220), Text.c_str());
	DrawList->AddText(TextPos, IM_COL32(235, 238, 242, 255), Text.c_str());
}

// ─────────────────────────────────────────────────────────────────────────────
// Bone tree (Skeleton tab)
// ─────────────────────────────────────────────────────────────────────────────

void FMeshEditorWidget::RenderBoneTree(USkeleton* Skeleton, const FReferenceSkeleton& RefSkeleton, int32 Index)
{
	const FReferenceBone& Bone = RefSkeleton.Bones[Index];

	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_DefaultOpen;

	if (Index == SelectedBoneIndex && SelectedSocketIndex < 0)
	{
		Flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool bHasChildren = false;
	for (int32 i = Index + 1; i < RefSkeleton.GetNumBones(); ++i)
	{
		if (RefSkeleton.Bones[i].ParentIndex == Index)
		{
			bHasChildren = true;
			break;
		}
	}

	bool bHasSockets = false;
	if (Skeleton)
	{
		const FName BoneName(Bone.Name);
		for (const FSkeletalMeshSocket& Socket : Skeleton->GetSockets())
		{
			if (Socket.BoneName == BoneName)
			{
				bHasSockets = true;
				break;
			}
		}
	}

	if (!bHasChildren && !bHasSockets)
	{
		Flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	bool bOpen = ImGui::TreeNodeEx(Bone.Name.c_str(), Flags);

	if (ImGui::IsItemClicked())
	{
		SelectedBoneIndex = Index;
		SelectedSocketIndex = -1;
		ViewportClient.SetSelectedBone(Cast<USkeletalMesh>(EditedObject), Index);
	}

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Add Socket") && Skeleton)
		{
			FSkeletalMeshSocket Socket;
			Socket.Name = FName(GenerateUniqueSocketName(Skeleton, "Socket"));
			Socket.BoneName = FName(Bone.Name);
			Skeleton->GetMutableSockets().push_back(Socket);
			SelectedBoneIndex = -1;
			SelectedSocketIndex = static_cast<int32>(Skeleton->GetSockets().size()) - 1;
			BufferedSocketIndex = -2;
			ViewportClient.SetSelectedSocket(Cast<USkeletalMesh>(EditedObject), Skeleton, SelectedSocketIndex);
			bSkeletonDirty = true;
		}
		ImGui::EndPopup();
	}

	if (bOpen && (bHasChildren || bHasSockets))
	{
		for (int32 SocketIndex = 0; Skeleton && SocketIndex < static_cast<int32>(Skeleton->GetSockets().size()); ++SocketIndex)
		{
			if (Skeleton->GetSockets()[SocketIndex].BoneName == FName(Bone.Name))
			{
				RenderSocketTreeNode(Skeleton, SocketIndex);
			}
		}

		for (int32 i = Index + 1; i < RefSkeleton.GetNumBones(); ++i)
		{
			if (RefSkeleton.Bones[i].ParentIndex == Index)
			{
				RenderBoneTree(Skeleton, RefSkeleton, i);
			}
		}
		ImGui::TreePop();
	}
}

void FMeshEditorWidget::RenderSocketTreeNode(USkeleton* Skeleton, int32 SocketIndex)
{
	if (!Skeleton || SocketIndex < 0 || SocketIndex >= static_cast<int32>(Skeleton->GetSockets().size()))
	{
		return;
	}

	const FSkeletalMeshSocket& Socket = Skeleton->GetSockets()[SocketIndex];
	ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (SocketIndex == SelectedSocketIndex)
	{
		Flags |= ImGuiTreeNodeFlags_Selected;
	}

	const FString Label = Socket.Name.ToString() + "##Socket" + std::to_string(SocketIndex);
	ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(170, 170, 255, 255));
	ImGui::TreeNodeEx(Label.c_str(), Flags);
	ImGui::PopStyleColor();
	if (ImGui::IsItemClicked())
	{
		SelectedBoneIndex = -1;
		SelectedSocketIndex = SocketIndex;
		BufferedSocketIndex = -2;
		ViewportClient.SetSelectedSocket(Cast<USkeletalMesh>(EditedObject), Skeleton, SocketIndex);
	}

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Delete Socket"))
		{
			TArray<FSkeletalMeshSocket>& Sockets = Skeleton->GetMutableSockets();
			Sockets.erase(Sockets.begin() + SocketIndex);
			SelectedSocketIndex = -1;
			BufferedSocketIndex = -2;
			ViewportClient.SetSelectedBone(Cast<USkeletalMesh>(EditedObject), -1);
			bSkeletonDirty = true;
		}
		ImGui::EndPopup();
	}
}
