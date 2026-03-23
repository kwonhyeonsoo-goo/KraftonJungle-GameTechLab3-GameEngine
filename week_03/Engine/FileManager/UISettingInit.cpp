#include "UISettingInit.h"

FString FUISettingInitializer::ViewportSavePath = "editor.ini";
float FUISettingInitializer::ViewCamMoveSpeed = 10.f;
float FUISettingInitializer::ViewCamRotSpeed = 0.15;
uint32 FUISettingInitializer::GridSize = 5.f;
IniFile FUISettingInitializer::ViewportDump;

void FUISettingInitializer::SaveEditorViewSettings() {
	ViewportDump.Set<float>("Editor", "CameraMoveSpeed", ViewCamMoveSpeed);
	ViewportDump.Set<float>("Editor", "CameraRotSpeed", ViewCamRotSpeed);
	ViewportDump.Set<uint32>("Editor", "GridSize", GridSize);

	ViewportDump.Save(ViewportSavePath);
}

void FUISettingInitializer::LoadEditorViewSettings() {
	ViewportDump.Load(ViewportSavePath);
	ViewCamMoveSpeed = ViewportDump.GetFloat("Editor", "CameraMoveSpeed", 10.f);
	ViewCamRotSpeed = ViewportDump.GetFloat("Editor", "CameraRotSpeed", 10.f);
	GridSize = ViewportDump.GetFloat("Editor", "GridSize", 5.f);
}