#include "Stats/StatsSystem.h"

#include <algorithm>
#include <array>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>

#include <Windows.h>

#include "Picking/PickingSystem.h"

namespace
{
	uint64 QueryCycles64()
	{
		LARGE_INTEGER Counter = {};
		QueryPerformanceCounter(&Counter);
		return static_cast<uint64>(Counter.QuadPart);
	}

	double GetSecondsPerCycle()
	{
		static const double SecondsPerCycle = []()
		{
			LARGE_INTEGER Frequency = {};
			QueryPerformanceFrequency(&Frequency);
			return 1.0 / static_cast<double>(Frequency.QuadPart);
		}();
		return SecondsPerCycle;
	}

#if defined(BENCHMARK)
	struct FNumericSummary
	{
		double Average = 0.0;
		double Min = 0.0;
		double Max = 0.0;
	};

	void EmitBenchmarkDebugMessage(std::string_view InMessage)
	{
		OutputDebugStringA(std::string(InMessage).c_str());
	}

	std::filesystem::path GetExecutableDirectory()
	{
		std::array<wchar_t, MAX_PATH> ModulePath = {};
		const DWORD CharacterCount = GetModuleFileNameW(nullptr, ModulePath.data(), static_cast<DWORD>(ModulePath.size()));
		if (CharacterCount == 0)
		{
			return {};
		}

		return std::filesystem::path(ModulePath.data()).parent_path();
	}

	std::filesystem::path GetBenchmarkLogDirectory()
	{
		if (const std::filesystem::path ExecutableDirectory = GetExecutableDirectory(); !ExecutableDirectory.empty())
		{
			return ExecutableDirectory / L"BenchmarkLogs";
		}

		return std::filesystem::current_path() / L"BenchmarkLogs";
	}

	std::string MakeRunTimestamp()
	{
		const std::time_t CurrentTime = std::time(nullptr);
		std::tm LocalTime = {};
		localtime_s(&LocalTime, &CurrentTime);

		std::ostringstream Stream;
		Stream << std::put_time(&LocalTime, "%Y%m%d_%H%M%S");
		return Stream.str();
	}

	template <typename TSample, typename TValueAccessor>
	FNumericSummary ComputeSummary(const std::vector<TSample>& InSamples, TValueAccessor InAccessor)
	{
		if (InSamples.empty())
		{
			return {};
		}

		double Sum = 0.0;
		double Min = std::numeric_limits<double>::max();
		double Max = std::numeric_limits<double>::lowest();
		for (const TSample& Sample : InSamples)
		{
			const double Value = InAccessor(Sample);
			Sum += Value;
			Min = std::min(Min, Value);
			Max = std::max(Max, Value);
		}

		FNumericSummary Summary;
		Summary.Average = Sum / static_cast<double>(InSamples.size());
		Summary.Min = Min;
		Summary.Max = Max;
		return Summary;
	}

	bool WriteFrameBenchmarkCsv(
		const std::filesystem::path& InFilePath,
		const std::vector<FStatsSystem::FFrameBenchmarkSample>& InSamples)
	{
		std::ofstream File(InFilePath, std::ios::out | std::ios::trunc);
		if (!File.is_open())
		{
			return false;
		}

		File << "frame,frame_ms,fps\n";
		File << std::fixed << std::setprecision(6);
		for (const FStatsSystem::FFrameBenchmarkSample& Sample : InSamples)
		{
			File
				<< Sample.Frame << ','
				<< Sample.FrameTimeMs << ','
				<< Sample.FramesPerSecond << '\n';
		}

		return File.good();
	}

	bool WritePickBenchmarkCsv(
		const std::filesystem::path& InFilePath,
		const std::vector<FStatsSystem::FPickBenchmarkSample>& InSamples)
	{
		std::ofstream File(InFilePath, std::ios::out | std::ios::trunc);
		if (!File.is_open())
		{
			return false;
		}

		File << "frame,pick_ms,hit,selected_primitive_id\n";
		File << std::fixed << std::setprecision(6);
		for (const FStatsSystem::FPickBenchmarkSample& Sample : InSamples)
		{
			File
				<< Sample.Frame << ','
				<< Sample.PickTimeMs << ','
				<< (Sample.bHit ? 1 : 0) << ','
				<< Sample.SelectedPrimitiveId << '\n';
		}

		return File.good();
	}

	bool WriteBenchmarkSummary(
		const std::filesystem::path& InFilePath,
		const std::string& InRunTimestamp,
		const FBenchmarkRunMetadata& InMetadata,
		const std::vector<FStatsSystem::FFrameBenchmarkSample>& InFrameSamples,
		const std::vector<FStatsSystem::FPickBenchmarkSample>& InPickSamples,
		double InTotalPickTimeMs,
		uint64 InTotalPickCount)
	{
		std::ofstream File(InFilePath, std::ios::out | std::ios::trunc);
		if (!File.is_open())
		{
			return false;
		}

		const FNumericSummary FrameMsSummary = ComputeSummary(InFrameSamples, [](const FStatsSystem::FFrameBenchmarkSample& Sample)
		{
			return Sample.FrameTimeMs;
		});
		const FNumericSummary FpsSummary = ComputeSummary(InFrameSamples, [](const FStatsSystem::FFrameBenchmarkSample& Sample)
		{
			return Sample.FramesPerSecond;
		});
		const FNumericSummary PickMsSummary = ComputeSummary(InPickSamples, [](const FStatsSystem::FPickBenchmarkSample& Sample)
		{
			return Sample.PickTimeMs;
		});

		double TotalRuntimeMs = 0.0;
		for (const FStatsSystem::FFrameBenchmarkSample& Sample : InFrameSamples)
		{
			TotalRuntimeMs += Sample.FrameTimeMs;
		}

		File << std::fixed << std::setprecision(6);
		File << "RunTimestamp: " << InRunTimestamp << '\n';
		File << "TotalFrames: " << InFrameSamples.size() << '\n';
		File << "TotalRuntimeMs: " << TotalRuntimeMs << '\n';
		File << "AverageFrameMs: " << FrameMsSummary.Average << '\n';
		File << "MinimumFrameMs: " << FrameMsSummary.Min << '\n';
		File << "MaximumFrameMs: " << FrameMsSummary.Max << '\n';
		File << "AverageFps: " << FpsSummary.Average << '\n';
		File << "MinimumFps: " << FpsSummary.Min << '\n';
		File << "MaximumFps: " << FpsSummary.Max << '\n';
		File << "TotalPickCount: " << InTotalPickCount << '\n';
		File << "TotalPickTimeMs: " << InTotalPickTimeMs << '\n';
		File << "AveragePickMs: " << PickMsSummary.Average << '\n';
		File << "MaximumPickMs: " << PickMsSummary.Max << '\n';
		File << "AdapterName: " << InMetadata.AdapterName << '\n';
		File << "DedicatedVideoMemoryMB: " << InMetadata.DedicatedVideoMemoryMB << '\n';
		File << "Resolution: " << InMetadata.ViewportWidth << 'x' << InMetadata.ViewportHeight << '\n';

		return File.good();
	}
#endif
}

void FStatsSystem::Reset()
{
	LastFrameCounter = 0;
	FrameTimeMs = 0.0;
	FramesPerSecond = 0.0;
	LastPickTimeMs = 0.0;
	TotalPickTimeMs = 0.0;
	TotalPickCount = 0;
	FrameNumber = 0;

#if defined(BENCHMARK)
	FrameBenchmarkSamples.clear();
	PickBenchmarkSamples.clear();
#endif
}

void FStatsSystem::BeginFrame()
{
	const uint64 CurrentFrameCounter = QueryCycles64();
	if (LastFrameCounter == 0)
	{
		LastFrameCounter = CurrentFrameCounter;
	}

	const double DeltaSeconds = std::clamp(
		static_cast<double>(CurrentFrameCounter - LastFrameCounter) * GetSecondsPerCycle(),
		0.0,
		0.25);

	LastFrameCounter = CurrentFrameCounter;
	FrameTimeMs = DeltaSeconds * 1000.0;
	FramesPerSecond = DeltaSeconds > 1.e-9 ? (1.0 / DeltaSeconds) : 0.0;
}

void FStatsSystem::EndFrame()
{
	++FrameNumber;

#if defined(BENCHMARK)
	FrameBenchmarkSamples.push_back({ FrameNumber, FrameTimeMs, FramesPerSecond });
#endif
}

void FStatsSystem::ApplyPickState(const FPickState& InPickState)
{
	LastPickTimeMs = InPickState.LastPickTimeMs;
	TotalPickTimeMs = InPickState.TotalPickTimeMs;
	TotalPickCount = InPickState.TotalPickCount;
}

void FStatsSystem::RecordPickEvent(const FPickState& InPickState)
{
#if defined(BENCHMARK)
	PickBenchmarkSamples.push_back({
		FrameNumber + 1,
		InPickState.LastPickTimeMs,
		InPickState.bHit,
		InPickState.SelectedPrimitiveId,
	});
#else
	(void)InPickState;
#endif
}

void FStatsSystem::WriteBenchmarkLogs(const FBenchmarkRunMetadata& InMetadata) const
{
#if defined(BENCHMARK)
	std::error_code ErrorCode;
	const std::filesystem::path LogDirectory = GetBenchmarkLogDirectory();
	std::filesystem::create_directories(LogDirectory, ErrorCode);
	if (ErrorCode)
	{
		std::ostringstream Stream;
		Stream << "[Benchmark] Failed to create log directory: " << LogDirectory.string() << '\n';
		EmitBenchmarkDebugMessage(Stream.str());
		return;
	}

	const std::string RunTimestamp = MakeRunTimestamp();
	const std::filesystem::path FilePrefix = LogDirectory / RunTimestamp;
	const std::filesystem::path FrameCsvPath = FilePrefix;
	const std::filesystem::path PickCsvPath = FilePrefix;
	const std::filesystem::path SummaryPath = FilePrefix;

	const std::filesystem::path FrameFilePath = FrameCsvPath.string() + "_frames.csv";
	const std::filesystem::path PickFilePath = PickCsvPath.string() + "_pick_events.csv";
	const std::filesystem::path SummaryFilePath = SummaryPath.string() + "_summary.txt";

	if (!WriteFrameBenchmarkCsv(FrameFilePath, FrameBenchmarkSamples))
	{
		std::ostringstream Stream;
		Stream << "[Benchmark] Failed to write frame log: " << FrameFilePath.string() << '\n';
		EmitBenchmarkDebugMessage(Stream.str());
	}

	if (!WritePickBenchmarkCsv(PickFilePath, PickBenchmarkSamples))
	{
		std::ostringstream Stream;
		Stream << "[Benchmark] Failed to write pick log: " << PickFilePath.string() << '\n';
		EmitBenchmarkDebugMessage(Stream.str());
	}

	if (!WriteBenchmarkSummary(
		SummaryFilePath,
		RunTimestamp,
		InMetadata,
		FrameBenchmarkSamples,
		PickBenchmarkSamples,
		TotalPickTimeMs,
		TotalPickCount))
	{
		std::ostringstream Stream;
		Stream << "[Benchmark] Failed to write summary log: " << SummaryFilePath.string() << '\n';
		EmitBenchmarkDebugMessage(Stream.str());
	}
#else
	(void)InMetadata;
#endif
}
