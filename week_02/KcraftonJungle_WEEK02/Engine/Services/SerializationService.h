#pragma once

#pragma region Types

#include <cstdint>
#include <string>
#include <vector>

typedef std::string FString;

typedef int int32;
typedef unsigned int uint32;
typedef int64_t int64;
typedef uint64_t uint64;

#pragma endregion

struct AppContext;

class SerializationService
{
public:
	static bool Save(const AppContext& ctx);
	static bool Load(AppContext& ctx);
};

