#pragma once

#pragma region Types

#include <cstdint>
#include <string>
#include <vector>

template <typename T>
using TArray = std::vector<T>;

typedef std::string FString;

typedef int int32;
typedef unsigned int uint32;
typedef int64_t int64;
typedef uint64_t uint64;

#pragma endregion

struct AppContext;

class SerializationService
{
	static bool Save(const AppContext& ctx);
	static bool Load(AppContext& ctx);
};

