#pragma once
#include "Math/Vector.h"
#include "imgui.h"
#include <functional>
class FCore;
class UStaticMeshComponent;
class AStaticMeshActor;
class AActor;
using FPropertyChangedCallback = std::function<void(const FVector&, const FVector&, const FVector&)>;

class FPropertyWindow
{
public:
	void Render(FCore* Core);
	void SetTarget(const FVector& Location, const FVector& Rotation, const FVector& Scale,
		const char* ActorName = nullptr);

	bool    IsModified()    const { return bModified; }
	FVector GetLocation()   const { return EditLocation; }
	FVector GetRotation()   const { return EditRotation; }
	FVector GetScale()      const { return EditScale; }

	void SetOnChanged(FPropertyChangedCallback Callback) { OnChanged = Callback; }

	FPropertyChangedCallback OnChanged;
private:
	void DrawTransformSection();

	void DrawBillboardSection(AActor* SelectedActor);

	//Property 패널에서 머테리얼 슬롯 및 UV 스크롤을 컨트롤합니다.
	void DrawMaterialSlots(FCore* Core, UStaticMeshComponent* SMComp, AActor* SelectedActor);
	void DrawUVScrollControls(UStaticMeshComponent* SMComp, uint32 SlotIdx);

	void DrawStaticMeshSection(FCore* Core, AStaticMeshActor* SMActor);

	FVector EditLocation = { 0.0f, 0.0f, 0.0f };
	FVector EditRotation = { 0.0f, 0.0f, 0.0f };
	FVector EditScale = { 1.0f, 1.0f, 1.0f };
	char    ActorNameBuf[128] = "None";
private:
	int SelectedMeshIndex = -1;
	int SelectedTextureIndex = -1;
	int SelectedMaterialIndex = -1;
	bool    bModified = false;
};
