#include "Editor/UI/EditorMainPanel.h"
//#include "Editor/Viewport/EditorViewportClient.h"
#include "Editor/EditorEngine.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include "ImGui/imgui_internal.h"
// Merge icons into default tool font
#include "ImGui/IconsFontAwesome4.h"

#include "Render/Renderer/Renderer.h"
#include "World/Gizmo/GizmoManager.h"
#include "World/Primitives/Primitives.h"
#include "FileManager/SceneSaveManager.h"
#include "Core/Common.h"
#include "Engine/Core/InputSystem.h"
#include "Classes/ASpotlight.h"

#define SEPARATOR(); ImGui::Spacing(); ImGui::Spacing(); ImGui::Separator(); ImGui::Spacing(); ImGui::Spacing();

void FEditorMainPanel::Create(HWND InHWindow, FRenderer& InRenderer, FEditorEngine* InEditorEngine, FEditorViewportClient* InViewport)
{
	Viewport = InViewport;
	EditorEngine = InEditorEngine;
	SelectedPrimitiveType = static_cast<int>(EPrimitiveType::EPT_Cube);
	Renderer = &InRenderer;

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init((void*)InHWindow);
	ImGui_ImplDX11_Init(InRenderer.GetFD3DDevice().GetDevice(), InRenderer.GetFD3DDevice().GetDeviceContext());

	//폰트 아이콘
	ImGuiIO& io = ImGui::GetIO();
	io.Fonts->AddFontDefault();
	ImFontConfig config;
	config.MergeMode = true;
	config.GlyphOffset = ImVec2(0.0f, 2.0f);  // ← 이거 추가 (양수 = 아래로)
	config.GlyphMinAdvanceX = 13.0f; // Use if you want to make the icon monospaced
	static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
	io.Fonts->AddFontFromFileTTF("ImGui/fontawesome-webfont.ttf", 13.0f, &config, icon_ranges);
	io.Fonts->AddFontFromFileTTF("ImGui/NanumGothic.otf", 13.0f, &config, io.Fonts->GetGlyphRangesKorean());
}

void FEditorMainPanel::Release()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void FEditorMainPanel::Render(float DeltaTime, FViewOutput& ViewOutput)
{
	auto& SceneManager = EditorEngine->GetWorld().lock()->GetSceneManager();
	(void)DeltaTime;
	if (!EditorEngine)
	{
		return;
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();

	ImGui::NewFrame();

	ImGui::SetNextWindowCollapsed(false, ImGuiCond_Once);
	ImGui::SetNextWindowSize(ImVec2(500.0f, 480.0f), ImGuiCond_Once);

	ImGui::Begin("Jungle Control Panel");

	ConsoleInstance.Draw("Console", &bShowConsole);

	ImGui::Text("FPS : %.1f", EditorEngine->GetMainLoopFPS());

	ImGui::SameLine();

	ImGui::Text("Memory Allocated : %d", EngineStatics::GetTotalAllocationBytes());

	ImGui::SameLine();

	ImGui::Text("Times Allocated : %d", EngineStatics::GetTotalAllocationCount());

	ImGui::SeparatorText("Spawn");

	ImGui::Combo("Primitive", &SelectedPrimitiveType, PrimitiveTypes, IM_ARRAYSIZE(PrimitiveTypes));

	if (ImGui::Button("Spawn Primitive Actor"))
	{
		for (int i = 0; i < NumberOfSpawnedActors; i++) {
			switch ((EPrimitiveType)SelectedPrimitiveType)
			{
			case EPrimitiveType::EPT_Cube:
				EditorEngine->SpawnNewPrimitiveActor<UCubeComponent>(CurSpawnPoint);
				break;
			case EPrimitiveType::EPT_Sphere:
				EditorEngine->SpawnNewPrimitiveActor<USphereComponent>(CurSpawnPoint);
				break;
			case EPrimitiveType::EPT_Plane:
				EditorEngine->SpawnNewPrimitiveActor<UPlaneComponent>(CurSpawnPoint);
				break;
			case EPrimitiveType::EPT_SubUV:
				EditorEngine->SpawnNewPrimitiveActor<USubUVComponent>(CurSpawnPoint);
				break;
			case EPrimitiveType::EPT_Text:
				EditorEngine->SpawnNewPrimitiveActor<UTextComponent>(CurSpawnPoint);
				break;
			}
		}

		NumberOfSpawnedActors = 1;
	}

	ImGui::Combo("Utility", &SelectedUtilityActorType, UtilTypes, IM_ARRAYSIZE(UtilTypes));
	if (ImGui::Button("Spawn Utility Actor"))
	{
		for (int i = 0; i < NumberOfSpawnedActors; i++) {
			switch ((EUtilType)SelectedUtilityActorType)
			{
			case EUtilType::EUT_Spotlight:
				EditorEngine->SpawnNewUtilActor<ASpotlight>(CurSpawnPoint);
				break;
			}
		}

		NumberOfSpawnedActors = 1;
	}

	ImGui::InputInt("Number of Spawn", &NumberOfSpawnedActors, 1, 10);

	ImGui::SeparatorText("Scene");

	if (ImGui::Button("New Scene")) {
		Viewport->GetGizmoManager().SetVisibility(false);
		ViewOutput.Object = nullptr;
		EditorEngine->NewScene();
		NewSceneNotificationTimer = common::constants::imgui::NotificationTimer;
	}
	if (NewSceneNotificationTimer > 0.0f) {
		NewSceneNotificationTimer -= DeltaTime;
		ImGui::SameLine();
		ImGui::Text("New scene created");
	}

	if (ImGui::Button("Save Scene")) {
		FSceneSaveManager::SaveSceneAsJSON(EditorEngine->GetWorld().lock()->GetActiveScene().lock().get());
		SceneSaveNotificationTimer = common::constants::imgui::NotificationTimer;
	}
	if (SceneSaveNotificationTimer > 0.0f) {
		SceneSaveNotificationTimer -= DeltaTime;
		ImGui::Text("Scene saved");
	}

	if (ImGui::Button("Load Scene")) {
		UScene* Scene = FSceneSaveManager::LoadSceneFromJSON(EditorEngine->GetWorld().lock().get());
		if (Scene) {
			Viewport->GetGizmoManager().SetVisibility(false);
			ViewOutput.Object = nullptr;
			//EditorEngine->GetWorld()->GetSceneManager().RemoveActiveScene();
			SceneManager.AddScene(Scene);
			SceneManager.SetActiveScene(Scene);
			Viewport->ResetViewport();
			EditorEngine->ResetViewportScene();
			Sleep(50);	// Safeguard
			SceneLoadNotificationTimer = common::constants::imgui::NotificationTimer;
		}
	}

	if (SceneLoadNotificationTimer > 0.0f) {
		SceneLoadNotificationTimer -= DeltaTime;
		ImGui::Text("Scene loaded");
	}

	SEPARATOR();

	FCameraState& CameraState = Viewport->GetCamera().lock()->GetCameraState();
	ImGui::Checkbox("Orthographic", &(CameraState.bIsOrthogonal));

	float CameraFOV_Deg = CameraState.FOV * (RAD_TO_DEG);
	float OrthoWidth = CameraState.OrthoWidth;

	if (!CameraState.bIsOrthogonal) {
		if (ImGui::DragFloat("Camera FOV",
			&CameraFOV_Deg,
			0.5f,          // speed in degrees
			1.0f,          // min deg
			90.0f))        // max deg
		{
			CameraState.FOV = CameraFOV_Deg * (DEG_TO_RAD);
		}
	}
	else {
		float OrthoWidth = CameraState.OrthoWidth;
		if (ImGui::DragFloat("Ortho Width", &OrthoWidth, 0.1f, 0.1f, 1000.0f))
		{
			CameraState.OrthoWidth = Clamp(OrthoWidth, 0.1f, 1000.0f);
		}
	}
	UCamera* Camera = Viewport->GetCamera().lock().get();
	FVector CamPos = Camera->GetWorldLocation();
	float CameraLocation[3] = { CamPos.X, CamPos.Y, CamPos.Z };
	if (ImGui::DragFloat3("Camera Location", CameraLocation, 0.1f))
	{
		Camera->SetWorldLocation(FVector(CameraLocation[0], CameraLocation[1], CameraLocation[2]));
	}
	FVector CamRot = Camera->GetRelativeRotation();
	float CameraRotation[3] = { CamRot.X, CamRot.Y, CamRot.Z };
	if (ImGui::DragFloat3("Camera Rotation", CameraRotation, 0.1f))
	{
		Camera->SetRelativeRotation(FVector(Clamp(CameraRotation[0], CamRot.X, CamRot.X), CameraRotation[1], CameraRotation[2]));
	}

	int GridSize = static_cast<int>(FUISettingInitializer::GetGridSize());
	if (ImGui::DragInt("Grid Size", &GridSize, 1, 1, 100))
	{
		FUISettingInitializer::SetGridSize(static_cast<uint32>(GridSize));
	}

	float CamMoveSpeed = FUISettingInitializer::GetViewCamMoveSpeed();
	if (ImGui::SliderFloat("Camera Movement Sensitivity", &CamMoveSpeed, 5.f, 30.f)) {
		FUISettingInitializer::SetViewCamMoveSpeed(CamMoveSpeed);
	}

	float CamRotSpeed = FUISettingInitializer::GetViewCamRotSpeed();
	if (ImGui::SliderFloat("Camera Rotation Sensitivity", &CamRotSpeed, 0.05f, 0.6f)) {
		FUISettingInitializer::SetViewCamRotSpeed(CamRotSpeed);
	}

	SEPARATOR();
	ImGui::SeparatorText("Gizmo");

	// Space Select Button(World or Local)
	static int SelectedSpace = 0;
	if (ImGui::RadioButton("World", &SelectedSpace, 0))
	{
		Viewport->GetGizmoManager().SetWorldSpace(true);
		std::cout << "Switched to World Space\n";
	}

	ImGui::SameLine();

	if (ImGui::RadioButton("Local", &SelectedSpace, 1))
	{
		Viewport->GetGizmoManager().SetWorldSpace(false);
		std::cout << "Switched to Local Space\n";
	}


	ImGui::SeparatorText("Tools");

	if (ImGui::Button("Translate")) Viewport->GetGizmoManager().SetTranslateMode();
	ImGui::SameLine();
	if (ImGui::Button("Rotate")) Viewport->GetGizmoManager().SetRotateMode();
	ImGui::SameLine();
	if (ImGui::Button("Scale")) Viewport->GetGizmoManager().SetScaleMode();

	ImGui::SeparatorText("View Mode");

	static int ViewMode = 0;
	if (ImGui::RadioButton(ICON_FA_ADJUST" Lit", &ViewMode, 0)) {
		Renderer->viewMode = EViewModeIndex::VMI_Unlit;
	}

	if (ImGui::RadioButton(ICON_FA_CIRCLE" Unlit", &ViewMode, 1)) {
		Renderer->viewMode = EViewModeIndex::VMI_Unlit;
	}

	if (ImGui::RadioButton(ICON_FA_CODEPEN" Wireframe", &ViewMode, 2)) {
		Renderer->viewMode = EViewModeIndex::VMI_Wireframe;
	}

	ImGui::SeparatorText("Common Show Flag");


	ImGui::CheckboxFlags(ICON_FA_CUBES" Primitives", (int*)&Renderer->showFlag, (uint64)EEngineShowFlags::SF_Primitives);
	ImGui::CheckboxFlags(ICON_FA_FONT " BillboardText", (int*)&Renderer->showFlag, (uint64)EEngineShowFlags::SF_BillboardText);



	ImGui::End();

	RenderPickedObjectWindow(ViewOutput.Object);
	RenderObjectManager(ViewOutput);

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void FEditorMainPanel::RenderObjectManager(FViewOutput& ViewOutput) {
	ImGui::Begin("Scene Manager");
	auto& SceneManager = EditorEngine->GetWorld().lock()->GetSceneManager();

	// Loaded scenes dropdown
	if (ImGui::CollapsingHeader("Scenes")) {
		const TArray<UScene*>& Scenes = SceneManager.GetScenes();
		for (int i = 0; i < Scenes.size(); i++) {
			UScene* Scene = Scenes[i];
			if (!Scene) continue;

			std::string Label = "Scene_" + std::to_string(Scene->UUID);

			bool bSelected = (SelectedSceneIndex == i);
			if (ImGui::Selectable(Label.c_str(), bSelected)) {
				SelectedSceneIndex = i;
				SceneManager.SetActiveScene(Scene);
				Viewport->ResetViewport();
				EditorEngine->ResetViewportScene();
			}
		}
	}

	// Load actors present in the current scene
	TArray<AActor*> PrimitiveActors;
	TArray<AActor*> NonPrimitiveActors;
	const TArray<AActor*>& Actors = SceneManager.GetActiveScene().lock()->GetActors();
	for (AActor* Actor : Actors) {
		if (!Actor || Actor->bPendingKill) continue;

		// We define Primitive actor = has a UPrimitiveComponent as its root
		if (Actor->GetRootComponent() && Actor->GetRootComponent()->IsA<UPrimitiveComponent>()) {
			PrimitiveActors.push_back(Actor);
		}
		else {
			NonPrimitiveActors.push_back(Actor);
		}
	}

	// Primitive Actors dropdown
	if (ImGui::CollapsingHeader("Scene Actors")) {
		for (int i = 0; i < PrimitiveActors.size(); i++) {
			AActor* Actor = PrimitiveActors[i];
			FString Label = "Actor_" + std::to_string(Actor->UUID);

			//bool bSelected = (SelectedActorIndex == Actor->UUID);
			//if (ImGui::Selectable(Label.c_str(), bSelected)) {
			//	SelectedActorIndex = Actor->UUID;
			//	auto* TargetComponent = Actor->GetRootComponent();

			//	// Move gizmo to target component
			//	Viewport->GetGizmoManager().SetTarget(Actor->GetRootComponent());

			//	// Let the viewoutput know about selected object
			//	ViewOutput.Object = TargetComponent;
			//	ViewOutput.ObjectPicked = TargetComponent->GetTypeInfo()->name;
			//}
			ImGui::Indent();
			if (ImGui::CollapsingHeader(Label.c_str())) {
				for (auto* Comps : Actor->GetComponents()) {
					bool bSelected = (SelectedComponentIndex == Comps->UUID);
					FString CompLabel = Comps->GetTypeInfo()->name + std::to_string(Comps->UUID);
					if (ImGui::Selectable(CompLabel.c_str(), bSelected)) {
						SelectedComponentIndex = Comps->UUID;
						Viewport->GetGizmoManager().SetTarget(Comps);
						ViewOutput.Object = Comps;
						ViewOutput.ObjectPicked = Comps->GetTypeInfo()->name;
					}

					if (bSelected)
					{
						// Text 컴포넌트면 입력창 표시
						if (Comps->IsA<UTextComponent>()) {
							UTextComponent* TextComp = static_cast<UTextComponent*>(Comps);

							// UUID별로 버퍼 관리
							static std::unordered_map<uint32, std::array<char, 256>> TextBuffers;
							auto& Buffer = TextBuffers[Comps->UUID];

							// 최초 1회 현재 텍스트로 초기화
							if (bSelected) {
								std::wstring Current = TextComp->GetText();
								WideCharToMultiByte(CP_UTF8, 0, Current.c_str(), -1, Buffer.data(), 256, nullptr, nullptr);
							}

							FString InputLabel = "##TextInput" + std::to_string(Comps->UUID);
							ImGui::Indent();
							if (ImGui::InputText(InputLabel.c_str(), Buffer.data(), Buffer.size())) {
								wchar_t Wide[256];
								MultiByteToWideChar(CP_UTF8, 0, Buffer.data(), -1, Wide, 256);
								std::wstring WideStr(Wide);
								TextComp->SetText(WideStr);
							}
							ImGui::Unindent();
						}
					}

				}
			}
			ImGui::Unindent();
		}
	}

	// Non-Primitive Actors Dropdown
	if (ImGui::CollapsingHeader("Non-Scene Actors")) {
		for (int i = 0; i < NonPrimitiveActors.size(); i++) {
			AActor* Actor = NonPrimitiveActors[i];
			std::string Label = "Actor_" + std::to_string(Actor->UUID);

			bool bSelected = (SelectedActorIndex == Actor->UUID);
			if (ImGui::Selectable(Label.c_str(), bSelected)) {
				SelectedActorIndex = Actor->UUID;
				Viewport->GetGizmoManager().Deactivate();
			}
		}
	}

	ImGui::End();
}

void FEditorMainPanel::RenderPickedObjectWindow(UObject*& ObjectPicked) {
	ImGui::Begin("Picked Object");
	if(!ObjectPicked) {
		ImGui::End();
		return;
	}
	if (ObjectPicked) {
		ImGui::Text("Class: %s", ObjectPicked->GetTypeInfo()->name);
		ImGui::Text("Object Size: %d", sizeof(*ObjectPicked));
		//ImGui::Text("Object Name: %s", ObjectPicked->Name.GetFString().c_str());
	}
	if (ObjectPicked->IsA<USceneComponent>()) {
		ImGui::Text("Transform");
		ImGui::Separator();

		USceneComponent* SceneComp = ObjectPicked->Cast<USceneComponent>();
		FVector Pos = SceneComp->GetWorldLocation();
		float PosArray[3] = { Pos.X, Pos.Y, Pos.Z };

		FVector Rot = SceneComp->GetRelativeRotation();
		float RotArray[3] = { Rot.X, Rot.Y, Rot.Z };

		FVector Scale = SceneComp->GetRelativeScale();
		float ScaleArray[3] = { Scale.X, Scale.Y, Scale.Z };


		FGizmoManager& Gizmo = Viewport->GetGizmoManager();
		if (ImGui::DragFloat3("Location", PosArray, 0.1f))
		{
			Gizmo.SetTargetLocation(FVector(PosArray[0], PosArray[1], PosArray[2]));
		}
		if (ImGui::DragFloat3("Rotation", RotArray, 0.1f))
		{
			Gizmo.SetTargetRotation(FVector(RotArray[0], RotArray[1], RotArray[2]));
		}
		if (ImGui::DragFloat3("Scale", ScaleArray, 0.1f))
		{
			Gizmo.SetTargetScale(FVector(ScaleArray[0], ScaleArray[1], ScaleArray[2]));
		}

		SEPARATOR();

		if (ImGui::Button("Remove Object") && ObjectPicked) {
			ObjectPicked->bPendingKill = true;
			if (ObjectPicked->IsA<USceneComponent>()) {
				USceneComponent* SceneComp = ObjectPicked->Cast<USceneComponent>();
				if (SceneComp->GetOwningActor()) {
					// TODO:: Do this recursively
					UObjectManager::Get().DestroyObject(SceneComp->GetOwningActor());
				}
			}
			Viewport->GetGizmoManager().SetVisibility(false);
			Viewport->GetGizmoManager().Deactivate();
			ObjectPicked = nullptr;
		}

		if (SceneComp->GetOwningActor()) { RenderPickedActorWindow(SceneComp->GetOwningActor()); }
	}

	ImGui::End();
}

void FEditorMainPanel::RenderPickedActorWindow(AActor* Actor) {
	if (Actor->IsA<ASpotlight>()) {
		SEPARATOR();
		ASpotlight* SpotlightActor = ASpotlight::Cast(Actor);
		USpotlightComponent* Spotlight = SpotlightActor->GetSpotlightComp();
		float SpotlightRot[2] = { Spotlight->GetYaw(), Spotlight->GetPitch()};
		if (ImGui::DragFloat2("Spotlight Rotation", SpotlightRot, 0.1f)) {
			SpotlightRot[0] = Clamp(SpotlightRot[0], -80.0f, 80.0f); SpotlightRot[1] = Clamp(SpotlightRot[1], -80.0f, 80.0f);
			Spotlight->SetYaw(SpotlightRot[0]); Spotlight->SetPitch(SpotlightRot[1]);
		}

		int NumVertices = Spotlight->GetNumVertices();
		if (ImGui::DragInt("No. Vertices", &NumVertices, 1)) { 
			NumVertices = Clamp(NumVertices, 0, 500);
			if (NumVertices >= 0) {
				Spotlight->SetNumVertex(NumVertices);
			}
		}

		float SpotlightHeight = Spotlight->GetConeHeight();
		if (ImGui::DragFloat("Spotlight Height", &SpotlightHeight, 0.1f)) {SpotlightHeight =  Clamp(SpotlightHeight, 0.1f, 500.0f); Spotlight->SetConeHeight(SpotlightHeight); }

		float SpotlightRadius = Spotlight->GetRadius();
		if (ImGui::DragFloat("Spotlight Radius", &SpotlightRadius, 0.1f)) { SpotlightRadius = Clamp(SpotlightRadius, 0.1f, 100.0f); Spotlight->SetConeRadius(SpotlightRadius); }

		// Color
		FVector4 RayColor = Spotlight->GetColor();
		float Color[4] = { RayColor.X, RayColor.Y, RayColor.Z, RayColor.W };
		if (ImGui::ColorEdit4("Ray Color", Color)) {
			FVector4 NewColor(Color[0], Color[1], Color[2], Color[3]);
			Spotlight->SetColor(NewColor);
		}
	}
}

void FEditorMainPanel::Update()
{
	ImGuiIO& io = ImGui::GetIO();

	InputSystem::GuiInputState.bUsingMouse = io.WantCaptureMouse; 
	InputSystem::GuiInputState.bUsingKeyboard = io.WantCaptureKeyboard;
}
