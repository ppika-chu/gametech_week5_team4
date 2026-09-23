#pragma once

#include "ImGui/imgui.h"

class ConsoleWindow final {
public:
	bool bIsDrawerOpen = false;
	bool bShowLog = true;
	bool bShowWarn = true;
	bool bShowError = true;

	char InputBuf[256];
	ImVector<const char*> Commands;
	ImVector<char*> History;

	int HistoryPos = -1;    // -1: new line, 0..History.Size-1 browsing history.
	ImGuiTextFilter Filter;
	bool AutoScroll = true;
	bool ScrollToBottom;
	int MaxLine = 256;

	static constexpr float HEIGHT_RATIO = 0.3f;

	bool bShowStatFPS = false;
	bool bShowStatMemory = false;
	bool bShowStatRender = false;

	void Process(float BottomBarHeight);
	static ConsoleWindow& Get() {
		static ConsoleWindow Instance;
		return Instance;
	}

	inline void ToggleDrawer() { bIsDrawerOpen = !bIsDrawerOpen; }
	inline void SetIsDrawerOpen(bool InIsDrawerOpen) { bIsDrawerOpen = InIsDrawerOpen; }

	void Init(int MaxLines) {
		if (MaxLines > MaxLine) return;
		MaxLine = MaxLines;
	};
	ConsoleWindow(const ConsoleWindow&) = delete;
	ConsoleWindow& operator=(const ConsoleWindow&) = delete;
private:
	ConsoleWindow();
	~ConsoleWindow();

	static int TextEditCallbackStub(ImGuiInputTextCallbackData* data)
	{
		ConsoleWindow* console = (ConsoleWindow*)data->UserData;
		return console->TextEditCallback(data);
	}
	int TextEditCallback(ImGuiInputTextCallbackData* data);
	void ExecCommand(const char* command_line);

	float mDrawerHeight = 350.0f;
};
