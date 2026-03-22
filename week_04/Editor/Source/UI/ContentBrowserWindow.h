#pragma once
#include <filesystem>
#include <imgui.h>

class CContentBrowserWindow
{
public:
	void Render();

private:
	std::filesystem::path RootPath = "C:/jungle-techlab-week3-team5/Assets";
	std::filesystem::path CurrentPath = "C:/jungle-techlab-week3-team5/Assets";

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


