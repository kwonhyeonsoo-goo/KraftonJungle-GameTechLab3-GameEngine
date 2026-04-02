#include "PropertyWindow.h"

#include "Actor/Actor.h"
#include "Actor/StaticMeshActor.h"
#include "Component/StaticMeshComponent.h"
#include "Component/SubUVComponent.h"
#include "Component/TextComponent.h"
#include "Component/UUIDBillboardComponent.h"
#include "Core/Core.h"
#include "Core/Paths.h"
#include "Object/StaticMesh.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/Renderer.h"
#include "Asset/AssetRegistry.h"
#include "Asset/AssetManager.h"
#include "Actor/Actor.h"
#include "Debug/EngineLog.h"
#include <algorithm>
#include <filesystem>

void FPropertyWindow::SetTarget(const FVector& Location, const FVector& Rotation, const FVector& Scale, const char* ActorName)
{
	EditLocation = Location;
	EditRotation = Rotation;
	EditScale = Scale;
	bModified = false;

	if (ActorName)
	{
		snprintf(ActorNameBuf, sizeof(ActorNameBuf), "%s", ActorName);
	}
	else
	{
		snprintf(ActorNameBuf, sizeof(ActorNameBuf), "%s", "None");
	}
}

void FPropertyWindow::DrawTransformSection()
{
	const float ResetBtnWidth = 14.0f;
	const float Spacing = ImGui::GetStyle().ItemInnerSpacing.x;
	const float DragUIWidth = 200.0f;

	auto DrawTransformRow = [&](const char* Label, const char* BtnID, FVector& Vec, const FVector& ResetVal, const ImVec4& BaseColor, float Step, float Min, float Max, const char* Format)
	{
		float Value[3] = { Vec.X, Vec.Y, Vec.Z };

		ImGui::PushStyleColor(ImGuiCol_Button, BaseColor);
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(BaseColor.x + 0.2f, BaseColor.y + 0.1f, BaseColor.z + 0.1f, 1.0f));
		ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(BaseColor.x + 0.4f, BaseColor.y + 0.2f, BaseColor.z + 0.2f, 1.0f));

		if (ImGui::Button(BtnID, ImVec2(ResetBtnWidth, 0)))
		{
			Vec = ResetVal;
			bModified = true;
		}
		if (ImGui::IsItemHovered())
		{
			ImGui::SetTooltip("Reset %s", Label);
		}
		ImGui::PopStyleColor(3);

		ImGui::SameLine(0, Spacing);
		ImGui::PushItemWidth(DragUIWidth);
		if (ImGui::DragFloat3(Label, Value, Step, Min, Max, Format))
		{
			Vec = { Value[0], Value[1], Value[2] };
			bModified = true;
		}
		ImGui::PopItemWidth();
	};

	DrawTransformRow("Location", "##RL", EditLocation, { 0.f, 0.f, 0.f }, ImVec4(0.5f, 0.1f, 0.1f, 1.0f), 0.1f, 0.0f, 0.0f, "%.2f");
	DrawTransformRow("Rotation", "##RR", EditRotation, { 0.f, 0.f, 0.f }, ImVec4(0.1f, 0.4f, 0.1f, 1.0f), 0.5f, -360.0f, 360.0f, "%.1f");
	DrawTransformRow("Scale", "##RS", EditScale, { 1.f, 1.f, 1.f }, ImVec4(0.1f, 0.2f, 0.5f, 1.0f), 0.01f, 0.001f, 100.0f, "%.3f");

	if (bModified && OnChanged)
	{
		OnChanged(EditLocation, EditRotation, EditScale);
	}
}

void FPropertyWindow::DrawBillboardSection(AActor* SelectedActor)
{
	if (!ImGui::CollapsingHeader("Billboard", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}

	ImGui::Indent(8.0f);
	for (UActorComponent* Component : SelectedActor->GetComponents())
	{
		if (!Component)
		{
			continue;
		}

		if (Component->IsA(USubUVComponent::StaticClass()))
		{
			auto* SubUVComp = static_cast<USubUVComponent*>(Component);
			bool bBillboard = SubUVComp->IsBillboard();
			if (ImGui::Checkbox("SubUV Billboard", &bBillboard))
			{
				SubUVComp->SetBillboard(bBillboard);
			}
		}
		else if (Component->IsA(UTextComponent::StaticClass()) && !Component->IsA(UUUIDBillboardComponent::StaticClass()))
		{
			auto* TextComp = static_cast<UTextComponent*>(Component);
			bool bBillboard = TextComp->IsBillboard();
			if (ImGui::Checkbox("Text Billboard", &bBillboard))
			{
				TextComp->SetBillboard(bBillboard);
			}
		}
	}
	ImGui::Unindent(8.0f);
}

auto ProcessDragDrop = [](const std::vector<std::string>& TargetExts, auto OnDropValid)
{
	if (!ImGui::BeginDragDropTarget())
	{
		return;
	}
	//Accept하기 전에 현재 마우스에 매달려 있는 Payload 데이터를 확인
	if (const ImGuiPayload* Payload = ImGui::GetDragDropPayload())
	{
		if (Payload->IsDataType("CONTENT_BROWSER_ITEM"))
		{
			std::string AbsolutePath = static_cast<const char*>(Payload->Data);
			std::string LowerPath = AbsolutePath;
			std::transform(LowerPath.begin(), LowerPath.end(), LowerPath.begin(), ::tolower);

			bool bValid = false;
			for (const auto& Ext : TargetExts)
			{
				if (LowerPath.find(Ext) != std::string::npos)
				{
					bValid = true;
					break;
				}
			}

			// 원하는 확장자일 때만 진짜로 Accept
			if (bValid)
			{
				if (ImGui::AcceptDragDropPayload("CONTENT_BROWSER_ITEM"))
				{
					std::string RelativePath = FPaths::ToRelativePath(AbsolutePath);
					OnDropValid(AbsolutePath, RelativePath);
				}
			}
		}
	}

	ImGui::EndDragDropTarget();
};

void FPropertyWindow::DrawMaterialSlots(FCore* Core, UStaticMeshComponent* SMComp, AActor* SelectedActor)
{
	//AssetRegistry에서 가벼운 메타데이터 목록만 가져옴
	TArray<FAssetData> MaterialAssets = FAssetRegistry::Get().GetAssetsByClass("Material");
	TArray<FAssetData> TextureAssets = FAssetRegistry::Get().GetAssetsByClass("Texture");

	std::vector<const char*> MatItems = { "Default" };
	for (const auto& Asset : MaterialAssets)
	{
		MatItems.push_back(Asset.AssetName.c_str());
	}

	std::vector<const char*> TexItems = { "None" };
	for (const auto& Asset : TextureAssets)
	{
		TexItems.push_back(Asset.AssetName.c_str());
	}

	const uint32 NumSlots = SMComp->GetNumMaterials();
	static TWeakObjectPtr<AActor> LastSelectedActor;
	static FString LastMeshAsset = "";
	FString CurrentMeshAsset = SMComp->GetStaticMeshAsset();
	static std::vector<int> SlotMatIndices;
	static std::vector<int> SlotTexIndices;

	if (LastSelectedActor.Get() != SelectedActor || LastMeshAsset != CurrentMeshAsset)
	{
		SlotMatIndices.clear();
		SlotTexIndices.clear();
		LastSelectedActor = SelectedActor;
		LastMeshAsset = CurrentMeshAsset;
	}

	if (SlotMatIndices.size() < NumSlots) SlotMatIndices.resize(NumSlots, 0);
	if (SlotTexIndices.size() < NumSlots) SlotTexIndices.resize(NumSlots, 0);
	for (uint32 SlotIdx = 0; SlotIdx < NumSlots; ++SlotIdx)
	{
		//  매 프레임: 컴포넌트에 현재 적용된 매테리얼(.mtl 반영)과 UI 콤보박스를 강제 동기화!
		FMaterial* CurrentMat = SMComp->GetMaterial(SlotIdx);
		if (CurrentMat)
		{
			FString MatName = CurrentMat->GetOriginName();
			SlotMatIndices[SlotIdx] = 0; // Default로 일단 세팅
			for (int j = 0; j < (int)MaterialAssets.size(); ++j)
			{
				FString AssetName = std::filesystem::path(MaterialAssets[j].AssetName).stem().string();
				if (AssetName == MatName)
				{
					SlotMatIndices[SlotIdx] = j + 1; // 0은 Default이므로 +1
					break;
				}
			}
		}
		SlotTexIndices[SlotIdx] = 0;
		auto DynMatIt = SMComp->DynamicMaterialOwners.find(SlotIdx);
		if (DynMatIt != SMComp->DynamicMaterialOwners.end() && DynMatIt->second)
		{
			if (auto MatTex = DynMatIt->second->GetMaterialTexture())
			{
				FString CurrentTexPath = MatTex->AssetPath; // 기억해둔 텍스처 경로 꺼내기
				if (!CurrentTexPath.empty())
				{
					namespace fs = std::filesystem;
					for (int j = 0; j < (int)TextureAssets.size(); ++j)
					{
						// 역슬래시, 대소문자 등 꼬이지 않게 lexically_normal()로 깔끔하게 비교
						if (fs::path(FPaths::ToWide(TextureAssets[j].AssetPath)).lexically_normal() ==
							fs::path(FPaths::ToWide(CurrentTexPath)).lexically_normal())
						{
							SlotTexIndices[SlotIdx] = j + 1;
							break;
						}
					}
				}
			}
		}
		ImGui::PushID(SlotIdx);
		if (ImGui::TreeNodeEx(("Material Slot " + std::to_string(SlotIdx)).c_str(), ImGuiTreeNodeFlags_DefaultOpen))
		{
			// ==========================================
			// 1. Material 파트
			// ==========================================
			if (ImGui::Combo("Material", &SlotMatIndices[SlotIdx], MatItems.data(), static_cast<int>(MatItems.size())) && Core && SlotMatIndices[SlotIdx] > 0)
			{
				FString MatFileName = MaterialAssets[SlotMatIndices[SlotIdx] - 1].AssetName;
				FString MatName = std::filesystem::path(MatFileName).stem().string();
				if (auto Mat = FMaterialManager::Get().FindByName(MatName))
				{
					std::shared_ptr<FMaterialTexture> OldTex = GDefaultWhiteTexture;
					auto OldIt = SMComp->DynamicMaterialOwners.find(SlotIdx);
					if (OldIt != SMComp->DynamicMaterialOwners.end() && OldIt->second && OldIt->second->GetMaterialTexture())
					{
						OldTex = OldIt->second->GetMaterialTexture();
					}

					//shared_ptr로 안전하게 복사/전달
					std::shared_ptr<FDynamicMaterial> DynMat = Mat->CreateDynamicMaterial();
					if (DynMat)
					{
						DynMat->SetMaterialTexture(OldTex);
						SMComp->DynamicMaterialOwners[SlotIdx] = DynMat; // 소유권 저장
						SMComp->SetMaterial(SlotIdx, DynMat.get());      // 포인터 연결
					}
				}
			}

			// Material 콤보박스용 드래그 앤 드롭
			ProcessDragDrop({ ".mat", ".json" }, [&](const std::string& AbsPath, const std::string& RelPath) {
				std::string MatName = std::filesystem::path(RelPath).stem().string();
				if (auto Mat = FMaterialManager::Get().FindByName(MatName.c_str()))
				{
					std::shared_ptr<FMaterialTexture> OldTex = GDefaultWhiteTexture;
					auto OldIt = SMComp->DynamicMaterialOwners.find(SlotIdx);
					if (OldIt != SMComp->DynamicMaterialOwners.end() && OldIt->second && OldIt->second->GetMaterialTexture())
					{
						OldTex = OldIt->second->GetMaterialTexture();
					}

					// 드래그 앤 드롭에도 텍스처 보존 로직 동일하게 적용
					std::shared_ptr<FDynamicMaterial> DynMat = Mat->CreateDynamicMaterial();
					if (DynMat)
					{
						DynMat->SetMaterialTexture(OldTex);
						SMComp->DynamicMaterialOwners[SlotIdx] = DynMat;
						SMComp->SetMaterial(SlotIdx, DynMat.get());
					}
				}
			});

			// ==========================================
			// 2. Texture 파트
			// ==========================================
			if (ImGui::Combo("Texture", &SlotTexIndices[SlotIdx], TexItems.data(), static_cast<int>(TexItems.size())) && Core && SlotTexIndices[SlotIdx] > 0 && GRenderer)
			{
				FString TexPath = TextureAssets[SlotTexIndices[SlotIdx] - 1].AssetPath;
				SMComp->LoadTextureToSlot(GRenderer->GetDevice(), TexPath, SlotIdx);
			}

			// 텍스처 콤보박스용 드래그 앤 드롭
			ProcessDragDrop({ ".png", ".jpg", ".jpeg" }, [&](const std::string& AbsPath, const std::string& RelPath) {
				if (GRenderer)
				{
					SMComp->LoadTextureToSlot(GRenderer->GetDevice(), RelPath, SlotIdx);
				}

				std::string NormalizedRel = RelPath;
				std::replace(NormalizedRel.begin(), NormalizedRel.end(), '\\', '/');

				for (int i = 0; i < (int)TextureAssets.size(); ++i) {
					std::string NormalizedTex = TextureAssets[i].AssetPath;
					std::replace(NormalizedTex.begin(), NormalizedTex.end(), '\\', '/');

					if (NormalizedTex == NormalizedRel) {
						SlotTexIndices[SlotIdx] = i + 1;
						break;
					}
				}
			});

			DrawUVScrollControls(SMComp, SlotIdx);

			ImGui::TreePop();
		}
		ImGui::PopID();
	}
}

/**
 * UV scroll을 키고 끌 수 있는 기능을 패널에 추가합니다.
 * 
 * \param SMComp
 * \param SlotIdx
 */
void FPropertyWindow::DrawUVScrollControls(UStaticMeshComponent* SMComp, uint32 SlotIdx)
{
	if (!SMComp)
	{
		return;
	}

	ImGui::SeparatorText("UV Scroll");
	if (!SMComp->IsUVScrollSupported(SlotIdx))
	{
		ImGui::TextDisabled("M_StaticMesh dynamic material required");
		return;
	}

	bool bUVScrollEnabled = SMComp->IsUVScrollEnabled(SlotIdx);
	if (ImGui::Checkbox("Enable UV Scroll", &bUVScrollEnabled))
	{
		SMComp->SetUVScrollEnabled(SlotIdx, bUVScrollEnabled);
	}

	FVector2 UVScrollSpeed = SMComp->GetUVScrollSpeed(SlotIdx);
	float SpeedValues[2] = { UVScrollSpeed.X, UVScrollSpeed.Y };
	if (ImGui::DragFloat2("Scroll Speed", SpeedValues, 0.01f, -10.0f, 10.0f, "%.2f"))
	{
		SMComp->SetUVScrollSpeed(SlotIdx, FVector2(SpeedValues[0], SpeedValues[1]));
	}
}

void FPropertyWindow::DrawStaticMeshSection(FCore* Core, AStaticMeshActor* SMActor)
{
	if (!ImGui::CollapsingHeader("Static Mesh", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}

	ImGui::Indent(8.0f);
	UStaticMeshComponent* SMComp = SMActor->GetStaticMeshComponent();
	if (!SMComp)
	{
		ImGui::Unindent(8.0f);
		return;
	}

	//  AssetRegistry에서 StaticMesh 목록을 가져옴 
	TArray<FAssetData> MeshAssets = FAssetRegistry::Get().GetAssetsByClass("StaticMesh");

	std::vector<const char*> Items = { "None" };
	for (const auto& Asset : MeshAssets)
	{
		Items.push_back(Asset.AssetName.c_str());
	}

	int SelectedMeshIndex = 0;
	FString CurrentAsset = SMComp->GetStaticMeshAsset();
	std::replace(CurrentAsset.begin(), CurrentAsset.end(), '\\', '/');

	namespace fs = std::filesystem;
	for (int i = 0; i < (int)MeshAssets.size(); ++i) {
		FString AssetPath = MeshAssets[i].AssetPath;
		if (fs::path(FPaths::ToWide(AssetPath)).lexically_normal() == fs::path(FPaths::ToWide(CurrentAsset)).lexically_normal()) {
			SelectedMeshIndex = i + 1;
			break;
		}
	}

	// 지연 로딩: 사용자가 콤보박스를 클릭했을 때만 AssetManager를 통해 로드!
	if (ImGui::Combo("Mesh Asset", &SelectedMeshIndex, Items.data(), static_cast<int>(Items.size())) && Core && SelectedMeshIndex > 0 && GRenderer)
	{
		FString SelectedPath = MeshAssets[SelectedMeshIndex - 1].AssetPath;
		UStaticMesh* LoadedMesh = FAssetManager::Get().LoadStaticMesh(GRenderer->GetDevice(), SelectedPath);
		if (LoadedMesh)
		{
			SMComp->SetStaticMeshData(GRenderer->GetDevice(), LoadedMesh);
		}
	}

	//  사용자가 드래그 앤 드롭 했을 때만 AssetManager를 통해 로드
	ProcessDragDrop({ ".dasset" }, [&](const std::string& AbsPath, const std::string& RelPath) {
		if (GRenderer)
		{
			UStaticMesh* LoadedMesh = FAssetManager::Get().LoadStaticMesh(GRenderer->GetDevice(), RelPath);
			if (LoadedMesh)
			{
				SMComp->SetStaticMeshData(GRenderer->GetDevice(), LoadedMesh);
			}
		}
	});

	if (!CurrentAsset.empty())
	{
		DrawMaterialSlots(Core, SMComp, SMActor);
	}

	ImGui::Unindent(8.0f);
}

void FPropertyWindow::Render(FCore* Core)
{
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 8));
	const bool bOpen = ImGui::Begin("Properties");
	ImGui::PopStyleVar();

	if (!bOpen)
	{
		ImGui::End();
		return;
	}

	bModified = false;
	ImGui::TextDisabled("Selected:");
	ImGui::SameLine();
	ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.4f, 1.0f), "%s", ActorNameBuf);
	ImGui::Separator();

	if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		ImGui::Indent(8.0f);
		DrawTransformSection();
		ImGui::Unindent(8.0f);
	}

	if (Core && Core->GetSelectedActor())
	{
		AActor* SelectedActor = Core->GetSelectedActor();
		DrawBillboardSection(SelectedActor);

		if (SelectedActor->IsA(AStaticMeshActor::StaticClass()))
		{
			DrawStaticMeshSection(Core, static_cast<AStaticMeshActor*>(SelectedActor));
		}
	}

	ImGui::End();
}