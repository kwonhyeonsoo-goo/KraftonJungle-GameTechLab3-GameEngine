#pragma once
#include <algorithm>
#include "CoreTypes.h"
#include <functional>

#define UE_LOG(fmt, ...) EngineLog::Print(fmt, ##__VA_ARGS__)

namespace EngineLog {
	static std::function<void(const FString&)> GOutputCallback = nullptr;

	void Print(const char* fmt, ...);
	void SetOutputCallback(std::function<void(const FString&)> cv);
}