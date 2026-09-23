#include "Console.h"
#include "FLogManager.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "ImGui/imgui_impl_win32.h"
#include <string.h>
#include <ctime>

namespace {
	void ButtonHelper(bool& bShow, int type) {
		ImVec4 BaseColor;
		if (bShow)
			BaseColor = ImGui::GetStyleColorVec4(ImGuiCol_Header);
		else
			BaseColor = ImGui::GetStyleColorVec4(ImGuiCol_MenuBarBg);

		ImVec4 HoverColor{ BaseColor.x * 1.3f, BaseColor.y * 1.3f, BaseColor.z * 1.3f, BaseColor.w };
		ImGui::PushStyleColor(ImGuiCol_HeaderHovered, HoverColor);

		const char* name = nullptr;
		switch (type) {
		case 0: name = "Log"; break;
		case 1: name = "Warning"; break;
		case 2: name = "Error"; break;
		default: break;
		}
		auto SelectableWidth = [](const char* Text)
			{
				return ImGui::CalcTextSize(Text).x;
			};
		if (ImGui::Selectable(name, bShow, 0, ImVec2(SelectableWidth(name), 0.0f))) {
			bShow = !bShow;
		}
		ImGui::PopStyleColor();
	}
}

static int   Stricmp(const char* s1, const char* s2) { int d; while ((d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; } return d; }
static int   Strnicmp(const char* s1, const char* s2, int n) { int d = 0; while (n > 0 && (d = toupper(*s2) - toupper(*s1)) == 0 && *s1) { s1++; s2++; n--; } return d; }
static char* Strdup(const char* s) { IM_ASSERT(s); size_t len = strlen(s) + 1; void* buf = ImGui::MemAlloc(len); IM_ASSERT(buf); return (char*)memcpy(buf, (const void*)s, len); }
static void  Strtrim(char* s) { char* str_end = s + strlen(s); while (str_end > s && str_end[-1] == ' ') str_end--; *str_end = 0; }

void ConsoleWindow::Process(float BottomBarHeight)
{
	if (!bIsDrawerOpen)
		return;

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();


	const ImVec2 DrawerPos = { Viewport->WorkPos.x, Viewport->WorkPos.y + Viewport->WorkSize.y - BottomBarHeight - mDrawerHeight };
	const ImVec2 DrawerSize = { Viewport->WorkSize.x, mDrawerHeight };

	ImGui::SetNextWindowPos(DrawerPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(DrawerSize, ImGuiCond_Appearing);
	ImGui::SetNextWindowViewport(Viewport->ID);

	const ImGuiWindowFlags DrawerFlags =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDocking;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(32, 32, 32, 245));

	if (!ImGui::Begin("Console Window", &bIsDrawerOpen, DrawerFlags))
	{
		ImGui::End();
		ImGui::PopStyleColor();
		return;
	}

	mDrawerHeight = ImGui::GetWindowHeight();

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Close Console")) {
			bIsDrawerOpen = false;
		}
		ImGui::EndPopup();
	}

	if (!ImGui::Begin("Console Window", &bIsDrawerOpen, ImGuiWindowFlags_MenuBar)) {
		ImGui::End();
		return;
	}

	if (ImGui::BeginPopupContextItem())
	{
		if (ImGui::MenuItem("Close Console")) {
			bIsDrawerOpen = false;
		}
		ImGui::EndPopup();
	}
	bool copy_to_clipboard = false;
	if (ImGui::BeginMenuBar()) {
		if (ImGui::BeginMenu("Actions"))
		{
			copy_to_clipboard = ImGui::MenuItem("Copy");
			if (ImGui::MenuItem("Clear")) { FLogManager::Get().Clear(); }
			ImGui::EndMenu();
		}

		ButtonHelper(bShowLog, 0);
		ButtonHelper(bShowWarn, 1);
		ButtonHelper(bShowError, 2);

		//Filter.Draw();
		if (ImGui::InputTextWithHint("##Filter", "Filter (inc,-exc)", Filter.InputBuf, IM_ARRAYSIZE(Filter.InputBuf)))
			Filter.Build();

		ImGui::EndMenuBar();
	}

	std::time_t now = std::time(nullptr);
	std::tm local_time;
	localtime_s(&local_time, &now);

	if (ImGui::SmallButton("Add Debug Text")) { UE_LOG("%dY-%dm-%dd %dH:%dM:%dS", local_time.tm_year + 1900, local_time.tm_mon + 1, local_time.tm_mday, local_time.tm_hour, local_time.tm_min, local_time.tm_sec); UE_LOG_WARN("진돗개 둘"); UE_LOG_ERROR("DEFCON 1!!"); }

	ImGuiStyle& style = ImGui::GetStyle();
	const float footer_height_to_reserve = style.SeparatorSize + style.ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
	if (ImGui::BeginChild("ScrollingRegion", ImVec2(0, -footer_height_to_reserve), ImGuiChildFlags_NavFlattened, ImGuiWindowFlags_HorizontalScrollbar)) {
		if (ImGui::BeginPopupContextWindow())
		{
			if (ImGui::Selectable("Clear")) FLogManager::Get().Clear();;
			ImGui::EndPopup();
		}
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
		if (copy_to_clipboard)
			ImGui::LogToClipboard();
		for (FString item : FLogManager::Get().GetLogs())
		{
			const char* str = item.c_str();
			if (!Filter.PassFilter(str))
				continue;

			// Normally you would store more information in your item than just a string.
			// (e.g. make Items[] an array of structure, store color/type etc.)
			ImVec4 color;
			bool has_color = false;
			if (strstr(str, "[ERROR]")) {
				if (!bShowError) continue;
				color = ImVec4(1.0f, 0.4f, 0.4f, 1.0f); has_color = true;
			}
			else if (strstr(str, "[Warning]")) {
				if (!bShowWarn) continue;
				color = ImVec4(0.6f, 0.8f, 0.4f, 1.0f); has_color = true;
			}
			else if (!bShowLog) continue;
			else if (strncmp(str, "# ", 2) == 0) { color = ImVec4(1.0f, 0.8f, 0.6f, 1.0f); has_color = true; }
			if (has_color)
				ImGui::PushStyleColor(ImGuiCol_Text, color);
			ImGui::TextUnformatted(str);
			if (has_color)
				ImGui::PopStyleColor();
		}
		if (copy_to_clipboard)
			ImGui::LogFinish();

		// Keep up at the bottom of the scroll region if we were already at the bottom at the beginning of the frame.
		// Using a scrollbar or mouse-wheel will take away from the bottom edge.
		if (ScrollToBottom || (AutoScroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
			ImGui::SetScrollHereY(1.0f);
		ScrollToBottom = false;

		ImGui::PopStyleVar();


	}
	ImGui::EndChild();

	ImGui::Separator();

	// Command-line
	bool reclaim_focus = false;
	ImGuiInputTextFlags input_text_flags = ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_EscapeClearsAll | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory;
	if (ImGui::InputText("Input", InputBuf, IM_COUNTOF(InputBuf), input_text_flags, &TextEditCallbackStub, (void*)this))
	{
		char* s = InputBuf;
		Strtrim(s);
		if (s[0])
			ExecCommand(s);
		strcpy_s(s, 256, "");
		reclaim_focus = true;
	}

	// Auto-focus on window apparition
	ImGui::SetItemDefaultFocus();
	if (reclaim_focus)
		ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget


	ImGui::End();

	ImGui::End();
	ImGui::PopStyleColor();
}

ConsoleWindow::ConsoleWindow()
{
	FLogManager::Get().Clear();
	memset(InputBuf, 0, sizeof(InputBuf));
	HistoryPos = -1;

	// "CLASSIFY" is here to provide the test case where "C"+[tab] completes to "CL" and display multiple matches.
	Commands.push_back("HELP");
	Commands.push_back("HISTORY");
	Commands.push_back("CLEAR");
	//Commands.push_back("CLASSIFY");
	Commands.push_back("STAT FPS");
	Commands.push_back("STAT MEMORY");
	Commands.push_back("STAT RENDER");
	Commands.push_back("STAT ALL");
	Commands.push_back("STAT NONE");
	AutoScroll = true;
	ScrollToBottom = false;
}

ConsoleWindow::~ConsoleWindow()
{
	FLogManager::Get().Clear();
	for (int i = 0; i < History.Size; i++)
		ImGui::MemFree(History[i]);
}

int ConsoleWindow::TextEditCallback(ImGuiInputTextCallbackData* data)
{
	//AddLog("cursor: %d, selection: %d-%d", data->CursorPos, data->SelectionStart, data->SelectionEnd);
	switch (data->EventFlag)
	{
	case ImGuiInputTextFlags_CallbackCompletion:
	{
		// Example of TEXT COMPLETION

		// Locate beginning of current word
		const char* word_end = data->Buf + data->CursorPos;
		const char* word_start = word_end;
		while (word_start > data->Buf)
		{
			const char c = word_start[-1];
			if (c == ' ' || c == '\t' || c == ',' || c == ';')
				break;
			word_start--;
		}

		// Build a list of candidates
		ImVector<const char*> candidates;
		for (int i = 0; i < Commands.Size; i++)
			if (Strnicmp(Commands[i], word_start, (int)(word_end - word_start)) == 0)
				candidates.push_back(Commands[i]);

		if (candidates.Size == 0)
		{
			// No match
			UE_LOG("No match for \"%.*s\"!\n", (int)(word_end - word_start), word_start);
		}
		else if (candidates.Size == 1)
		{
			// Single match. Delete the beginning of the word and replace it entirely so we've got nice casing.
			data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
			data->InsertChars(data->CursorPos, candidates[0]);
			data->InsertChars(data->CursorPos, " ");
		}
		else
		{
			// Multiple matches. Complete as much as we can..
			// So inputting "C"+Tab will complete to "CL" then display "CLEAR" and "CLASSIFY" as matches.
			int match_len = (int)(word_end - word_start);
			for (;;)
			{
				int c = 0;
				bool all_candidates_matches = true;
				for (int i = 0; i < candidates.Size && all_candidates_matches; i++)
					if (i == 0)
						c = toupper(candidates[i][match_len]);
					else if (c == 0 || c != toupper(candidates[i][match_len]))
						all_candidates_matches = false;
				if (!all_candidates_matches)
					break;
				match_len++;
			}

			if (match_len > 0)
			{
				data->DeleteChars((int)(word_start - data->Buf), (int)(word_end - word_start));
				data->InsertChars(data->CursorPos, candidates[0], candidates[0] + match_len);
			}

			// List matches
			UE_LOG("Possible matches:\n");
			for (int i = 0; i < candidates.Size; i++)
				UE_LOG("- %s\n", candidates[i]);
		}

		break;
	}
	case ImGuiInputTextFlags_CallbackHistory:
	{
		// Example of HISTORY
		const int prev_history_pos = HistoryPos;
		if (data->EventKey == ImGuiKey_UpArrow)
		{
			if (HistoryPos == -1)
				HistoryPos = History.Size - 1;
			else if (HistoryPos > 0)
				HistoryPos--;
		}
		else if (data->EventKey == ImGuiKey_DownArrow)
		{
			if (HistoryPos != -1)
				if (++HistoryPos >= History.Size)
					HistoryPos = -1;
		}

		// A better implementation would preserve the data on the current input line along with cursor position.
		if (prev_history_pos != HistoryPos)
		{
			const char* history_str = (HistoryPos >= 0) ? History[HistoryPos] : "";
			data->DeleteChars(0, data->BufTextLen);
			data->InsertChars(0, history_str);
		}
	}
	}
	return 0;
}

void ConsoleWindow::ExecCommand(const char* command_line)
{
	UE_LOG("# %s\n", command_line);

	// Insert into history. First find match and delete it so it can be pushed to the back.
	// This isn't trying to be smart or optimal.
	HistoryPos = -1;
	for (int i = History.Size - 1; i >= 0; i--)
		if (Stricmp(History[i], command_line) == 0)
		{
			ImGui::MemFree(History[i]);
			History.erase(History.begin() + i);
			break;
		}
	History.push_back(Strdup(command_line));

	// Process command
	if (Stricmp(command_line, "CLEAR") == 0)
	{
		FLogManager::Get().Clear();
	}
	else if (Stricmp(command_line, "HELP") == 0)
	{
		UE_LOG("Commands:");
		for (int i = 0; i < Commands.Size; i++)
			UE_LOG("- %s", Commands[i]);
	}
	else if (Stricmp(command_line, "HISTORY") == 0)
	{
		int first = History.Size - 10;
		for (int i = first > 0 ? first : 0; i < History.Size; i++)
		{
			UE_LOG("%3d: %s\n", i, History[i]);
		}
	}
	else if (Stricmp(command_line, "STAT FPS") == 0)
	{
		if (bShowStatFPS)
		{
			bShowStatFPS = false;
			return;
		}
		bShowStatFPS = true;
	}
	else if (Stricmp(command_line, "STAT MEMORY") == 0)
	{
		if (bShowStatMemory)
		{
			bShowStatMemory = false;
			return;
		}
		bShowStatMemory = true;
	}
	else if (Stricmp(command_line, "STAT RENDER") == 0)
	{
		if (bShowStatRender)
		{
			bShowStatRender = false;
			return;
		}
		bShowStatRender = true;
	}
	else if (Stricmp(command_line, "STAT ALL") == 0)
	{
		bShowStatFPS = true;
		bShowStatMemory = true;
		bShowStatRender = true;
	}
	else if (Stricmp(command_line, "STAT NONE") == 0)
	{
		bShowStatFPS = false;
		bShowStatMemory = false;
		bShowStatRender = false;
	}
	else
	{
		UE_LOG("Unknown command: '%s'\n", command_line);
	}

	// On command input, we scroll to bottom even if AutoScroll==false
	ScrollToBottom = true;
}

