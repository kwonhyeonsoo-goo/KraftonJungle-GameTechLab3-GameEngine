#include "ContentBrowserWindow.h"
#include <filesystem>
#include <d3d11.h>
#include <algorithm>
#include "Debug/EngineLog.h"
#include "Core/Paths.h"

FContentBrowserWindow::FContentBrowserWindow() :
	RootPath(std::filesystem::current_path()),
	CurrentPath(std::filesystem::current_path())
{
	while (!std::filesystem::exists(RootPath / "Engine"))
	{
		RootPath = RootPath.parent_path();
	}

	RootPath = RootPath / "Assets";
	CurrentPath = RootPath;
}

void FContentBrowserWindow::Render()
{
	if (!ImGui::Begin("Content Browser"))
	{
		ImGui::End();
		return;
	}

	bIsMouseOnDirectory = false;
	bIsMouseOnFile = false;
	DirectoryPathUnderMouse = "";
	FilePathUnderMouse = "";

	// 상단 경로 + 뒤로가기
	if (ImGui::Button("<-"))
	{
		if (CurrentPath != RootPath)
		{
			CurrentPath = CurrentPath.parent_path();
		}
	}

	ImGui::SameLine();
	ImGui::Text("%s", CurrentPath.u8string().c_str());

	ImGui::Separator();

	// 좌측 폴더 트리
	ImGui::BeginChild("LeftPanel", ImVec2(200, 0), true);
	DrawFolderTree(RootPath);
	ImGui::EndChild();

	ImGui::SameLine();

	// 우측 파일 그리드
	ImGui::BeginChild("RightPanel", ImVec2(0, 0), true);
	DrawFileGrid();
	ImGui::EndChild();

	bIsHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

	if (bFileOnDrag && ImGui::IsMouseReleased(ImGuiMouseButton_Left))
	{
		if (OnFileDragEnd && !SelectedFilePath.empty())
		{
			OnFileDragEnd(SelectedFilePath.string(), DirectoryPathUnderMouse.string());
		}

		bFileOnDrag = false;
		SelectedFilePath.clear();
	}

	ImGui::End();
	// 예전 수동 Drag & Drop 데드 코드 완벽히 삭제 완료!
}

void FContentBrowserWindow::SetFolderIcon(ID3D11ShaderResourceView* FolderSRV)
{
	FolderIcon = (ImTextureID)(FolderSRV);
}

void FContentBrowserWindow::SetFileIcon(ID3D11ShaderResourceView* FileSRV)
{
	FileIcon = (ImTextureID)(FileSRV);
}

void FContentBrowserWindow::DrawFolderTree(const std::filesystem::path& Path)
{
	for (auto& Entry : std::filesystem::directory_iterator(Path))
	{
		if (!Entry.is_directory())
			continue;

		const auto& DirPath = Entry.path();
		auto NameUtf8 = DirPath.filename().u8string();
		std::string Name(NameUtf8.begin(), NameUtf8.end());

		ImGuiTreeNodeFlags Flags = ImGuiTreeNodeFlags_OpenOnArrow |
			((CurrentPath == DirPath) ? ImGuiTreeNodeFlags_Selected : 0);

		bool bOpened = ImGui::TreeNodeEx(Name.c_str(), Flags);

		if (ImGui::IsItemClicked())
		{
			CurrentPath = DirPath;
		}

		if (bOpened)
		{
			DrawFolderTree(DirPath);
			ImGui::TreePop();
		}

		if (ImGui::IsItemHovered())
		{
			if (Entry.is_directory())
			{
				bIsMouseOnDirectory = true;
				DirectoryPathUnderMouse = DirPath;
			}
			else if (Entry.is_regular_file())
			{
				bIsMouseOnFile = true;
				FilePathUnderMouse = DirPath;
			}
		}
	}
}

void FContentBrowserWindow::DrawFileGrid()
{
	const float CellSize = 80.0f;
	float PanelWidth = ImGui::GetContentRegionAvail().x;

	int ColumnCount = (int)(PanelWidth / CellSize);
	if (ColumnCount < 1) ColumnCount = 1;

	ImGui::Columns(ColumnCount, 0, false);

	for (auto& Entry : std::filesystem::directory_iterator(CurrentPath))
	{
		DrawGridItem(Entry); // 그리드 아이템 그리기 로직을 분리!
	}

	ImGui::Columns(1);
}

// ─── 리팩토링: 개별 아이템 렌더링 로직 분리 ────────────────────────
void FContentBrowserWindow::DrawGridItem(const std::filesystem::directory_entry& Entry)
{
	const auto& Path = Entry.path();
	auto NameUtf8 = Path.filename().u8string();
	std::string Name(NameUtf8.begin(), NameUtf8.end());

	// 1. 파일 확장자 필터링 (텍스처 포맷 추가!)
	if (Entry.is_regular_file())
	{
		std::string Ext = Path.extension().string();
		std::ranges::transform(Ext, Ext.begin(), [](unsigned char c) { return std::tolower(c); });

		if (Ext != ".json" && Ext != ".dasset" && Ext != ".png" && Ext != ".jpg" && Ext != ".jpeg")
		{
			return; // 지원하지 않는 포맷은 건너뜀
		}
	}

	// 2. 고유 ID 푸시
	ImGui::PushID(Name.c_str());

	const float IconSize = 48.0f;
	ImTextureID Icon = Entry.is_directory() ? FolderIcon : FileIcon;

	// 3. 아이콘 버튼
	ImGui::ImageButton(Name.c_str(), Icon, ImVec2(IconSize, IconSize));

	// 4. Drag & Drop Source 및 Context Menu
	if (!Entry.is_directory())
	{
		if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID))
		{
			std::string PayloadPath = Path.string();
			std::replace(PayloadPath.begin(), PayloadPath.end(), '\\', '/');
			ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", PayloadPath.c_str(), PayloadPath.size() + 1);
			bFileOnDrag = true;
			SelectedFilePath = Path;

			ImGui::Image(FileIcon, ImVec2(32, 32));
			ImGui::SameLine();
			ImGui::Text("%s", Name.c_str());

			ImGui::EndDragDropSource();
		}
	}
	else
	{
		if (ImGui::BeginPopupContextItem())
		{
			if (ImGui::MenuItem("Delete"))
			{
				std::filesystem::remove(Path);
			}
			ImGui::EndPopup();
		}
	}

	// 5. 선택 처리
	if (ImGui::IsItemClicked())
	{
		SelectedPath = Path;
	}

	// 6. 호버 및 더블 클릭 처리
	if (ImGui::IsItemHovered())
	{
		if (Entry.is_directory())
		{
			bIsMouseOnDirectory = true;
			DirectoryPathUnderMouse = Path;
		}
		else if (Entry.is_regular_file())
		{
			bIsMouseOnFile = true;
			FilePathUnderMouse = Path;
		}

		if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
		{
			if (Entry.is_directory())
			{
				CurrentPath /= Path.filename(); // 폴더 진입
			}
			else
			{
				OnFileDoubleClickCallback(Path.string());
			}
		}
	}

	// 7. 이름 출력
	ImGui::TextWrapped("%s", Name.c_str());

	ImGui::NextColumn();
	ImGui::PopID();
}
