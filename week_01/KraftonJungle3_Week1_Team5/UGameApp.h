#pragma once
#include "UWindow.h"

class UScene;
class UGameObject;
class URenderer;

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
	void Initialize();
	void Tick(float dt);
	void Shutdown();
	void OnDestroy() override;
	LRESULT OnMessage(UINT msg, WPARAM wp, LPARAM lp) override;

private:
	void CreateRenderer();
	void EditorUpdate(float dt);

	void DrawSceneManagerPanel();

private:
	URenderer* Renderer;
	UScene* CurrentScene;
};
