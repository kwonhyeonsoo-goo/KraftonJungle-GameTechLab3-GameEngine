#pragma once

#include "Core/ViewportClient.h"
#include "Gizmo/Gizmo.h"
#include "Picking/Picker.h"
#include "Types/CoreTypes.h"
#include "CameraFuction/FCameraFunctionManager.h"
#include "CameraFuction/FPersToPers.h"
#include "CameraFuction/FPerspectToOrtho.h"
#include "CameraFuction/FOrthoToPerspect.h"
#include "CameraFuction/FOrthoToOrtho.h"
#include <unordered_map>

class FPersToPers;
class FEditorUI;
class FWindow;
class AActor;
class ULevel;
class UWorld;
class FMaterial;
struct FMeshData;
struct FRenderCommandQueue;
class SViewportWindow;

enum class ERenderMode
{
	Lighting,
	NoLighting,
	Wireframe,
	SolidWireframe,
	UV,
	Normals,
};

enum class EEditorViewportType : uint8_t
{
	Perspective,
	Orthographic,
	Top,
	Front,
	Right
};

class FEditorViewportClient : public FViewportClient
{
public:
	FEditorViewportClient(FEditorUI& InEditorUI, FWindow* InMainWindow, EEditorViewportType InViewportType, ELevelType InWorldType);

	void Attach(FCore* Core) override;
	void Detach() override;
	void HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam) override;
	UWorld* ResolveWorld(FCore* Core) const override;
	const char* GetViewportLabel() const override;
	EGizmoMode GetGizmoMode() const { return Gizmo.GetMode(); }
	void SetGizmoMode(EGizmoMode InMode) { Gizmo.SetMode(InMode); }
	ERenderMode GetRenderMode() const { return RenderMode; }
	void SetRenderMode(ERenderMode InRenderMode) { RenderMode = InRenderMode; }
	EEditorViewportType GetViewportType() const { return CameraViewType; }
	bool SupportsEditingTools() const { return WorldType == ELevelType::Editor; }
	void DrawUI() override;
	void HandleFileDoubleClick(const FString& FilePath);
	void HandleFileDropOnViewport(const FString& FilePath);
	void BuildRenderCommands(TArray<AActor*>& InActors, FRenderCommandQueue& OutQueue) override;
	void PostRender(FCore* Core, FRenderer* Renderer) override;
	void Tick(float DeltaTime) override;
	float GetGridSize() const { return GridSize; }
	void SetGridSize(float InSize);
	float GetLineThickness() const { return LineThickness; }
	void SetLineThickness(float InThickness);
	bool IsGridVisible() const;
	void SetGridVisible(bool bVisible);
	void SaveInitialCameraState();
	void ResetCameraToInitialState();
	void RefreshObjViewerCameraPivot(AActor* TargetActor = nullptr);
	void FrameObjViewerCamera(AActor* TargetActor, bool bSaveInitialState);
	float GetObjViewerBottomZ(AActor* TargetActor) const;

protected:
	virtual void ConfigureDefaultView();
	virtual void DrawControllerOptions();
	virtual void DrawViewportSpecificOptions();
	virtual void ProcessCameraInput(FCore* Core, float DeltaTime) override;
	virtual void OnMouseButtonDown(UINT Msg, WPARAM WParam, LPARAM LParam);
	virtual void OnMouseButtonUp(UINT Msg, WPARAM WParam, LPARAM LParam);
	virtual void OnMouseMove(WPARAM WParam, LPARAM LParam);
	virtual void OnMouseWheel(float WheelDelta, WPARAM WParam, LPARAM LParam);
	virtual void OnKeyDown(WPARAM WParam, LPARAM LParam);
	virtual void OnKeyUp(WPARAM WParam, LPARAM LParam);
	bool CanUseEditingTools(FCore* Core, ULevel*& OutLevel, UWorld*& OutWorld) const;
	void HandleEditorHotkeys(WPARAM WParam, bool bRightMouseDown);
	void HandleSelectionClick(FCore* Core, UWorld* World, AActor* SelectedActor);
	void HandleMouseMoveForTools(AActor* SelectedActor);
	void HandleMouseReleaseForTools();
	AActor* GetSelectedActor() const;
	AActor* GetGizmoTarget() const;
	void CreateGridResource(FRenderer* Renderer);
	void CreateViewerDebugMaterials(FRenderer* Renderer);
	void ApplyViewerNoCull(FMaterial* Material) const;
	void ShowViewOptionPanel();
	void DrawCameraOption();
	void StartOrthographicTransition();
	void StartPerspectiveTransition();
	void StartOrthoTransition(EEditorViewportType OrthoViewType);
	void ResetPerspectiveMovementState();
	void ApplyPerspectiveLookInput(float MouseDeltaX, float MouseDeltaY);
	void ApplyPerspectivePanInput(float MouseDeltaX, float MouseDeltaY, float DeltaTime);
	void UpdateOrthoCameraTransform();
	FVector GetOrthoForwardVector() const;
	FVector GetOrthoRightVector() const;
	FVector GetOrthoUpVector() const;
	static EEditorViewportType GetOrthoViewTypeFromViewportType(EEditorViewportType InViewportType);
	FVector ResolveTransitionPivot() const;
	FVector GetViewportUpVector() const;
	FMatrix GetGridWorldMatrix() const;

	FEditorUI& EditorUI;
	FWindow* MainWindow = nullptr;
	FPicker Picker;
	mutable FGizmo Gizmo;

	FCameraFunctionManager CameraFunctionManager;
	FPersToPers FocusCameraFunction;
	FPerspectToOrtho ChangePersToOrthFunction;
	FOrthoToPerspect ChangeOrthoToPersFunction;
	FOrthoToOrtho ChangeOrthoToOrthoFunction;

	ERenderMode RenderMode = ERenderMode::Lighting;
	const FString WireframeMaterialName = "M_Wireframe";
	const FString SolidWireframeMaterialName = "M_SolidWireframe";
	std::shared_ptr<FMaterial> WireFrameMaterial = nullptr;
	std::shared_ptr<FMaterial> SolidWireFrameMaterial = nullptr;
	std::shared_ptr<FMaterial> ViewerUVMaterial = nullptr;
	std::shared_ptr<FMaterial> ViewerNormalMaterial = nullptr;
	std::unordered_map<const FMeshData*, std::shared_ptr<FMeshData>> SolidWireframeMeshCache;

	std::unique_ptr<FMeshData> GridMesh;
	std::shared_ptr<FMaterial> GridMaterial;
	float GridSize = 1.0f;
	float LineThickness = 1.0f;
	bool bShowGrid = true;
	FVector InitialCameraPosition = FVector::ZeroVector;
	FVector InitialCameraTarget = FVector::ZeroVector;
	float InitialCameraYaw = 0.0f;
	float InitialCameraPitch = 0.0f;
	float InitialCameraFOV = 45.0f;
	bool bHasInitialCameraState = false;
	FVector ResetAnimationStartPosition = FVector::ZeroVector;
	FVector ResetAnimationStartTarget = FVector::ZeroVector;
	float ResetAnimationStartYaw = 0.0f;
	float ResetAnimationStartPitch = 0.0f;
	float ResetAnimationStartFOV = 45.0f;
	float ResetAnimationElapsed = 0.0f;
	float ResetAnimationDuration = 0.25f;
	bool bResetCameraAnimating = false;
	EEditorViewportType CameraViewType = EEditorViewportType::Perspective;

	// Perspective interaction state
	bool bPerspectiveRotating = false;
	bool bPerspectivePanning = false;
	bool bMoveForward = false;
	bool bMoveBackward = false;
	bool bMoveRight = false;
	bool bMoveLeft = false;
	bool bMoveUp = false;
	bool bMoveDown = false;

	// Ortho interaction state
	FVector OrthoCenter = FVector::ZeroVector;
	float OrthoViewDistance = 25.0f;
	float PendingOrthoZoomStep = 0.0f;
	bool bOrthoPanning = false;
};
