#include "FWindowsIO.h"
#include "Core/Paths.h"
#include <filesystem>

FWindowsBinWriter::FWindowsBinWriter(const FString& Path)
{
	FString AbsolutePath = FPaths::ToAbsolutePath(Path);
	std::filesystem::path path = FPaths::ToU8String(AbsolutePath);
	File.open(path, std::ios::binary | std::ios::out);
}

void FWindowsBinWriter::WriteString(const FString& Str)
{
	uint32_t Len = Str.size();
	Write(Len);
	File.write(Str.data(), Len);
}

void FWindowsBinWriter::WriteStringArray(const TArray<FString>& Arr)
{
	uint32_t Count = Arr.size();
	Write(Count);
	for (const FString& S : Arr)
	{
		WriteString(S);
	}
}

FWindowsBinReader::FWindowsBinReader(const FString& Path)
{
	FString AbsolutePath = FPaths::ToAbsolutePath(Path);
	std::filesystem::path path = FPaths::ToU8String(AbsolutePath);
	File.open(path, std::ios::binary | std::ios::in);
}

void FWindowsBinReader::ReadString(FString& OutString)
{
	uint32_t Len;
	Read(Len);
	OutString.resize(Len);
	File.read(OutString.data(), Len);
}

void FWindowsBinReader::ReadStringArray(TArray<FString>& OutArr)
{
	uint32_t Count;
	Read(Count);
	OutArr.resize(Count);
	for (FString& S : OutArr)
	{
		ReadString(S);
	}
}
