#pragma once

#include "CoreMinimal.h"
#include "Mesh/ObjImporter.h"

class AActor;
class FCore;
class FEditorUI;
class FEditorViewportClient;

class FObjViewerPanel
{
public:
	void Render(FCore* Core, FEditorViewportClient* ActiveViewportClient, FEditorUI& EditorUI);
	FVector GetDisplayedLocation(AActor* Actor, FEditorViewportClient* ViewportClient) const;
	void ApplyDisplayedLocation(AActor* Actor, FEditorViewportClient* ViewportClient, const FVector& DisplayLocation) const;

private:
	FString DraftAxisAssetPath;
	FObjImporter::FImportAxisMapping DraftImportAxisMapping = FObjImporter::MakeDefaultImportAxisMapping();
	FObjImporter::FImportAxisMapping LastAppliedImportAxisMapping = FObjImporter::MakeDefaultImportAxisMapping();
};
