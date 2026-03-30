#pragma once
#include "OutlinerWindow.h"
#include "ControlPanelWindow.h"
#include "PropertyWindow.h"
#include "ConsoleWindow.h"
#include "StatWindow.h"
#include "Viewport.h"
#include "Types/ObjectPtr.h"
#include "ContentBrowserWindow.h"
#include <vector>

class FCore;
class FWindow;
class FRenderer;
class AActor;

class IViewportClient;

class SWindow;
class SSplitter;

class FEditorUI
{
public:
	void Initialize(FCore* InCore);
	void SetupWindow(FWindow* InWindow);
	void AttachToRenderer(FRenderer* InRenderer);
	void DetachFromRenderer(FRenderer* InRenderer);
	void Render();
	void SyncSelectedActorProperty();

	bool GetViewportMousePosition(int32 WindowMouseX, int32 WindowMouseY,
		int32& OutViewportX, int32& OutViewportY,
		int32& OutWidth, int32& OutHeight) const;
	bool IsViewportInteractive() const;
	bool HandleInput(HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam);
	FConsoleWindow& GetConsole() { return Console; }
	FCore* GetCore() { return Core; }

	void LinkViewportClient(int32 Index, IViewportClient* InClient);

	IViewportClient* GetFocusedViewportClient() const;
	IViewportClient* GetPrimaryViewportClient() const;
	IViewportClient* GetViewportClientAt(int32 Index) const;

	// 현재 마우스가 올라가 있는 뷰포트 인덱스 (-1이면 없음)
	int32 GetHoveredViewportIndex() const;

	// 뷰포트 인덱스 기준 마우스 로컬 좌표 반환
	bool GetMousePositionInViewport(int32 ViewportIndex,
		int32 WindowMouseX, int32 WindowMouseY,
		int32& OutLocalX, int32& OutLocalY,
		int32& OutWidth, int32& OutHeight) const;

private:
	void BuildDefaultLayout(uint32 DockID);
	void LoadEditorSettings();
	void SaveEditorSettings();
	std::wstring GetEditorIniPathW() const;
	FViewport* GetViewportAt(int32 Index);

	FCore* Core = nullptr;
	TObjectPtr<AActor> CachedSelectedActor;
	FWindow* MainWindow = nullptr;

	SSplitter* RootWindow;
	TArray<SSplitter*> DraggedSplitters;
	TArray<SWindow*> Windows;	//뷰포트를 소유한 Window만 저장

	FControlPanelWindow ControlPanel;
	FPropertyWindow Property;
	FConsoleWindow Console;
	FStatWindow Stat;
	FOutlinerWindow Outliner;
	FContentBrowserWindow ContentBrowser;

	bool bWindowSetup = false;
	bool bViewportActive = false;
	bool bLayoutInitialized = false;
	FRenderer* CurrentRenderer = nullptr;

	bool bIsCliked = false;
	FVector2 PreviousMousePoint;
	FVector2 DeltaMouse;
	FVector2 CachedViewportScreenPos;
};