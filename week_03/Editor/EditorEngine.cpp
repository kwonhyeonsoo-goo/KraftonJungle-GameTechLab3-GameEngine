#include "Editor/EditorEngine.h"
#include "Engine/Core/InputSystem.h"
#include "Render/Scene/RenderCollector.h"
#include "Render/Scene/RenderCollectorContext.h"
#include "World/Primitives/Primitives.h"

void FEditorEngine::Create(HWND InHWindow)
{
	HWindow = InHWindow;

	Renderer.Create(HWindow);
	FRenderCollector::Initialize(Renderer.GetFD3DDevice().GetDevice());
	RenderBus.Create(Renderer.GetFD3DDevice().GetDevice());
	EditorWorld = UObjectManager::Get().CreateObject<UWorld>();
	auto WorldPtr = EditorWorld.lock();
	WorldPtr->Init();
	WorldPtr->SpawnPrimitiveActor<UCubeComponent>(FVector(-3.f, 0, 0));	// Test sample

	RECT rect;
	GetClientRect(HWindow, &rect);
	WindowWidth = static_cast<float>(rect.right - rect.left);
	WindowHeight = static_cast<float>(rect.bottom - rect.top);

	ViewportClient.Initialize(HWindow);
	ViewportClient.SetViewportSize(WindowWidth, WindowHeight);
	ViewportClient.SetScene(WorldPtr->GetActiveScene());

	ViewportClient.SetViewportSize(WindowWidth, WindowHeight);
    MainPanel.Create(HWindow, Renderer, this, &ViewportClient);
}

void FEditorEngine::OnWindowResized(uint32 Width, uint32 Height)
{
	if (Width <= 0 || Height <= 0)
	{
		return;
	}

	WindowWidth = static_cast<float>(Width);
	WindowHeight = static_cast<float>(Height);
	ViewportClient.SetViewportSize(WindowWidth, WindowHeight);

	Renderer.GetFD3DDevice().OnResizeViewport(Width, Height);
}

void FEditorEngine::NewScene() {
	auto Worldptr = EditorWorld.lock();
	Worldptr->GetSceneManager().AddNewScene();
	ViewportClient.ResetViewport();
	ResetViewportScene();
}

void FEditorEngine::ResetViewportScene() {
	ViewportClient.SetScene(EditorWorld.lock()->GetActiveScene());
}

void FEditorEngine::Release()
{
	if (auto Worldptr = EditorWorld.lock()) {
		Worldptr->EndPlay();
	}
	FRenderCollector::Release();
	MainPanel.Release();
	RenderBus.Release();
	Renderer.Release();
}

void FEditorEngine::BeginPlay()
{
	if (auto Worldptr = EditorWorld.lock()) {
		Worldptr->BeginPlay();
	}
}

void FEditorEngine::BeginFrame(float DeltaTime)
{
	InputSystem::Update();
	ViewportClient.Tick(DeltaTime);
	MainPanel.Update();
	ViewportClient.SyncCameraFromRenderHandler();
}

void FEditorEngine::Update(float DeltaTime)
{
	
}

void FEditorEngine::Render(float DeltaTime)
{
	RenderBus.Clear();
	BuildRenderCommands();

	Renderer.BeginFrame();
	Renderer.Render(RenderBus);
	MainPanel.Render(DeltaTime, ViewportClient.GetViewOutput());
	Renderer.RenderOverlay(RenderBus);	//	UI가 그려진 후 Overlay 그리기
	Renderer.EndFrame();
}

void FEditorEngine::EndFrame()
{
	//UObjectManager::Get().CollectGarbage();
}

void FEditorEngine::UpdateWorld(float DeltaTime)
{
	if (auto Worldptr = EditorWorld.lock())
	{
		Worldptr->Tick(DeltaTime);
	}
}

void FEditorEngine::BuildRenderCommands()
{
	FRenderCollectorContext Context;
	Context.Scene = EditorWorld.lock()->GetActiveScene().lock().get();
	Context.Camera = ViewportClient.GetCamera().lock().get();
	Context.Gizmo = ViewportClient.GetGizmo().lock().get();
	Context.CursorOverlayState = &ViewportClient.GetCursorOverlayState();
	Context.ViewportHeight = WindowHeight;
	Context.ViewportWidth = WindowWidth;
	auto GizmoPtr = ViewportClient.GetGizmo().lock();
	Context.SelectedComponent = GizmoPtr && GizmoPtr->HasTarget() ? (UPrimitiveComponent*)GizmoPtr->GetTarget() : nullptr;
	Context.GridSize = GridSize;

	FRenderCollector::Collect(Context, RenderBus);
}
