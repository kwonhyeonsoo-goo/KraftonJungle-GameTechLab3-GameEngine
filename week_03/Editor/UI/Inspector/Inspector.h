//#pragma once  
////#include <functional>  
////#include <iostream>  
////#include <unordered_map>  
//#include "Classes/AActor.h"  
//#include "World/SceneComponent.h"
//
//#include "ImGui/imgui.h"
//#include "ImGui/imgui_impl_dx11.h"
//#include "ImGui/imgui_impl_win32.h"
//
//// Registers an inspector to each actor type  
////class FInspectorRegistrar {  
////public:  
////static FInspectorRegistrar& Get() {  
////	static FInspectorRegistrar Registrar;  
////	return Registrar;  
////}  
////
////void Register(const char* ActorName, std::function<void()> InfoPanel) {    
////	Registry[ActorName] = InfoPanel;    
////}
////
////std::function<void()> CreateInfoPanel(const std::string& ActorName) {
////	auto InfoPanel = Registry.find(ActorName);
////	if (InfoPanel != Registry.end()) { return InfoPanel->second; }
////	return nullptr;
////}
////
////private:  
////	TMap<std::string, std::function<void()>> Registry;
////};
//
//static void InspectSceneComponent(UObject* ObjectPicked) {
//	ImGui::Begin("Picked Object");
//	if (!ObjectPicked) {
//		ImGui::End();
//		return;
//	}
//	if (ObjectPicked) {
//		ImGui::Text("Class: %s", ObjectPicked->GetTypeInfo()->name);
//		ImGui::Text("Object Size: %d", sizeof(*ObjectPicked));
//	}
//	if (ObjectPicked->IsA<USceneComponent>()) {
//		ImGui::Text("Transform");
//		ImGui::Separator();
//
//		USceneComponent* SceneComp = ObjectPicked->Cast<USceneComponent>();
//		FVector Pos = SceneComp->GetWorldLocation();
//		float PosArray[3] = { Pos.X, Pos.Y, Pos.Z };
//
//		FVector Rot = SceneComp->GetRelativeRotation();
//		float RotArray[3] = { Rot.X, Rot.Y, Rot.Z };
//
//		FVector Scale = SceneComp->GetRelativeScale();
//		float ScaleArray[3] = { Scale.X, Scale.Y, Scale.Z };
//
//
//		FGizmoManager& Gizmo = Viewport->GetGizmoManager();
//		if (ImGui::DragFloat3("Location", PosArray, 0.1f))
//		{
//			Gizmo.SetTargetLocation(FVector(PosArray[0], PosArray[1], PosArray[2]));
//		}
//		if (ImGui::DragFloat3("Rotation", RotArray, 0.1f))
//		{
//			Gizmo.SetTargetRotation(FVector(RotArray[0], RotArray[1], RotArray[2]));
//		}
//		if (ImGui::DragFloat3("Scale", ScaleArray, 0.1f))
//		{
//			Gizmo.SetTargetScale(FVector(ScaleArray[0], ScaleArray[1], ScaleArray[2]));
//		}
//
//		SEPARATOR();
//
//		if (ImGui::Button("Remove Object") && ObjectPicked) {
//			ObjectPicked->bPendingKill = true;
//			if (ObjectPicked->IsA<USceneComponent>()) {
//				USceneComponent* SceneComp = ObjectPicked->Cast<USceneComponent>();
//				if (SceneComp->GetOwningActor()) {
//					// TODO:: Do this recursively
//					UObjectManager::Get().DestroyObject(SceneComp->GetOwningActor());
//				}
//			}
//			Viewport->GetGizmoManager().SetVisibility(false);
//			Viewport->GetGizmoManager().Deactivate();
//			ObjectPicked = nullptr;
//		}
//
//		if (SceneComp->GetOwningActor()) { RenderPickedActorWindow(SceneComp->GetOwningActor()); }
//	}
//
//	ImGui::End();
//}
//
//static void InsepctPickedActor(AActor* Actor) {
//
//}