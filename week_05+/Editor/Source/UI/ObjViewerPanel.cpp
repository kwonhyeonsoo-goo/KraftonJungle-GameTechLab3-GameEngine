#include "ObjViewerPanel.h"

#include "Actor/Actor.h"
#include "Actor/StaticMeshActor.h"
#include "Asset/AssetCooker.h"
#include "Asset/AssetManager.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SceneComponent.h"
#include "Component/StaticMeshComponent.h"
#include "Core/Core.h"
#include "Core/Paths.h"
#include "Debug/EngineLog.h"
#include "Mesh/ObjManager.h"
#include "Mesh/StaticMeshRenderData.h"
#include "Object/StaticMesh.h"
#include "Renderer/Renderer.h"
#include "UI/EditorUI.h"
#include "UI/EditorViewportClient.h"
#include "World/Level.h"
#include "imgui.h"

#include <cmath>
#include <filesystem>

namespace
{
	constexpr float ObjViewerImportScaleTolerance = 1.0e-4f;

	/**
	 * 축 매핑 UI에서 사용할 축 방향 라벨을 반환합니다.
	 * 축 방향에 따라 ImGUI에서 출력할 문자열을 반환합니다.
	 *
	 * \param AxisDirection 현재 표시할 축 방향 enum
	 * \return `+X`, `-X`, `+Y`, `-Y`, `+Z`, `-Z` 중 하나
	 */
	const char* GetAxisDirectionLabel(FObjImporter::EAxisDirection AxisDirection)
	{
		switch (AxisDirection)
		{
		case FObjImporter::EAxisDirection::PositiveX:
			return "+X";
		case FObjImporter::EAxisDirection::NegativeX:
			return "-X";
		case FObjImporter::EAxisDirection::PositiveY:
			return "+Y";
		case FObjImporter::EAxisDirection::NegativeY:
			return "-Y";
		case FObjImporter::EAxisDirection::PositiveZ:
			return "+Z";
		case FObjImporter::EAxisDirection::NegativeZ:
		default:
			return "-Z";
		}
	}

	/**
	 * 축 방향 enum에서 축의 종류를 추출합니다.
	 * 이 패널은 축의 종류(X/Y/Z)와 부호(+/-)를 따로 다룹니다.
	 *
	 * \param AxisDirection 축 방향 enum
	 * \return X=0, Y=1, Z=2
	 */
	int GetAxisBaseIndex(FObjImporter::EAxisDirection AxisDirection)
	{
		switch (AxisDirection)
		{
		case FObjImporter::EAxisDirection::PositiveX:
		case FObjImporter::EAxisDirection::NegativeX:
			return 0;
		case FObjImporter::EAxisDirection::PositiveY:
		case FObjImporter::EAxisDirection::NegativeY:
			return 1;
		case FObjImporter::EAxisDirection::PositiveZ:
		case FObjImporter::EAxisDirection::NegativeZ:
		default:
			return 2;
		}
	}

	/**
	 * 축 방향이 음수 방향인지 판별합니다.
	 * 축 교환 버튼과 부호 토글 버튼은 서로 다른 역할을 가지므로,
	 * 현재 방향에서 부호만 유지하거나 뒤집을 때 이 helper를 사용합니다.
	 *
	 * \param AxisDirection 축 방향 enum
	 * \return 음수 축이면 true
	 */
	bool IsAxisNegative(FObjImporter::EAxisDirection AxisDirection)
	{
		switch (AxisDirection)
		{
		case FObjImporter::EAxisDirection::NegativeX:
		case FObjImporter::EAxisDirection::NegativeY:
		case FObjImporter::EAxisDirection::NegativeZ:
			return true; //negative
		case FObjImporter::EAxisDirection::PositiveX:
		case FObjImporter::EAxisDirection::PositiveY:
		case FObjImporter::EAxisDirection::PositiveZ:
		default:
			return false; //positive
		}
	}

	/**
	 * 축 종류(X/Y/Z)와 부호(+/-)를 조합해 최종 축 방향 enum을 만듭니다.
	 *
	 * \param AxisBaseIndex X=0, Y=1, Z=2
	 * \param bNegative 음수 방향 여부
	 * \return 조합된 축 방향 enum
	 */
	FObjImporter::EAxisDirection MakeAxisDirection(int AxisBaseIndex, bool bNegative)
	{
		switch (AxisBaseIndex)
		{
		case 0:
			return bNegative ? FObjImporter::EAxisDirection::NegativeX : FObjImporter::EAxisDirection::PositiveX;
		case 1:
			return bNegative ? FObjImporter::EAxisDirection::NegativeY : FObjImporter::EAxisDirection::PositiveY;
		case 2:
		default:
			return bNegative ? FObjImporter::EAxisDirection::NegativeZ : FObjImporter::EAxisDirection::PositiveZ;
		}
	}

	/**
	 * 드래프트 축 매핑의 특정 행이 현재 어떤 축 방향을 가리키는지 읽습니다.
	 * 패널은 `Engine X / Engine Y / Engine Z` 세 행을 같은 코드로 반복 렌더링합니다.
	 * 패널에서 3번 호출됩니다.
	 *
	 * \param ImportAxisMapping 현재 드래프트 매핑
	 * \param RowIndex 0=X, 1=Y, 2=Z
	 * \return 해당 행의 축 방향 enum
	 */
	FObjImporter::EAxisDirection GetMappingAxisDirection(const FObjImporter::FImportAxisMapping& ImportAxisMapping, int RowIndex)
	{
		switch (RowIndex)
		{
		case 0:
			return ImportAxisMapping.EngineX;
		case 1:
			return ImportAxisMapping.EngineY;
		case 2:
		default:
			return ImportAxisMapping.EngineZ;
		}
	}

	/**
	 * 드래프트 축 매핑의 특정 행 값을 새 축 방향으로 갱신합니다.
	 *
	 * \param ImportAxisMapping 수정할 드래프트 매핑
	 * \param RowIndex 0=X, 1=Y, 2=Z
	 * \param AxisDirection 새 축 방향 enum
	 */
	void SetMappingAxisDirection(FObjImporter::FImportAxisMapping& ImportAxisMapping, int RowIndex, FObjImporter::EAxisDirection AxisDirection)
	{
		switch (RowIndex)
		{
		case 0:
			ImportAxisMapping.EngineX = AxisDirection;
			break;
		case 1:
			ImportAxisMapping.EngineY = AxisDirection;
			break;
		case 2:
		default:
			ImportAxisMapping.EngineZ = AxisDirection;
			break;
		}
	}

	/**
	 * 드래프트 매핑의 한 행을 다른 축 종류로 바꾸면서, 축 중복을 자동으로 해소합니다.
	 *
	 * 이 패널은 항상 X/Y/Z가 서로 다른 원본 축을 사용하도록 유지해야 합니다.
	 * 그래서 어떤 행이 이미 다른 행이 쓰고 있는 축을 선택하면,
	 * 기존 사용 행과 현재 행의 축 종류를 swap하여 유효한 상태를 유지합니다.
	 *
	 * \param ImportAxisMapping 수정할 드래프트 매핑
	 * \param RowIndex 축 종류를 바꿀 행
	 * \param NewAxisBaseIndex 새 축 종류(X=0, Y=1, Z=2)
	 */
	void SetMappingAxisBase(FObjImporter::FImportAxisMapping& ImportAxisMapping, int RowIndex, int NewAxisBaseIndex)
	{
		const FObjImporter::EAxisDirection CurrentDirection = GetMappingAxisDirection(ImportAxisMapping, RowIndex);
		const bool bCurrentNegative = IsAxisNegative(CurrentDirection);

		for (int OtherRowIndex = 0; OtherRowIndex < 3; ++OtherRowIndex)
		{
			if (OtherRowIndex == RowIndex)
			{
				continue;
			}

			const FObjImporter::EAxisDirection OtherDirection = GetMappingAxisDirection(ImportAxisMapping, OtherRowIndex);
			if (GetAxisBaseIndex(OtherDirection) == NewAxisBaseIndex)
			{
				// 같은 축을 이미 쓰고 있던 행은 기존 축으로 밀어내 중복을 없앱니다.
				const int CurrentAxisBaseIndex = GetAxisBaseIndex(CurrentDirection);
				SetMappingAxisDirection(ImportAxisMapping, OtherRowIndex, MakeAxisDirection(CurrentAxisBaseIndex, IsAxisNegative(OtherDirection)));
				break;
			}
		}

		SetMappingAxisDirection(ImportAxisMapping, RowIndex, MakeAxisDirection(NewAxisBaseIndex, bCurrentNegative));
	}

	/* 특정 행의 부호만 반전합니다. */
	void ToggleMappingAxisSign(FObjImporter::FImportAxisMapping& ImportAxisMapping, int RowIndex)
	{
		const FObjImporter::EAxisDirection CurrentDirection = GetMappingAxisDirection(ImportAxisMapping, RowIndex);
		SetMappingAxisDirection(ImportAxisMapping, RowIndex, MakeAxisDirection(GetAxisBaseIndex(CurrentDirection), !IsAxisNegative(CurrentDirection)));
	}

	/**
	 * 현재 적용된 축 매핑을 한 줄 요약 문자열로 만듭니다.
	 * 패널 상단의 `Applied` 섹션에서 사용되며,
	 * 사용자가 지금 실제로 메시 cook에 반영된 값이 무엇인지 즉시 확인할 수 있게 합니다.
	 *
	 * \param ImportAxisMapping 요약할 축 매핑
	 * \return 사람이 읽기 쉬운 요약 문자열
	 */
	FString BuildImportAxisSummary(const FObjImporter::FImportAxisMapping& ImportAxisMapping)
	{
		return FString("X=") + GetAxisDirectionLabel(ImportAxisMapping.EngineX)
			+ "  Y=" + GetAxisDirectionLabel(ImportAxisMapping.EngineY)
			+ "  Z=" + GetAxisDirectionLabel(ImportAxisMapping.EngineZ)
			+ "  Scale=" + std::to_string(ImportAxisMapping.ImportScale);
	}

	/* 현재 OBJ Viewer 레벨에서 표시 중인 메인 StaticMeshActor를 찾습니다. */
	AStaticMeshActor* FindObjViewerMeshActor(ULevel* Level)
	{
		if (!Level)
		{
			return nullptr;
		}

		for (AActor* Actor : Level->GetActors())
		{
			if (Actor && Actor->IsA(AStaticMeshActor::StaticClass()))
			{
				return static_cast<AStaticMeshActor*>(Actor);
			}
		}

		return nullptr;
	}

	/**
	 * 액터의 바닥면이 원하는 Z 높이에 오도록 루트 위치를 보정합니다.
	 * 현재 bounds 하단이 목표 Z에 오도록 위치를 재계산해야 합니다.
	 * 보정 후에는 orbit 카메라 기준점도 다시 갱신해 카메라와 메시 중심이 어긋나지 않게 합니다.
	 *
	 * \param Actor 바닥 높이를 맞출 액터
	 * \param ViewportClient bounds와 카메라 pivot 갱신에 사용할 뷰포트
	 * \param TargetBottomZ 액터 바닥면이 도달해야 할 목표 Z
	 */
	void SnapObjViewerActorBottomTo(AActor* Actor, FEditorViewportClient* ViewportClient, float TargetBottomZ)
	{
		if (!Actor || !ViewportClient)
		{
			return;
		}

		USceneComponent* Root = Actor->GetRootComponent();
		if (!Root)
		{
			return;
		}

		FTransform Transform = Root->GetRelativeTransform();
		FVector ActualLocation = Transform.GetLocation();
		ActualLocation.Z += TargetBottomZ - ViewportClient->GetObjViewerBottomZ(Actor);
		Transform.SetLocation(ActualLocation);
		Root->SetRelativeTransform(Transform);
		ViewportClient->RefreshObjViewerCameraPivot(Actor);
	}
}

/**
 * 프로퍼티 패널에 표시할 뷰어용 위치 값을 계산합니다.
 * 뷰어는 사용자가 "바닥 기준 Z"를 조절하도록 되어 있으므로 표시 값이 다릅니다.
 *
 * \param Actor 표시 위치를 구할 액터
 * \param ViewportClient 바닥 Z 계산에 사용할 뷰포트
 * \return 프로퍼티 UI에 보여줄 위치 값
 */
FVector FObjViewerPanel::GetDisplayedLocation(AActor* Actor, FEditorViewportClient* ViewportClient) const
{
	if (!Actor || !ViewportClient)
	{
		return FVector::ZeroVector;
	}

	if (USceneComponent* Root = Actor->GetRootComponent())
	{
		FVector DisplayLocation = Root->GetRelativeTransform().GetLocation();
		DisplayLocation.Z = ViewportClient->GetObjViewerBottomZ(Actor);
		return DisplayLocation;
	}

	return FVector::ZeroVector;
}

/**
 * 프로퍼티 패널에서 수정된 "표시용 위치"를 실제 액터 트랜스폼에 반영합니다.
 * X/Y는 일반 위치처럼 직접 반영하지만, Z는 뷰어 규칙상 바닥 높이로 해석됩니다.
 *
 * \param Actor 위치를 적용할 액터
 * \param ViewportClient 바닥 보정과 pivot 갱신에 사용할 뷰포트
 * \param DisplayLocation 프로퍼티 패널이 전달한 표시용 위치
 */
void FObjViewerPanel::ApplyDisplayedLocation(AActor* Actor, FEditorViewportClient* ViewportClient, const FVector& DisplayLocation) const
{
	if (!Actor)
	{
		return;
	}

	if (USceneComponent* Root = Actor->GetRootComponent())
	{
		FTransform Transform = Root->GetRelativeTransform();
		Transform.SetLocation({ DisplayLocation.X, DisplayLocation.Y, Transform.GetLocation().Z });
		Root->SetRelativeTransform(Transform);

		if (ViewportClient)
		{
			SnapObjViewerActorBottomTo(Actor, ViewportClient, DisplayLocation.Z);
		}
	}
}

/**
 * OBJ Viewer 전용 정보 패널 전체를 렌더링합니다.
 *
 * 1. 현재 표시 중인 메시의 메타데이터와 통계를 보여줍니다.
 * 2. 현재 적용 중인 축 매핑과 드래프트 축 매핑을 구분해 보여줍니다.
 * 3. 사용자가 드래프트를 적용하면 메시를 다시 cook/load하여 결과를 즉시 확인하게 합니다.
 *
 * 함수 초반에서는 현재 뷰어 컨텍스트와 메시 데이터를 안전하게 확보하고,
 * 중간에서는 ImGui 섹션을 그리며,
 * 마지막 `Apply` 버튼에서는 축 매핑 적용, 캐시 무효화, cooked asset 제거,
 * 메시 재로드, 위치/카메라 복원까지 한 번에 처리합니다.
 *
 * \param Core 현재 에디터 엔진 코어
 * \param ActiveViewportClient 현재 활성 뷰어 뷰포트
 * \param EditorUI 선택 액터 동기화 호출에 사용할 상위 UI 객체
 */
void FObjViewerPanel::Render(FCore* Core, FEditorViewportClient* ActiveViewportClient, FEditorUI& EditorUI)
{
	if (!Core || !ActiveViewportClient || !GRenderer) return;
	ULevel* Level = ActiveViewportClient->ResolveLevel(Core);
	if (!Level) return;
	AStaticMeshActor* MeshActor = FindObjViewerMeshActor(Level);
	if (!MeshActor) return;
	UStaticMeshComponent* MeshComp = MeshActor->GetStaticMeshComponent();
	if (!MeshComp) return;
	UStaticMesh* StaticMesh = MeshComp->GetStaticMesh();
	if (!StaticMesh) return;
	FStaticMeshRenderData* MeshData = StaticMesh->GetStaticMeshAsset();
	if (!MeshData) return;
	FMeshData* CPUData = MeshData->GetMeshData();
	if (!CPUData) return;

	ImGui::SetNextWindowPos(ImVec2(10, 30), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);

	ImGuiWindowFlags WindowFlags = ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize;
	if (!ImGui::Begin("OBJ Information", nullptr, WindowFlags))
	{
		ImGui::End();
		return;
	}

	ImVec4 HeaderColor = ImVec4(0.4f, 0.7f, 1.0f, 1.0f);
	ImVec4 LabelColor = ImVec4(0.7f, 0.7f, 0.7f, 1.0f);
	ImVec4 ValueColor = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	const FObjImporter::FImportAxisMapping AppliedImportAxisMapping = FObjImporter::GetImportAxisMapping();
	const FString MeshAssetPath = MeshData->GetAssetPath();

	const bool bAppliedAxisMappingChanged =
		LastAppliedImportAxisMapping.EngineX != AppliedImportAxisMapping.EngineX ||
		LastAppliedImportAxisMapping.EngineY != AppliedImportAxisMapping.EngineY ||
		LastAppliedImportAxisMapping.EngineZ != AppliedImportAxisMapping.EngineZ ||
		!(std::fabs(LastAppliedImportAxisMapping.ImportScale - AppliedImportAxisMapping.ImportScale) <= ObjViewerImportScaleTolerance);
	if (DraftAxisAssetPath != MeshAssetPath || bAppliedAxisMappingChanged)
	{
		// 다른 메시를 보고 있거나 외부에서 적용 값이 바뀌었으면 드래프트를 현재 상태로 재동기화합니다.
		DraftAxisAssetPath = MeshAssetPath;
		DraftImportAxisMapping = AppliedImportAxisMapping;
		LastAppliedImportAxisMapping = AppliedImportAxisMapping;
	}

	if (ImGui::CollapsingHeader("Asset Information", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		ImGui::TextColored(LabelColor, "Path:");
		ImGui::SameLine();
		ImGui::TextWrapped("%s", MeshData->GetAssetPath().c_str());
		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("Mesh Statistics", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		auto StatRow = [&](const char* Label, int Value)
			{
				ImGui::TextColored(LabelColor, "%s:", Label);
				ImGui::SameLine(120);
				ImGui::TextColored(ValueColor, "%d", Value);
			};

		StatRow("Vertices", static_cast<int>(CPUData->Vertices.size()));
		StatRow("Indices", static_cast<int>(CPUData->Indices.size()));
		StatRow("Triangles", static_cast<int>(CPUData->Indices.size()) / 3);
		StatRow("Sections", static_cast<int>(CPUData->Sections.size()));
		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("Bounding Box", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		FVector Min = CPUData->GetMinCoord();
		FVector Max = CPUData->GetMaxCoord();
		FVector Center = CPUData->GetCenterCoord();

		auto VectorRow = [&](const char* Label, const FVector& V)
			{
				ImGui::TextColored(LabelColor, "%s:", Label);
				ImGui::SameLine(80);
				ImGui::TextColored(ValueColor, "(%.2f, %.2f, %.2f)", V.X, V.Y, V.Z);
			};

		VectorRow("Min", Min);
		VectorRow("Max", Max);
		VectorRow("Center", Center);

		ImGui::TextColored(LabelColor, "Size:");
		ImGui::SameLine(80);
		ImGui::TextColored(ValueColor, "(%.2f, %.2f, %.2f)", Max.X - Min.X, Max.Y - Min.Y, Max.Z - Min.Z);

		ImGui::TextColored(LabelColor, "Radius:");
		ImGui::SameLine(80);
		ImGui::TextColored(ValueColor, "%.2f", CPUData->GetLocalBoundRadius());
		ImGui::Unindent();
	}

	if (ImGui::CollapsingHeader("OBJ Axis Mapping", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent();
		ImGui::TextColored(HeaderColor, "Applied");
		ImGui::TextWrapped("%s", BuildImportAxisSummary(AppliedImportAxisMapping).c_str());
		ImGui::SeparatorText("Draft");

		const char* RowLabels[] = { "Engine X", "Engine Y", "Engine Z" };
		const char* AxisBaseLabels[] = { "X", "Y", "Z" };
		for (int RowIndex = 0; RowIndex < 3; ++RowIndex)
		{
			FObjImporter::EAxisDirection RowDirection = GetMappingAxisDirection(DraftImportAxisMapping, RowIndex);
			ImGui::PushID(RowIndex);
			ImGui::TextColored(LabelColor, "%s:", RowLabels[RowIndex]);

			for (int AxisBaseIndex = 0; AxisBaseIndex < 3; ++AxisBaseIndex)
			{
				ImGui::SameLine(AxisBaseIndex == 0 ? 100.0f : 0.0f);
				if (AxisBaseIndex > 0)
				{
					ImGui::SameLine();
				}

				const bool bSelectedBase = GetAxisBaseIndex(RowDirection) == AxisBaseIndex;
				if (bSelectedBase)
				{
					ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.25f, 0.45f, 0.85f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.35f, 0.55f, 0.95f, 1.0f));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.20f, 0.40f, 0.80f, 1.0f));
				}
				if (ImGui::Button(AxisBaseLabels[AxisBaseIndex], ImVec2(28.0f, 0.0f)))
				{
					SetMappingAxisBase(DraftImportAxisMapping, RowIndex, AxisBaseIndex);
					RowDirection = GetMappingAxisDirection(DraftImportAxisMapping, RowIndex);
				}
				if (bSelectedBase)
				{
					ImGui::PopStyleColor(3);
				}
			}

			ImGui::SameLine();
			const bool bNegative = IsAxisNegative(RowDirection);
			if (bNegative)
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.75f, 0.3f, 0.3f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.85f, 0.4f, 0.4f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.65f, 0.25f, 0.25f, 1.0f));
			}
			else
			{
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.3f, 0.65f, 0.35f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.75f, 0.45f, 1.0f));
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.25f, 0.55f, 0.3f, 1.0f));
			}
			if (ImGui::Button(bNegative ? "-" : "+", ImVec2(28.0f, 0.0f)))
			{
				ToggleMappingAxisSign(DraftImportAxisMapping, RowIndex);
			}
			ImGui::PopStyleColor(3);
			ImGui::SameLine();
			ImGui::TextColored(ValueColor, "%s", GetAxisDirectionLabel(GetMappingAxisDirection(DraftImportAxisMapping, RowIndex)));
			ImGui::PopID();
		}

		ImGui::TextColored(LabelColor, "Import Scale:");
		ImGui::SameLine(100.0f);
		ImGui::SetNextItemWidth(120.0f);
		ImGui::DragFloat("##ImportScale", &DraftImportAxisMapping.ImportScale, 0.1f, 0.1f, 10.0f);

		const bool bHasValidDraftScale = DraftImportAxisMapping.ImportScale > ObjViewerImportScaleTolerance;
		const bool bHasPendingAxisChanges =
			DraftImportAxisMapping.EngineX != AppliedImportAxisMapping.EngineX ||
			DraftImportAxisMapping.EngineY != AppliedImportAxisMapping.EngineY ||
			DraftImportAxisMapping.EngineZ != AppliedImportAxisMapping.EngineZ ||
			!(std::fabs(DraftImportAxisMapping.ImportScale - AppliedImportAxisMapping.ImportScale) <= ObjViewerImportScaleTolerance);

		if (!bHasValidDraftScale)
		{
			ImGui::TextColored(ImVec4(0.95f, 0.45f, 0.35f, 1.0f), "Import scale must be greater than 0.");
		}
		else if (bHasPendingAxisChanges)
		{
			ImGui::TextColored(ImVec4(0.95f, 0.8f, 0.35f, 1.0f), "Pending changes. Click Apply to reload.");
		}
		else
		{
			ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.45f, 1.0f), "Axis mapping is up to date.");
		}

		if (!bHasPendingAxisChanges || !bHasValidDraftScale)
		{
			ImGui::BeginDisabled();
		}

		//apply 버튼.
		if (ImGui::Button("Apply"))
		{
			const FString AssetPath = MeshComp->GetStaticMeshAsset();
			const FTransform PreviousTransform = MeshActor->GetRootComponent()->GetRelativeTransform();
			const FVector PreviousDisplayedLocation = GetDisplayedLocation(MeshActor, ActiveViewportClient);
			const FString CookedPath = FAssetCooker::GetCookedPath(AssetPath);
			const bool bCookedExists = std::filesystem::exists(std::filesystem::path(FPaths::ToWide(CookedPath)));
			std::error_code Ec;
			if (bCookedExists)
			{
				std::filesystem::remove(std::filesystem::path(FPaths::ToWide(CookedPath)), Ec);
			}

			if (Ec)
			{
				UE_LOG("[ObjViewer] Reload failed: could not delete cooked asset '%s' (%s)", CookedPath.c_str(), Ec.message().c_str());
			}
			else if (FObjImporter::IsValidImportAxisMapping(DraftImportAxisMapping))
			{
				// 축 매핑 결과가 캐시와 cooked asset에 남아 있으므로 둘 다 무효화한 뒤 다시 로드합니다.
				FObjImporter::SetImportAxisMapping(DraftImportAxisMapping);
				FAssetManager::Get().InvalidateStaticMesh(AssetPath);
				FObjManager::ClearAssetCache(AssetPath);
				MeshActor->LoadStaticMesh(GRenderer->GetDevice(), AssetPath);

				if (MeshComp->GetStaticMesh() && MeshComp->GetMeshData())
				{
					// reload 과정에서 원점/바닥 기준이 달라질 수 있으므로 이전 표시 위치를 기준으로 다시 복원합니다.
					MeshActor->GetRootComponent()->SetRelativeTransform(PreviousTransform);
					SnapObjViewerActorBottomTo(MeshActor, ActiveViewportClient, PreviousDisplayedLocation.Z);
					Core->SetSelectedActor(MeshActor);
					ActiveViewportClient->FrameObjViewerCamera(MeshActor, true);
					EditorUI.SyncSelectedActorProperty();
					DraftAxisAssetPath = MeshAssetPath;
					DraftImportAxisMapping = FObjImporter::GetImportAxisMapping();
					LastAppliedImportAxisMapping = DraftImportAxisMapping;
				}
				else
				{
					UE_LOG("[ObjViewer] Reload failed: mesh load failed for '%s'", AssetPath.c_str());
				}
			}
			else
			{
				UE_LOG("[ObjViewer] Reload failed: invalid reload context");
			}
		}

		if (!bHasPendingAxisChanges || !bHasValidDraftScale)
		{
			ImGui::EndDisabled();
		}

		ImGui::Unindent();
	}

	ImGui::End();
}
