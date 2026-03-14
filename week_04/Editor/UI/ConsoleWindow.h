#pragma once
#include "imgui.h"
#include <functional>

// 커맨드 핸들러 타입: 커맨드 문자열 → 처리
using FCommandHandler = std::function<void(const char*)>;

class CConsoleWindow
{
public:
	CConsoleWindow();
	~CConsoleWindow();

	void Render();
	void AddLog(const char* Fmt, ...) IM_FMTARGS(2);
	void ClearLog();

	// 외부에서 커맨드 등록
	void RegisterCommand(const char* Command);
	// 커맨드 실행 핸들러 등록 (등록 안 하면 내장 HELP/CLEAR/HISTORY만 처리)
	void SetCommandHandler(FCommandHandler Handler) { CommandHandler = Handler; }

private:
	void ExecCommand(const char* CommandLine);

	static int  TextEditCallbackStub(ImGuiInputTextCallbackData* Data);
	int         TextEditCallback(ImGuiInputTextCallbackData* Data);

	static int  Stricmp(const char* S1, const char* S2);
	static int  Strnicmp(const char* S1, const char* S2, int N);
	static char* Strdup(const char* S);
	static void  Strtrim(char* S);

	char              InputBuf[256];
	ImVector<char*>   Items;
	ImVector<const char*> Commands;
	ImVector<char*>   History;
	int               HistoryPos = -1;
	ImGuiTextFilter   Filter;
	bool              AutoScroll = true;
	bool              ScrollToBottom = false;

	FCommandHandler   CommandHandler;
};