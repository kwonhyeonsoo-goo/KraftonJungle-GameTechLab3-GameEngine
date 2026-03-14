#include "ConsoleService.h"
#include "Engine/Foundation/Core/log.h"

ConsoleService::ConsoleService()
{
	EngineLog::SetOutputCallback([this](const FString& msg) {
		AddLog(msg);
		});
}

void ConsoleService::AddLog(const FString& message)
{
	Logs.push_back(message);
}

void ConsoleService::Clear()
{
	Logs.clear();
}

const TArray<FString>& ConsoleService::GetLogs() const
{
	// TODO: 여기에 return 문을 삽입합니다.
	return Logs;
}

int32 ConsoleService::GetCount() const
{
	return Logs.size();
}
