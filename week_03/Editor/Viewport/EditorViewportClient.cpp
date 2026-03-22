#include "Editor/Viewport/EditorViewportClient.h"


#include <iostream>

#include "Editor/Core/EditorConsole.h"
#include "Engine/Core/InputSystem.h"

#include "Engine/Scene/Camera.h"
#include "World.h"
#include "World/GizmoComponent.h"
#include "World/PrimitiveComponent.h"

void FEditorViewportClient::Initialize(HWND InHWindow)
{
	HWindow = InHWindow;
	Gizmo = UObjectManager::Get().CreateObject<UGizmoComponent>();
	Gizmo.lock()->SetWorldLocation(FVector(0.0f, 0.0f, 0.0f));
	Gizmo.lock()->Deactivate();

	Camera = UObjectManager::Get().CreateObject<UCamera>();
	ResetCamera(Camera);
	Camera.lock()->ApplyCameraState();


	UE_LOG("Hello ZZup Engine! %d", 2026);
}

void FEditorViewportClient::SetViewportSize(float InWidth, float InHeight)
{
	if (InWidth > 0.0f)
	{
		WindowWidth = InWidth;
	}

	if (InHeight > 0.0f)
	{
		WindowHeight = InHeight;
	}

	if (Camera.lock())
	{
		Camera.lock()->OnResize(static_cast<int>(WindowWidth), static_cast<int>(WindowHeight));
	}
}

void FEditorViewportClient::Tick(float DeltaTime)
{
	TickInput(DeltaTime);
	TickInteraction(DeltaTime);
	TickCursorOverlay(DeltaTime);
}

void FEditorViewportClient::FlushViewOutput() {
	ViewOutput = {};
}

void FEditorViewportClient::TickInput(float DeltaTime)
{
	auto Camerptr = Camera.lock();
	if (!Camerptr)
	{
		return;
	}

	if (InputSystem::GuiInputState.bUsingKeyboard == true)
	{
		return;
	}

	FCameraState& CameraState = Camerptr->GetCameraState();

	FVector move = FVector(0, 0, 0);

	if (InputSystem::GetKey('W') && !CameraState.bIsOrthogonal)
		move.X += CameraVelocity;
	if (InputSystem::GetKey('A'))
		move.Y -= CameraVelocity;
	if (InputSystem::GetKey('S') && !CameraState.bIsOrthogonal)
		move.X -= CameraVelocity;
	if (InputSystem::GetKey('D'))
		move.Y += CameraVelocity;
	if(InputSystem::GetKey('Q'))
		move.Z -= CameraVelocity;
	if (InputSystem::GetKey('E'))
		move.Z += CameraVelocity;

	move *= DeltaTime;
	Camerptr->MoveLocal(move);

	FVector rotation = FVector(0, 0, 0);
	FVector mouseRotation = FVector(0, 0, 0);

	float AngleVelocity = 50.0f;
	if (InputSystem::GetKey(VK_UP))
		rotation.Z -= AngleVelocity;
	if (InputSystem::GetKey(VK_LEFT))
		rotation.Y -= AngleVelocity;
	if (InputSystem::GetKey(VK_DOWN))
		rotation.Z += AngleVelocity;
	if (InputSystem::GetKey(VK_RIGHT))
		rotation.Y += AngleVelocity;

	// Mouse sensitivity is degrees per pixel (do not multiply by DeltaTime)
	float MouseRotationSpeed = 0.15f;

	if (InputSystem::GetKey(VK_RBUTTON))
	{
		float deltaX = static_cast<float>(InputSystem::MouseDeltaX());
		float deltaY = static_cast<float>(InputSystem::MouseDeltaY());

		mouseRotation.Y += deltaX * MouseRotationSpeed; // yaw
		mouseRotation.Z += deltaY * MouseRotationSpeed; // pitch

		mouseRotation.Y = Clamp(mouseRotation.Y, -89.0f, 89.0f);
		mouseRotation.Z = Clamp(mouseRotation.Z, -89.0f, 89.0f);
	}

	if (InputSystem::GetKeyUp(VK_SPACE))
		Gizmo.lock()->SetNextMode();

	rotation *= DeltaTime;
	Camerptr->Rotate(rotation.Y + mouseRotation.Y, rotation.Z + mouseRotation.Z);

	if (InputSystem::GetKeyDown('O')) {
		CameraState.bIsOrthogonal = !CameraState.bIsOrthogonal;
	}
}

void FEditorViewportClient::TickInteraction(float DeltaTime)
{
	(void)DeltaTime;
	auto Cameraptr = Camera.lock();
	auto Gizmoptr = Gizmo.lock();
	auto Sceneptr = Scene.lock();
	if (!Cameraptr || !Gizmoptr || !Sceneptr)
	{
		return;
	}

	Gizmoptr->ApplyScreenSpaceScaling(Cameraptr->GetWorldLocation());

	if (InputSystem::GuiInputState.bUsingMouse)
	{
		return;
	}


	FCameraState& CameraState = Cameraptr->GetCameraState();
	uint32 zoomspeed = 300;

	float scrollNotches = InputSystem::GetScrollNotches();
	if (scrollNotches != 0.0f) {
		if (CameraState.bIsOrthogonal) {
			float newWidth = CameraState.OrthoWidth - scrollNotches * zoomspeed * DeltaTime;
			CameraState.OrthoWidth = Clamp(newWidth, 0.1f, 1000.0f);
		}
		else {
			float newFOV = CameraState.FOV - scrollNotches * zoomspeed * DeltaTime;
			newFOV = Clamp(newFOV, 1.f * DEG_TO_RAD, 90.0f * DEG_TO_RAD);
			CameraState.FOV = newFOV;
		}
	}
	

	POINT mousepoint = InputSystem::mousePos;
	ScreenToClient(HWindow, &mousepoint);

	//	Cursor
	CursorOverlayState.ScreenX = static_cast<float>(mousepoint.x);
	CursorOverlayState.ScreenY = static_cast<float>(mousepoint.y);

	if (InputSystem::GetKeyDown(VK_LBUTTON))
	{
		CursorOverlayState.bPressed = true;
		CursorOverlayState.bVisible = true;
		CursorOverlayState.TargetRadius = CursorOverlayState.MaxRadius;
		CursorOverlayState.Color = FVector4(1.0f, 1.0f, 0.0f, 1.0f);  // Yellow for left-click

		if(bIsCursorVisible)
		{
			while(ShowCursor(FALSE) >= 0);
			bIsCursorVisible = false;
		}

	}

	if (InputSystem::GetKeyUp(VK_LBUTTON))
	{
		CursorOverlayState.bPressed = false;
		CursorOverlayState.TargetRadius = 0.0f;

		if(!bIsCursorVisible)
		{
			while(ShowCursor(TRUE) < 0);
			bIsCursorVisible = true;
		}
	}

	if (InputSystem::GetKeyDown(VK_RBUTTON))
	{
		CursorOverlayState.bPressed = true;
		CursorOverlayState.bVisible = true;
		CursorOverlayState.TargetRadius = CursorOverlayState.MaxRadius;
		CursorOverlayState.Color = FVector4(0.0f, 0.0f, 1.0f, 1.0f);  // Blue for right-click

		if(bIsCursorVisible)
		{
			while(ShowCursor(FALSE) >= 0);
			bIsCursorVisible = false;
		}
	}

	if (InputSystem::GetKeyUp(VK_RBUTTON))
	{
		CursorOverlayState.bPressed = false;
		CursorOverlayState.TargetRadius = 0.0f;

		if(!bIsCursorVisible)
		{
			while(ShowCursor(TRUE) < 0);
			bIsCursorVisible = true;
		}
	}
	
	FRay ray = Cameraptr->DeprojectScreenToWorld(mousepoint.x, mousepoint.y, WindowWidth, WindowHeight);
	FHitResult hitResult;

	//Gizmo Hover
	Gizmoptr->Raycast(ray, hitResult);

	if (InputSystem::GetKeyDown(VK_LBUTTON))
	{
		HandleDragStart(ray);
	}
	else if (InputSystem::GetLeftDragging())
	{
		//	눌려있고, Holding되지 않았다면 다음 Loop부터 드래그 업데이트 시작
		if (Gizmoptr->IsPressedOnHandle() && !Gizmoptr->IsHolding())
		{
			Gizmoptr->SetHolding(true);
		}

		if (Gizmoptr->IsHolding())
		{
			Gizmoptr->UpdateDrag(ray);
		}
	}
	else if (InputSystem::GetLeftDragEnd())
	{
		Gizmoptr->DragEnd();
	}
	//else if (InputSystem::GetKeyUp(VK_LBUTTON) && !Gizmo->HasTarget())
	//{
	//	std::cout << "Gizmo Deactivated\n";
	//	Gizmo->DragEnd();
	//}

	//if (InputSystem::GetRightDragStart())
	//{
	//	Gizmo->DragEnd();
	//}
}

void FEditorViewportClient::ResetCamera(weak_ptr<UCamera> Camera) {
	if (auto Cameraptr = Camera.lock()) {
		Cameraptr->SetWorldLocation(InitViewPos);
		Cameraptr->LookAt(InitLookAt);
	}
}

void FEditorViewportClient::SyncCameraFromRenderHandler()
{
	if (auto Cameraptr = Camera.lock())
	{
		Cameraptr->ApplyCameraState();
	}
}

void FEditorViewportClient::ResetViewport() {
	Gizmo.lock()->Deactivate();
	Gizmo.lock()->SetWorldLocation(FVector(0.0f, 0.0f, 0.0f));
	Camera.lock()->bPendingKill = true;
	UObjectManager::Get().CollectGarbage();

	Camera = UObjectManager::Get().CreateObject<UCamera>();
	SetViewportSize(WindowWidth, WindowHeight);
	Camera.lock()->ApplyCameraState();
	ResetCamera(Camera.lock());
	SyncCameraFromRenderHandler();
}

void FEditorViewportClient::HandleDragStart(const FRay& Ray)
{
	FHitResult hitResult{};
	auto Gizmoptr = Gizmo.lock();
	if (!Gizmoptr) { return; }
	if (Gizmoptr->Raycast(Ray, hitResult))
	{
		Gizmoptr->SetPressedOnHandle(true);
		//Gizmo->SetHolding(true);
		FString PickLog = "Gizmo is Holding: " + ViewOutput.ObjectPicked;
		UE_LOG(PickLog.c_str(), true);
	}
	else
	{
		FlushViewOutput();
		UPrimitiveComponent* bestTarget = nullptr;
		float closetDistance = FLT_MAX;

		auto Sceneptr = Scene.lock();
		if (!Sceneptr) { return; }
		for (auto* it : Sceneptr->GetActors())
		{
			if (!it || it->bPendingKill || (it->GetRootComponent() && it->GetRootComponent()->bPendingKill)) {
				continue;
			}
			UPrimitiveComponent* primitiveComp = dynamic_cast<UPrimitiveComponent*>(it->GetRootComponent());
			if (primitiveComp != nullptr)
			{
				hitResult = {};
				if (primitiveComp->Raycast(Ray, hitResult))
				{
					if (hitResult.Distance < closetDistance)
					{
						closetDistance = hitResult.Distance;
						bestTarget = primitiveComp;

						ViewOutput.ObjectPicked = primitiveComp->GetTypeInfo()->name;
						ViewOutput.Object = primitiveComp;
					}
				}
			}
		}

		if (bestTarget == nullptr)
		{
			Gizmoptr->Deactivate();
		}
		else
		{
			Gizmoptr->SetTarget(bestTarget);
		}
	}
}

void FEditorViewportClient::TickCursorOverlay(float DeltaTime)
{
	const float Alpha = std::min(1.0f, DeltaTime * CursorOverlayState.LerpSpeed);
	CursorOverlayState.CurrentRadius += (CursorOverlayState.TargetRadius - CursorOverlayState.CurrentRadius) * Alpha;

	if(!CursorOverlayState.bPressed && CursorOverlayState.CurrentRadius < 0.01f)
	{
		CursorOverlayState.CurrentRadius = 0.0f;
		CursorOverlayState.bVisible = false;
	}
}

void FEditorViewportClient::CloseViewport() {
	Gizmo.lock()->bPendingKill = true;
}