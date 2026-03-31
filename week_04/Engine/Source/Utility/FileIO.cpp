#include "FileIO.h"

std::string GetFilePathInternal(EFileDialogType Type, const char* Filter, const char* DefaultExt)
{
	char FileName[MAX_PATH] = "";
	FString ContentDir = FPaths::ContentDir().string();

	OPENFILENAMEA Ofn = {};
	Ofn.lStructSize = sizeof(OPENFILENAMEA);
	Ofn.lpstrFilter = Filter;
	Ofn.lpstrFile = FileName;
	Ofn.nMaxFile = MAX_PATH;
	Ofn.lpstrDefExt = DefaultExt;
	Ofn.lpstrInitialDir = ContentDir.c_str();

	if (Type == EFileDialogType::Save)
	{
		Ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

		if (GetSaveFileNameA(&Ofn))
			return std::string(FileName);
	}
	else
	{
		Ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

		if (GetOpenFileNameA(&Ofn))
			return std::string(FileName);
	}

	return "";
}

std::string GetJsonFilePath(EFileDialogType Type)
{
	return GetFilePathInternal(
		Type,
		"JSON Files (*.json)\0*.json\0All Files (*.*)\0*.*\0\0",
		"json"
	);
}

std::string GetObjFilePath(EFileDialogType Type)
{
	return GetFilePathInternal(
		Type,
		"OBJ Files (*.obj)\0*.obj\0All Files (*.*)\0*.*\0\0",
		"obj"
	);
}