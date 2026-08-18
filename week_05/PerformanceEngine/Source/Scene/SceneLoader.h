#pragma once

#include <string>

class FCore;
class FScene;
class FD3D11RHI;
class FCamera;
class FVisibilitySystem;
class FPickingSystem;
class FSceneLoader
{
public:
	FSceneLoader() = default;
	~FSceneLoader() = default;
	bool OpenSceneFileDialog(std::wstring& OutSelectedPath) const;
	const std::wstring& GetCurrentScenePath() const { return CurrentScenePath; }
	void SetCurrentScenePath(const std::wstring& InPath) { CurrentScenePath = InPath; }
	bool LoadScene(
		const std::wstring& InScenePath,
		FCore* InCore,
		FScene* InOutScene,
		FD3D11RHI* InRHI,
		FCamera* InOutCamera,
		FVisibilitySystem* InVisibilitySystem,
		FPickingSystem* InPickingSystem);
private:
	std::wstring CurrentScenePath;
};