#pragma once
#include <filesystem>
#include <imgui.h>
#include <functional>
#include "Types/String.h"

struct ID3D11ShaderResourceView;

class CContentBrowserWindow
{
public:
	CContentBrowserWindow();

	void Render();

	void SetFolderIcon(ID3D11ShaderResourceView* FolderSRV);
	void SetFileIcon(ID3D11ShaderResourceView* FileSRV);

	std::function<void(const FString& FilePath)> OnFileDoubleClickCallback;

private:
	std::filesystem::path RootPath;
	std::filesystem::path CurrentPath;
	std::filesystem::path SelectedPath;

	/*
		ID3D11ShaderResourceView* FolderSRV;
		ImTextureID FolderIcon = (ImTextureID)FolderSRV;
	*/
	ImTextureID FolderIcon;
	ImTextureID FileIcon;

	void DrawFolderTree(const std::filesystem::path& Path);
	void DrawFileGrid();
};


