#pragma once


#include "CoreMinimal.h"
#include <fstream>
#include "Types/String.h"
#include "Types/Array.h"
class FWindowsBinWriter
{
private:
	std::ofstream File;
public:
	FWindowsBinWriter(const FString& Path);

	template<typename T>
	void Write(const T& Data)
	{
		File.write(reinterpret_cast<const char*>(&Data), sizeof(T));
	}

	void WriteString(const FString& Str);
	void WriteStringArray(const TArray<FString>& Arr);

	template<typename T>
	void WriteArray(const TArray<T>& Arr)
	{
		uint32_t Count = Arr.size();
		Write(Count);
		File.write(reinterpret_cast<const char*>(Arr.data()), sizeof(T) * Count);
	}
};

class FWindowsBinReader
{
private:
	std::ifstream File;
public:
	FWindowsBinReader(const FString& Path);

	template<typename T>
	void Read(T& OutData)
	{
		File.read(reinterpret_cast<char*>(&OutData), sizeof(T));
	}

	bool IsOpen() const { return File.is_open(); }

	void ReadString(FString& OutString);
	void ReadStringArray(TArray<FString>& OutArr);

	template<typename T>
	void ReadArray(TArray<T>& OutArr)
	{
		uint32_t Count;
		Read(Count);
		OutArr.resize(Count);
		File.read(reinterpret_cast<char*>(OutArr.data()), sizeof(T) * Count);
	}
};