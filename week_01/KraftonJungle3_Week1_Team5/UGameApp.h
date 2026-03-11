#pragma once
#include "UEngine.h"
#include "UWindow.h"

class UScene;

class UGameApp : public UWindow
{
public:
	UGameApp(HINSTANCE hInst, FWindowDesc desc);
	~UGameApp() override;

	UGameApp(const UGameApp&) = delete;
	UGameApp& operator=(const UGameApp&) = delete;
	UGameApp(const UGameApp&&) = delete;
	UGameApp& operator=(const UGameApp&&) = delete;

	int Run(int nShowCmd);

protected:
	bool Initialize();
	void Tick(float dt);
	void Shutdown();
	void OnDestroy() override;
	LRESULT OnMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
	void EditorUpdate(float dt);

	void DrawSceneManagerPanel();
	void DrawSceneObjectsPanel();

	// 임시
	void DrawCurrentScenePanel();

private:
	UEngine* Engine;
};
