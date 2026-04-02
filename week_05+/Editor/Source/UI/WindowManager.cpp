#include "WindowManager.h"
#include "FEditorEngine.h"
#include "Core/Viewport.h"
#include "Core/ViewportClient.h"
#include "Core/ViewportContext.h"
#include "Input/InputManager.h"
#include "EditorViewportClient.h"
#include "ViewportWindow.h"
#include <windowsx.h>
#include <cstdio>

namespace
{
	const wchar_t* const LayoutRootSection = L"ViewportLayout";
	const wchar_t* const LayoutRootKey = L"Root";
	const wchar_t* const LayoutTypeKey = L"Type";
	const wchar_t* const LayoutRatioKey = L"Ratio";
	const wchar_t* const LayoutRatioHorizontalKey = L"RatioH";
	const wchar_t* const LayoutRatioVerticalKey = L"RatioV";
	const wchar_t* const LayoutSideLTKey = L"LT";
	const wchar_t* const LayoutSideLBKey = L"LB";
	const wchar_t* const LayoutSideRTKey = L"RT";
	const wchar_t* const LayoutSideRBKey = L"RB";
	const wchar_t* const LayoutViewportTypeKey = L"ViewportType";

	bool IsMouseMessage(UINT Msg)
	{
		switch (Msg)
		{
		case WM_MOUSEMOVE:
		case WM_LBUTTONDOWN:
		case WM_LBUTTONDBLCLK:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONDBLCLK:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONDBLCLK:
		case WM_MBUTTONUP:
		case WM_MOUSEWHEEL:
		case WM_MOUSEHWHEEL:
			return true;
		default:
			return false;
		}
	}

	bool IsKeyboardMessage(UINT Msg)
	{
		switch (Msg)
		{
		case WM_KEYDOWN:
		case WM_KEYUP:
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
		case WM_CHAR:
		case WM_SYSCHAR:
		case WM_IME_CHAR:
			return true;
		default:
			return false;
		}
	}

	bool IsMouseButtonDownMessage(UINT Msg)
	{
		return
			Msg == WM_LBUTTONDOWN ||
			Msg == WM_LBUTTONDBLCLK ||
			Msg == WM_RBUTTONDOWN ||
			Msg == WM_RBUTTONDBLCLK ||
			Msg == WM_MBUTTONDOWN ||
			Msg == WM_MBUTTONDBLCLK;
	}

	bool IsMouseButtonUpMessage(UINT Msg)
	{
		return
			Msg == WM_LBUTTONUP ||
			Msg == WM_RBUTTONUP ||
			Msg == WM_MBUTTONUP;
	}

	bool IsMouseWheelMessage(UINT Msg)
	{
		return Msg == WM_MOUSEWHEEL || Msg == WM_MOUSEHWHEEL;
	}

	std::wstring BuildLayoutNodeSectionName(const std::wstring& NodeId)
	{
		return std::wstring(LayoutRootSection) + L"." + NodeId;
	}

	void WriteIniString(const std::wstring& IniPath, const std::wstring& Section, const wchar_t* Key, const std::wstring& Value)
	{
		::WritePrivateProfileStringW(Section.c_str(), Key, Value.c_str(), IniPath.c_str());
	}

	std::wstring ReadIniString(const std::wstring& IniPath, const std::wstring& Section, const wchar_t* Key)
	{
		wchar_t Buffer[256] = {};
		::GetPrivateProfileStringW(Section.c_str(), Key, L"", Buffer, 256, IniPath.c_str());
		return std::wstring(Buffer);
	}

	float ReadIniFloat(const std::wstring& IniPath, const std::wstring& Section, const wchar_t* Key, float DefaultValue)
	{
		const std::wstring Value = ReadIniString(IniPath, Section, Key);
		if (Value.empty())
		{
			return DefaultValue;
		}

		try
		{
			return std::stof(Value);
		}
		catch (...)
		{
			return DefaultValue;
		}
	}

	std::wstring ToWStringFloat(float Value)
	{
		wchar_t Buffer[64] = {};
		swprintf_s(Buffer, L"%.6f", Value);
		return std::wstring(Buffer);
	}

	std::wstring GetViewportTypeName(EEditorViewportType ViewportType)
	{
		switch (ViewportType)
		{
		case EEditorViewportType::Top:
			return L"Top";
		case EEditorViewportType::Front:
			return L"Front";
		case EEditorViewportType::Right:
			return L"Right";
		case EEditorViewportType::Perspective:
		default:
			return L"Perspective";
		}
	}

	EEditorViewportType ParseViewportType(const std::wstring& Value)
	{
		if (Value == L"Top")
		{
			return EEditorViewportType::Top;
		}
		if (Value == L"Front")
		{
			return EEditorViewportType::Front;
		}
		if (Value == L"Right")
		{
			return EEditorViewportType::Right;
		}
		return EEditorViewportType::Perspective;
	}

	struct FLayoutSerializer
	{
		const FWindowManager& WindowManager;
		const std::wstring& IniPath;
		int32 NextNodeIndex = 0;

		std::wstring SerializeNode(const SWindow* Window)
		{
			if (Window == nullptr)
			{
				return L"";
			}

			const std::wstring NodeId = L"Node" + std::to_wstring(NextNodeIndex++);
			const std::wstring Section = BuildLayoutNodeSectionName(NodeId);

			if (const SSplitterH* SplitterH = dynamic_cast<const SSplitterH*>(Window))
			{
				WriteIniString(IniPath, Section, LayoutTypeKey, L"SplitterH");
				WriteIniString(IniPath, Section, LayoutRatioKey, ToWStringFloat(SplitterH->GetSplitRatio()));
				WriteIniString(IniPath, Section, LayoutSideLTKey, SerializeNode(SplitterH->GetSideLT()));
				WriteIniString(IniPath, Section, LayoutSideRBKey, SerializeNode(SplitterH->GetSideRB()));
				return NodeId;
			}

			if (const SSplitterV* SplitterV = dynamic_cast<const SSplitterV*>(Window))
			{
				WriteIniString(IniPath, Section, LayoutTypeKey, L"SplitterV");
				WriteIniString(IniPath, Section, LayoutRatioKey, ToWStringFloat(SplitterV->GetSplitRatio()));
				WriteIniString(IniPath, Section, LayoutSideLTKey, SerializeNode(SplitterV->GetSideLT()));
				WriteIniString(IniPath, Section, LayoutSideRBKey, SerializeNode(SplitterV->GetSideRB()));
				return NodeId;
			}

			if (const SSplitterC* SplitterC = dynamic_cast<const SSplitterC*>(Window))
			{
				WriteIniString(IniPath, Section, LayoutTypeKey, L"SplitterC");
				WriteIniString(IniPath, Section, LayoutRatioHorizontalKey, ToWStringFloat(SplitterC->GetSplitRatioHorizontal()));
				WriteIniString(IniPath, Section, LayoutRatioVerticalKey, ToWStringFloat(SplitterC->GetSplitRatioVertical()));
				WriteIniString(IniPath, Section, LayoutSideLTKey, SerializeNode(SplitterC->GetSideLT()));
				WriteIniString(IniPath, Section, LayoutSideLBKey, SerializeNode(SplitterC->GetSideLB()));
				WriteIniString(IniPath, Section, LayoutSideRTKey, SerializeNode(SplitterC->GetSideRT()));
				WriteIniString(IniPath, Section, LayoutSideRBKey, SerializeNode(SplitterC->GetSideRB()));
				return NodeId;
			}

			WriteIniString(IniPath, Section, LayoutTypeKey, L"Viewport");
			if (const SViewportWindow* ViewportWindow = dynamic_cast<const SViewportWindow*>(Window))
			{
				const FViewportContext* Context = ViewportWindow->GetViewportContext();
				const FEditorViewportClient* EditorViewportClient = dynamic_cast<const FEditorViewportClient*>(Context ? Context->GetViewportClient() : nullptr);
				if (EditorViewportClient)
				{
					WriteIniString(IniPath, Section, LayoutViewportTypeKey, GetViewportTypeName(EditorViewportClient->GetViewportType()));
				}
			}

			return NodeId;
		}
	};

	struct FLayoutDeserializer
	{
		FWindowManager& WindowManager;
		const std::wstring& IniPath;

		SViewportWindow* CreateViewportWindow(const FRect& Rect, EEditorViewportType ViewportType) const
		{
			FEditorEngine* EditorEngine = dynamic_cast<FEditorEngine*>(GEngine);
			FViewportContext* Context = EditorEngine
				? EditorEngine->CreateEditorViewportContext(Rect, ViewportType)
				: WindowManager.CreateViewportContext(Rect);
			if (Context == nullptr)
			{
				return nullptr;
			}

			return new SViewportWindow(Rect, Context);
		}

		SWindow* DeserializeNode(const std::wstring& NodeId, const FRect& Rect) const
		{
			if (NodeId.empty())
			{
				return nullptr;
			}

			const std::wstring Section = BuildLayoutNodeSectionName(NodeId);
			const std::wstring Type = ReadIniString(IniPath, Section, LayoutTypeKey);
			if (Type.empty())
			{
				return nullptr;
			}

			if (Type == L"Viewport")
			{
				const EEditorViewportType ViewportType = ParseViewportType(ReadIniString(IniPath, Section, LayoutViewportTypeKey));
				return CreateViewportWindow(Rect, ViewportType);
			}

			if (Type == L"SplitterH")
			{
				SSplitterH* Splitter = new SSplitterH(Rect, nullptr, nullptr, ReadIniFloat(IniPath, Section, LayoutRatioKey, 0.5f));
				SWindow* SideLT = DeserializeNode(ReadIniString(IniPath, Section, LayoutSideLTKey), Splitter->GetSideLTRect());
				SWindow* SideRB = DeserializeNode(ReadIniString(IniPath, Section, LayoutSideRBKey), Splitter->GetSideRBRect());
				if (SideLT == nullptr || SideRB == nullptr)
				{
					delete SideLT;
					delete SideRB;
					delete Splitter;
					return nullptr;
				}
				Splitter->SetSideLT(SideLT);
				Splitter->SetSideRB(SideRB);
				return Splitter;
			}

			if (Type == L"SplitterV")
			{
				SSplitterV* Splitter = new SSplitterV(Rect, nullptr, nullptr, ReadIniFloat(IniPath, Section, LayoutRatioKey, 0.5f));
				SWindow* SideLT = DeserializeNode(ReadIniString(IniPath, Section, LayoutSideLTKey), Splitter->GetSideLTRect());
				SWindow* SideRB = DeserializeNode(ReadIniString(IniPath, Section, LayoutSideRBKey), Splitter->GetSideRBRect());
				if (SideLT == nullptr || SideRB == nullptr)
				{
					delete SideLT;
					delete SideRB;
					delete Splitter;
					return nullptr;
				}
				Splitter->SetSideLT(SideLT);
				Splitter->SetSideRB(SideRB);
				return Splitter;
			}

			if (Type == L"SplitterC")
			{
				SSplitterC* Splitter = new SSplitterC(
					Rect,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					ReadIniFloat(IniPath, Section, LayoutRatioHorizontalKey, 0.5f),
					ReadIniFloat(IniPath, Section, LayoutRatioVerticalKey, 0.5f));
				SWindow* SideLT = DeserializeNode(ReadIniString(IniPath, Section, LayoutSideLTKey), Splitter->GetSideLTRect());
				SWindow* SideLB = DeserializeNode(ReadIniString(IniPath, Section, LayoutSideLBKey), Splitter->GetSideLBRect());
				SWindow* SideRT = DeserializeNode(ReadIniString(IniPath, Section, LayoutSideRTKey), Splitter->GetSideRTRect());
				SWindow* SideRB = DeserializeNode(ReadIniString(IniPath, Section, LayoutSideRBKey), Splitter->GetSideRBRect());
				if (SideLT == nullptr || SideLB == nullptr || SideRT == nullptr || SideRB == nullptr)
				{
					delete SideLT;
					delete SideLB;
					delete SideRT;
					delete SideRB;
					delete Splitter;
					return nullptr;
				}
				Splitter->SetSideLT(SideLT);
				Splitter->SetSideLB(SideLB);
				Splitter->SetSideRT(SideRT);
				Splitter->SetSideRB(SideRB);
				return Splitter;
			}

			return nullptr;
		}
	};
}

FWindowManager::~FWindowManager()
{
	Shutdown();
}

void FWindowManager::Initialize(FInputManager* InInputManager, FEnhancedInputManager* InEnhancedInputManager, std::function<FViewportContext*(const FRect&)> InViewportContextFactory)
{
	InputManager = InInputManager;
	EnhancedInputManager = InEnhancedInputManager;
	ViewportContextFactory = std::move(InViewportContextFactory);
}

void FWindowManager::Shutdown()
{
	ResetWindowTree();
	InputManager = nullptr;
	EnhancedInputManager = nullptr;
	ViewportContextFactory = {};
}

void FWindowManager::ResetWindowTree()
{
	if (MouseCaptureWindow && ::GetCapture() != nullptr)
	{
		::ReleaseCapture();
	}

	HoveredWindow = nullptr;
	PressedWindow = nullptr;
	MouseCaptureWindow = nullptr;
	KeyboardFocusWindow = nullptr;
	ActiveViewportWindow = nullptr;
	for (SWindow* Window : PendingDestroyWindows)
	{
		delete Window;
	}
	PendingDestroyWindows.clear();
	for (SWindow* Window : Windows)
	{
		delete Window;
	}
	Windows.clear();
}

FViewportContext* FWindowManager::CreateViewportContext(const FRect& Rect) const
{
	return ViewportContextFactory ? ViewportContextFactory(Rect) : nullptr;
}

void FWindowManager::SetRootRect(const FRect& InRect)
{
	if (Windows.empty() || !Windows[0])
	{
		return;
	}

	Windows[0]->SetRect(InRect);
}

void FWindowManager::CheckParent()
{
	for(int i = 0; i < Windows.size(); ++i)
	{
		if (!Windows[i])
			continue;

		if(!Windows[i]->GetParent())
			continue;

		Windows[i] = Windows[i]->GetParent();
	}
}

FPoint FWindowManager::ScreenToClient(HWND Hwnd, const FPoint& ScreenPoint) const
{
	POINT ClientPoint
	{
		static_cast<LONG>(ScreenPoint.X),
		static_cast<LONG>(ScreenPoint.Y)
	};

	if (Hwnd)
	{
		::ScreenToClient(Hwnd, &ClientPoint);
	}

	return FPoint(static_cast<float>(ClientPoint.x), static_cast<float>(ClientPoint.y));
}

bool FWindowManager::TryGetClientMousePoint(HWND Hwnd, UINT Msg, LPARAM LParam, FPoint& OutPoint) const
{
	if (IsMouseWheelMessage(Msg))
	{
		const FPoint ScreenPoint(static_cast<float>(GET_X_LPARAM(LParam)), static_cast<float>(GET_Y_LPARAM(LParam)));
		OutPoint = ScreenToClient(Hwnd, ScreenPoint);
		return true;
	}

	if (!IsMouseMessage(Msg))
	{
		return false;
	}

	OutPoint = FPoint(static_cast<float>(GET_X_LPARAM(LParam)), static_cast<float>(GET_Y_LPARAM(LParam)));
	return true;
}

SViewportWindow* FWindowManager::FindViewportWindow(SWindow* Window) const
{
	SWindow* Current = Window;
	while (Current)
	{
		if (SViewportWindow* ViewportWindow = dynamic_cast<SViewportWindow*>(Current))
		{
			return ViewportWindow;
		}

		Current = Current->GetParent();
	}

	return nullptr;
}

void FWindowManager::SetHoveredWindow(SWindow* NewHoveredWindow)
{
	if (HoveredWindow == NewHoveredWindow)
	{
		return;
	}

	if (SViewportWindow* PreviousViewportWindow = FindViewportWindow(HoveredWindow))
	{
		if (FViewportContext* PreviousContext = PreviousViewportWindow->GetViewportContext())
		{
			if (FViewport* PreviousViewport = PreviousContext->GetViewport())
			{
				PreviousViewport->SetHovered(false);
			}
		}
	}

	HoveredWindow = NewHoveredWindow;

	if (SViewportWindow* HoveredViewportWindow = FindViewportWindow(HoveredWindow))
	{
		if (FViewportContext* HoveredContext = HoveredViewportWindow->GetViewportContext())
		{
			if (FViewport* HoveredViewport = HoveredContext->GetViewport())
			{
				HoveredViewport->SetHovered(true);
			}
		}
	}
}

void FWindowManager::SetPressedWindow(SWindow* NewPressedWindow)
{
	PressedWindow = NewPressedWindow;
}

void FWindowManager::SetMouseCaptureWindow(HWND Hwnd, SWindow* NewMouseCaptureWindow)
{
	if (MouseCaptureWindow == NewMouseCaptureWindow)
	{
		return;
	}

	if (SViewportWindow* PreviousViewportWindow = FindViewportWindow(MouseCaptureWindow))
	{
		if (FViewportContext* PreviousContext = PreviousViewportWindow->GetViewportContext())
		{
			PreviousContext->SetCapturing(false);
		}
	}

	MouseCaptureWindow = NewMouseCaptureWindow;

	if (MouseCaptureWindow && Hwnd)
	{
		::SetCapture(Hwnd);
	}
	else if (!MouseCaptureWindow && ::GetCapture() == Hwnd)
	{
		::ReleaseCapture();
	}

	if (SViewportWindow* CaptureViewportWindow = FindViewportWindow(MouseCaptureWindow))
	{
		if (FViewportContext* CaptureContext = CaptureViewportWindow->GetViewportContext())
		{
			CaptureContext->SetCapturing(true);
		}
	}
}

void FWindowManager::ReleaseMouseCapture(HWND Hwnd)
{
	SetMouseCaptureWindow(Hwnd, nullptr);
}

void FWindowManager::SetKeyboardFocusWindow(SWindow* NewKeyboardFocusWindow)
{
	if (KeyboardFocusWindow == NewKeyboardFocusWindow)
	{
		return;
	}

	if (SViewportWindow* PreviousViewportWindow = FindViewportWindow(KeyboardFocusWindow))
	{
		if (FViewportContext* PreviousContext = PreviousViewportWindow->GetViewportContext())
		{
			if (FViewport* PreviousViewport = PreviousContext->GetViewport())
			{
				PreviousViewport->SetFocused(false);
			}
		}
	}

	KeyboardFocusWindow = NewKeyboardFocusWindow;

	if (SViewportWindow* FocusViewportWindow = FindViewportWindow(KeyboardFocusWindow))
	{
		if (FViewportContext* FocusContext = FocusViewportWindow->GetViewportContext())
		{
			if (FViewport* FocusViewport = FocusContext->GetViewport())
			{
				FocusViewport->SetFocused(true);
			}
		}
	}
}

void FWindowManager::SetActiveViewportWindow(SViewportWindow* NewActiveViewportWindow)
{
	if (ActiveViewportWindow == NewActiveViewportWindow)
	{
		return;
	}

	if (ActiveViewportWindow)
	{
		if (FViewportContext* PreviousContext = ActiveViewportWindow->GetViewportContext())
		{
			PreviousContext->SetActive(false);
		}
	}

	ActiveViewportWindow = NewActiveViewportWindow;
	if (ActiveViewportWindow)
	{
		if (FViewportContext* NewContext = ActiveViewportWindow->GetViewportContext())
		{
			NewContext->SetActive(true);
		}
	}
}

bool FWindowManager::HasAnyMouseButtonPressed() const
{
	return
		InputManager &&
		(
			InputManager->IsMouseButtonDown(FInputManager::MOUSE_LEFT) ||
			InputManager->IsMouseButtonDown(FInputManager::MOUSE_RIGHT) ||
			InputManager->IsMouseButtonDown(FInputManager::MOUSE_MIDDLE)
		);
}

bool FWindowManager::IsInWindowSubtree(SWindow* Window, SWindow* CandidateAncestor) const
{
	SWindow* Current = Window;
	while (Current)
	{
		if (Current == CandidateAncestor)
		{
			return true;
		}

		Current = Current->GetParent();
	}

	return false;
}

void FWindowManager::FlushPendingDestroyWindows()
{
	for (SWindow* WindowToDestroy : PendingDestroyWindows)
	{
		if (!WindowToDestroy)
		{
			continue;
		}

		if (IsInWindowSubtree(HoveredWindow, WindowToDestroy))
		{
			HoveredWindow = nullptr;
		}
		if (IsInWindowSubtree(PressedWindow, WindowToDestroy))
		{
			PressedWindow = nullptr;
		}
		if (IsInWindowSubtree(MouseCaptureWindow, WindowToDestroy))
		{
			MouseCaptureWindow = nullptr;
		}
		if (IsInWindowSubtree(KeyboardFocusWindow, WindowToDestroy))
		{
			KeyboardFocusWindow = nullptr;
		}
		if (IsInWindowSubtree(ActiveViewportWindow, WindowToDestroy))
		{
			ActiveViewportWindow = nullptr;
		}

		delete WindowToDestroy;
	}

	PendingDestroyWindows.clear();
}

SWindow* FWindowManager::GetWindowAtPoint(const FPoint& Point) const
{
	for (SWindow* Window : Windows)
	{
		if (Window && Window->ISHover(Point))
		{
			return Window->GetWindow(Point);
		}
	}
	return nullptr;
}

bool FWindowManager::RouteMouseMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	FPoint ClientPoint;
	if (!TryGetClientMousePoint(Hwnd, Msg, LParam, ClientPoint))
	{
		return false;
	}

	const LPARAM RoutedLParam = MAKELPARAM(
		static_cast<short>(ClientPoint.X),
		static_cast<short>(ClientPoint.Y));
	SWindow* HitWindow = GetWindowAtPoint(ClientPoint);
	SetHoveredWindow(HitWindow);

	if (IsMouseButtonDownMessage(Msg))
	{
		if (!HitWindow)
		{
			return false;
		}

		SetPressedWindow(HitWindow);
		SetKeyboardFocusWindow(HitWindow);
		SetMouseCaptureWindow(Hwnd, HitWindow);
		if (SViewportWindow* TargetViewportWindow = FindViewportWindow(HitWindow))
		{
			SetActiveViewportWindow(TargetViewportWindow);
		}

		return HitWindow->HandleMessage(Core, Hwnd, Msg, WParam, RoutedLParam);
	}

	if (IsMouseButtonUpMessage(Msg))
	{
		SWindow* TargetWindow = MouseCaptureWindow ? MouseCaptureWindow : HitWindow;
		if (!TargetWindow)
		{
			return false;
		}

		const bool bHandled = TargetWindow->HandleMessage(Core, Hwnd, Msg, WParam, RoutedLParam);
		SetPressedWindow(nullptr);
		if (!HasAnyMouseButtonPressed())
		{
			ReleaseMouseCapture(Hwnd);
		}
		return bHandled;
	}

	if (IsMouseWheelMessage(Msg))
	{
		return HitWindow ? HitWindow->HandleMessage(Core, Hwnd, Msg, WParam, RoutedLParam) : false;
	}

	SWindow* TargetWindow = MouseCaptureWindow ? MouseCaptureWindow : HitWindow;
	return TargetWindow ? TargetWindow->HandleMessage(Core, Hwnd, Msg, WParam, RoutedLParam) : false;
}

bool FWindowManager::RouteKeyboardMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	return KeyboardFocusWindow ? KeyboardFocusWindow->HandleMessage(Core, Hwnd, Msg, WParam, LParam) : false;
}

bool FWindowManager::HandleMessage(FCore* Core, HWND Hwnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
	if (!Core)
	{
		return false;
	}

	if (IsMouseMessage(Msg))
	{
		return RouteMouseMessage(Core, Hwnd, Msg, WParam, LParam);
	}

	if (IsKeyboardMessage(Msg))
	{
		return RouteKeyboardMessage(Core, Hwnd, Msg, WParam, LParam);
	}

	return false;
}

void FWindowManager::Tick(float DeltaTime)
{
	for (SWindow* Window : Windows)
	{
		if (Window)
		{
			Window->Tick(DeltaTime);
		}
	}
}

void FWindowManager::RenderWindows() const
{
	for (SWindow* Window : Windows)
	{
		if (Window)
		{
			Window->Render();
		}
	}
}

void FWindowManager::DrawWindows() const
{
	for (SWindow* Window : Windows)
	{
		if (Window)
		{
			Window->Draw();
		}
	}

	const_cast<FWindowManager*>(this)->FlushPendingDestroyWindows();
}

void FWindowManager::AddWindow(SWindow* NewWindow)
{
	if (!NewWindow)
	{
		return;
	}

	Windows.push_back(NewWindow);
}

void FWindowManager::SetRootWindow(SWindow* NewRootWindow)
{
	ResetWindowTree();
	if (NewRootWindow)
	{
		NewRootWindow->SetParent(nullptr);
		Windows.push_back(NewRootWindow);
	}
}

void FWindowManager::ReplaceWindow(SWindow* OldWindow, SWindow* NewWindow)
{
	if (OldWindow == nullptr || OldWindow == NewWindow)
	{
		return;
	}

	for (SWindow*& Window : Windows)
	{
		if (Window == OldWindow)
		{
			Window = NewWindow;
		}
	}
}

void FWindowManager::QueueDestroyWindow(SWindow* Window)
{
	if (!Window)
	{
		return;
	}

	PendingDestroyWindows.push_back(Window);
}

bool FWindowManager::SaveLayoutToIni(const std::wstring& IniPath) const
{
	if (Windows.empty() || Windows[0] == nullptr)
	{
		return false;
	}

	FLayoutSerializer Serializer{ *this, IniPath };
	const std::wstring RootNodeId = Serializer.SerializeNode(Windows[0]);
	if (RootNodeId.empty())
	{
		return false;
	}

	WriteIniString(IniPath, LayoutRootSection, LayoutRootKey, RootNodeId);
	return true;
}

bool FWindowManager::LoadLayoutFromIni(const std::wstring& IniPath, const FRect& RootRect)
{
	if (!ViewportContextFactory)
	{
		return false;
	}

	const std::wstring RootNodeId = ReadIniString(IniPath, LayoutRootSection, LayoutRootKey);
	if (RootNodeId.empty())
	{
		return false;
	}

	FLayoutDeserializer Deserializer{ *this, IniPath };
	SWindow* NewRootWindow = Deserializer.DeserializeNode(RootNodeId, RootRect);
	if (NewRootWindow == nullptr)
	{
		return false;
	}

	SetRootWindow(NewRootWindow);
	NewRootWindow->SetRect(RootRect);
	return true;
}

void FWindowManager::CollectViewportWindowsRecursive(SWindow* Window, TArray<SViewportWindow*>& OutViewportWindows) const
{
	if (Window == nullptr)
	{
		return;
	}

	if (SViewportWindow* ViewportWindow = dynamic_cast<SViewportWindow*>(Window))
	{
		OutViewportWindows.push_back(ViewportWindow);
		return;
	}

	if (SSplitter* Splitter = dynamic_cast<SSplitter*>(Window))
	{
		CollectViewportWindowsRecursive(Splitter->GetSideLT(), OutViewportWindows);
		CollectViewportWindowsRecursive(Splitter->GetSideRB(), OutViewportWindows);
		return;
	}

	if (SSplitterC* SplitterC = dynamic_cast<SSplitterC*>(Window))
	{
		CollectViewportWindowsRecursive(SplitterC->GetSideLT(), OutViewportWindows);
		CollectViewportWindowsRecursive(SplitterC->GetSideLB(), OutViewportWindows);
		CollectViewportWindowsRecursive(SplitterC->GetSideRT(), OutViewportWindows);
		CollectViewportWindowsRecursive(SplitterC->GetSideRB(), OutViewportWindows);
	}
}

FEditorViewportClient* FWindowManager::FindPerspectiveViewportClient() const
{
	TArray<SViewportWindow*> ViewportWindows;
	for (SWindow* RootWindow : Windows)
	{
		CollectViewportWindowsRecursive(RootWindow, ViewportWindows);
	}

	for (SViewportWindow* ViewportWindow : ViewportWindows)
	{
		if (ViewportWindow == nullptr)
		{
			continue;
		}

		FViewportContext* Context = ViewportWindow->GetViewportContext();
		FEditorViewportClient* EditorViewportClient = dynamic_cast<FEditorViewportClient*>(Context ? Context->GetViewportClient() : nullptr);
		if (EditorViewportClient && EditorViewportClient->GetViewportType() == EEditorViewportType::Perspective)
		{
			return EditorViewportClient;
		}
	}

	return nullptr;
}
