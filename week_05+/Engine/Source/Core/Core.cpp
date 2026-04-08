#include "Core.h"

#include "FEngine.h"
#include "Actor/Actor.h"
#include "Actor/SkySphereActor.h"
#include "Component/PrimitiveComponent.h"
#include "Component/SubUVComponent.h"
#include "Component/UUIDBillboardComponent.h"
#include "Core/ConsoleVariableManager.h"
#include "Core/Paths.h"
#include "Input/EnhancedInputManager.h"
#include "Input/InputManager.h"
#include "Math/Frustum.h"
#include "Memory/MemoryBase.h"
#include "Mesh/ObjManager.h"
#include "Object/Object.h"
#include "Object/ObjectFactory.h"
#include "Object/ObjectGlobals.h"
#include "Primitive/PrimitiveBase.h"
#include "Renderer/MaterialManager.h"
#include "Renderer/RenderCommand.h"
#include "Renderer/Renderer.h"
#include "ViewportClient.h"
#include "World/Level.h"

#include <filesystem>
#include <cstdio>
#include <algorithm>
#include <cctype>
#include "Asset/AssetRegistry.h"
#include "Asset/AssetManager.h"
#include <psapi.h>

#pragma comment(lib, "Psapi.lib")

namespace //양쪽 공백 제거, 소문자 변환, OverlayMode, BytesToKiB
{
	struct FProcessMemorySnapshot
	{
		bool bIsValid = false;
		SIZE_T WorkingSetBytes = 0;
		SIZE_T PeakWorkingSetBytes = 0;
		SIZE_T CommitBytes = 0;
		SIZE_T PeakCommitBytes = 0;
		SIZE_T PrivateBytes = 0;
	};

	struct FSystemMemorySnapshot
	{
		bool bIsValid = false;
		DWORD MemoryLoadPercent = 0;
		uint64 TotalPhysicalBytes = 0;
		uint64 AvailablePhysicalBytes = 0;
		uint64 TotalVirtualBytes = 0;
		uint64 AvailableVirtualBytes = 0;
	};

	FString TrimConsoleArg(const FString& InValue)
	{
		const size_t Start = InValue.find_first_not_of(" \t");
		if (Start == FString::npos)
		{
			return "";
		}

		const size_t End = InValue.find_last_not_of(" \t");
		return InValue.substr(Start, End - Start + 1);
	}

	FString ToLowerCopy(const FString& InValue)
	{
		FString Result = InValue;
		std::transform(Result.begin(), Result.end(), Result.begin(),
			[](unsigned char C) { return static_cast<char>(std::tolower(C)); });
		return Result;
	}

	uint8 GetStatOverlayModeFlag(FCore::EStatOverlayMode InMode)
	{
		return static_cast<uint8>(InMode);
	}

	bool HasStatOverlayMode(uint8 Flags, FCore::EStatOverlayMode InMode)
	{
		return (Flags & GetStatOverlayModeFlag(InMode)) != 0;
	}

	FString GetStatModeName(uint8 Flags)
	{
		if (Flags == static_cast<uint8>(FCore::EStatOverlayMode::None))
		{
			return "none";
		}

		FString Result;
		if (HasStatOverlayMode(Flags, FCore::EStatOverlayMode::FPS))
		{
			Result += "fps";
		}
		if (HasStatOverlayMode(Flags, FCore::EStatOverlayMode::Memory))
		{
			if (!Result.empty())
			{
				Result += " ";
			}
			Result += "memory";
		}

		return Result.empty() ? "none" : Result;
	}

	float BytesToKiB(uint32 Bytes)
	{
		return static_cast<float>(Bytes) / 1024.0f;
	}

	double BytesToMiB(uint64 Bytes)
	{
		return static_cast<double>(Bytes) / (1024.0 * 1024.0);
	}

	double BytesToGiB(uint64 Bytes)
	{
		return static_cast<double>(Bytes) / (1024.0 * 1024.0 * 1024.0);
	}

	FProcessMemorySnapshot CaptureProcessMemorySnapshot()
	{
		FProcessMemorySnapshot Snapshot;

		PROCESS_MEMORY_COUNTERS_EX Counters = {};
		Counters.cb = sizeof(Counters);
		if (GetProcessMemoryInfo(GetCurrentProcess(), reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&Counters), sizeof(Counters)))
		{
			Snapshot.bIsValid = true;
			Snapshot.WorkingSetBytes = Counters.WorkingSetSize;
			Snapshot.PeakWorkingSetBytes = Counters.PeakWorkingSetSize;
			Snapshot.CommitBytes = Counters.PagefileUsage;
			Snapshot.PeakCommitBytes = Counters.PeakPagefileUsage;
			Snapshot.PrivateBytes = Counters.PrivateUsage;
		}

		return Snapshot;
	}

	FSystemMemorySnapshot CaptureSystemMemorySnapshot()
	{
		FSystemMemorySnapshot Snapshot;

		MEMORYSTATUSEX Status = {};
		Status.dwLength = sizeof(Status);
		if (GlobalMemoryStatusEx(&Status))
		{
			Snapshot.bIsValid = true;
			Snapshot.MemoryLoadPercent = Status.dwMemoryLoad;
			Snapshot.TotalPhysicalBytes = Status.ullTotalPhys;
			Snapshot.AvailablePhysicalBytes = Status.ullAvailPhys;
			Snapshot.TotalVirtualBytes = Status.ullTotalVirtual;
			Snapshot.AvailableVirtualBytes = Status.ullAvailVirtual;
		}

		return Snapshot;
	}
}

FCore::~FCore()
{
	Release();
}

bool FCore::Initialize(HWND Hwnd, int32 Width, int32 Height, EWorldType StartupLevelType)
{
	FPaths::Initialize();
	WindowWidth = Width;
	WindowHeight = Height;

	GRenderer = new FRenderer(Hwnd, Width, Height);
	if (!GRenderer)
	{
		return false;
	}

	FString AssetRootDir = (FPaths::ProjectRoot() / "Assets").string();
	FAssetRegistry::Get().SearchAllAssets(AssetRootDir);
	// Material
	FMaterialManager::Get().LoadAllMaterials(GRenderer->GetDevice(), GRenderer->GetRenderStateManager().get());

	RegisterConsoleVariables();

	return true;
}

void FCore::Release()
{
	CPrimitiveBase::ClearCache();//shared_ptr<FMeshData> 해제
	 
	if (GRenderer)
	{
		delete GRenderer;
		GRenderer = nullptr;
	}
}

void FCore::OnResize(int32 Width, int32 Height)
{
	if (Width == 0 || Height == 0)
	{
		return;
	}

	WindowWidth = Width;
	WindowHeight = Height;

	if (GRenderer)
	{
		GRenderer->OnResize(Width, Height);
	}
}

void FCore::RegisterConsoleVariables()
{
	FConsoleVariableManager& CVM = FConsoleVariableManager::Get();

	FConsoleVariable* VSyncVar = CVM.Register("r.VSync", 0, "Enable VSync (0 = off, 1 = on)");
	if (GRenderer)
	{
		GRenderer->SetVSync(false);
	}


	VSyncVar->SetOnChanged([this](FConsoleVariable* Var)
	{
		if (GRenderer)
		{
			// 성능 테스트를 위해 강제로 false 고정 (사용자 입력을 무시하려면 false)
			GRenderer->SetVSync(false);
		}
	});


	StatOverlayModeFlags |= GetStatOverlayModeFlag(EStatOverlayMode::FPS);

	
	CVM.RegisterCommand("stat", [this](const FString& InArgs, FString& OutResult)
	
		{
			const FString Args = ToLowerCopy(TrimConsoleArg(InArgs));
			if (Args.empty())
			{
				OutResult = FString("stat = ") + GetStatModeName(StatOverlayModeFlags) + "  (usage: stat fps | stat memory | stat none)";
				return;
			}

			if (Args == "fps")
			{
				StatOverlayModeFlags ^= GetStatOverlayModeFlag(EStatOverlayMode::FPS);
			}
			else if (Args == "memory")
			{
				StatOverlayModeFlags ^= GetStatOverlayModeFlag(EStatOverlayMode::Memory);
			}
			else if (Args == "none")
			{
				StatOverlayModeFlags = static_cast<uint8>(EStatOverlayMode::None);
			}
			else
			{
				OutResult = "usage: stat fps | stat memory | stat none";
				return;
			}

			OutResult = FString("stat = ") + GetStatModeName(StatOverlayModeFlags);
		}, "Show overlay stats: stat fps | stat memory | stat none");
}

/**
 * StatOverlay를 렌더링합니다. StatOverlayMode에 따라 FPS 또는 Memory 정보를 화면에 표시합니다.
 * 
 * \param Renderer
 * \param ViewportWidth
 * \param ViewportHeight
 */
void FCore::RenderStatOverlay(FRenderer* Renderer, int32 ViewportWidth, int32 ViewportHeight) const
{
	if (!Renderer || StatOverlayModeFlags == static_cast<uint8>(EStatOverlayMode::None) || ViewportWidth <= 0 || ViewportHeight <= 0)
	{
		return;
	}

	bool bIsFirst = true;

	constexpr float Margin = 30.0f;
	constexpr float Padding = 5.0f;
	constexpr float LineSpacing = 3.0f;
	const FVector4 BackgroundColor(0.05f, 0.05f, 0.05f, 0.80f);
	const FVector4 BorderColor(1.0f, 1.0f, 1.0f, 0.10f);
	const FVector4 TitleColor(0.95f, 0.95f, 0.95f, 1.0f);
	const FVector4 ValueColor(0.80f, 0.90f, 1.0f, 1.0f);

	const float LineHeight = Renderer->GetOverlayTextLineHeight();
	float LeftCursorBoxY = Margin;

	auto DrawStatBox = [&](const TArray<FString>& Lines, float BoxWidth, float BoxX, float& CursorBoxY)
		{
			if (Lines.empty())
			{
				return;
			}
			const float BoxHeight = Padding * 2.0f + LineHeight * static_cast<float>(Lines.size()) + LineSpacing * static_cast<float>(Lines.size() - 1);
			Renderer->DrawOverlayRect(BoxX, CursorBoxY, BoxWidth, BoxHeight, BackgroundColor, ViewportWidth, ViewportHeight);
			bIsFirst = false; // 이후 모든 드로우 콜은 행렬 업데이트 생략


			Renderer->DrawOverlayRectOutline(BoxX, CursorBoxY, BoxWidth, BoxHeight, BorderColor, ViewportWidth, ViewportHeight);
			float CursorY = CursorBoxY + Padding;
			for (size_t Index = 0; Index < Lines.size(); ++Index)
			{
				const FVector4& TextColor = (Index == 0) ? TitleColor : ValueColor;
				Renderer->DrawOverlayText(Lines[Index], BoxX + Padding, CursorY, TextColor, ViewportWidth, ViewportHeight);
				CursorY += LineHeight + LineSpacing;
			}

			CursorBoxY += BoxHeight + Margin;
		};

	if (HasStatOverlayMode(StatOverlayModeFlags, EStatOverlayMode::FPS))
	{
		TArray<FString> Lines;
		char Buffer[128] = {};
		Lines.push_back("STAT FPS");
		snprintf(Buffer, sizeof(Buffer), "%.2f FPS", GEngine->GetTimer()->GetDisplayFPS());
		Lines.push_back(Buffer);
		snprintf(Buffer, sizeof(Buffer), "%.2f ms", GEngine->GetTimer()->GetFrameTimeMs());
		Lines.push_back(Buffer);

		const float FPSBoxWidth = 320.0f;
		const float FPSBoxX = static_cast<float>(ViewportWidth) - Margin - FPSBoxWidth;
		float FPSCursorBoxY = Margin;
		DrawStatBox(Lines, FPSBoxWidth, FPSBoxX, FPSCursorBoxY);
	}

	if (HasStatOverlayMode(StatOverlayModeFlags, EStatOverlayMode::Memory))
	{
		const FMallocStats& MallocStats = GetGMalloc()->MallocStats;
		const FProcessMemorySnapshot ProcessMemory = CaptureProcessMemorySnapshot();
		TArray<FString> Lines;
		char Buffer[192] = {};
		Lines.push_back("STAT MEMORY");

		if (ProcessMemory.bIsValid)
		{
			snprintf(Buffer, sizeof(Buffer), "Process Working Set-: %.2f MiB", BytesToMiB(ProcessMemory.WorkingSetBytes));
			Lines.push_back(Buffer);
			snprintf(Buffer, sizeof(Buffer), "Process Peak WS-----: %.2f MiB", BytesToMiB(ProcessMemory.PeakWorkingSetBytes));
			Lines.push_back(Buffer);
			snprintf(Buffer, sizeof(Buffer), "Process Commit -----: %.2f MiB", BytesToMiB(ProcessMemory.CommitBytes));
			Lines.push_back(Buffer);
			snprintf(Buffer, sizeof(Buffer), "Process Peak Commit-: %.2f MiB", BytesToMiB(ProcessMemory.PeakCommitBytes));
			Lines.push_back(Buffer);
			snprintf(Buffer, sizeof(Buffer), "Process Private ----: %.2f MiB", BytesToMiB(ProcessMemory.PrivateBytes));
			Lines.push_back(Buffer);
		}
		snprintf(Buffer, sizeof(Buffer), "Heap Usage ---------: %.2f KiB", BytesToKiB(MallocStats.CurrentAllocationBytes));
		Lines.push_back(Buffer);
		snprintf(Buffer, sizeof(Buffer), "Heap Allocations ---: %u", MallocStats.CurrentAllocationCount);
		Lines.push_back(Buffer);
		DrawStatBox(Lines, 560.0f, Margin, LeftCursorBoxY);
	}
}