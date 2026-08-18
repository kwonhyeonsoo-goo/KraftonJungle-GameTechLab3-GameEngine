#pragma once
#include "../Foundation/Core/CoreTypes.h"

class ConsoleService
{
public:
    ConsoleService();

    void AddLog(const FString& message);
    void Clear();

    const TArray<FString>& GetLogs()  const;
    int32 GetCount() const;
private:
    TArray<FString> Logs;
};

