#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>

class UWindow
{
public:
	struct FWindowDesc
	{
		std::wstring ClassName = L"JungleWindowClass";
		std::wstring Title = L"Game Tech Lab";
		int Width = 1024;
		int Height = 1024;
		DWORD Style = WS_OVERLAPPEDWINDOW;
		DWORD ExStyle = 0;
	};

	explicit UWindow(HINSTANCE hInst, FWindowDesc desc);
	virtual ~UWindow();

	UWindow(const UWindow&) = delete;
	UWindow& operator=(const UWindow&) = delete;
	UWindow(const UWindow&&) = delete;
	UWindow& operator=(const UWindow&&) = delete;

	HWND Handle() const;
	void Show(int nShowCmd) const;

protected:
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual LRESULT OnMessage(UINT msg, WPARAM wp, LPARAM lp);

private:
	void RegisterWindowClass() const;
	void UnregisterWindowClass() const;
	void Create();
	static LRESULT CALLBACK WndProcSetup(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);
	static LRESULT CALLBACK WndProcThunk(HWND hWnd, UINT msg, WPARAM wp, LPARAM lp);

private:
	HINSTANCE HInstance{};
	HWND Hwnd{};
	FWindowDesc Desc{};
};

