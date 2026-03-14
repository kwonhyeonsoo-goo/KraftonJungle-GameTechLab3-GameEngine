#include "log.h"
#include "CoreTypes.h"
#include <cstdarg>

void EngineLog::Print(const char* fmt, ...)
{
	char buffer[1024] = {};
	va_list args;
	va_start(args, fmt);
	std::vsnprintf(buffer, sizeof(buffer), fmt, args);
	va_end(args);
	std::printf("%s\n", buffer);
	if (GOutputCallback)
	{
		GOutputCallback(FString(buffer));
	}
}

void EngineLog::SetOutputCallback(std::function<void(const FString&)> cv)
{
	GOutputCallback = std::move(cv);
}
