#pragma once
#include <windows.h>
#include <vector>

//현재 프레임 입력 상태
struct FInputState
{
	bool bDown[256] = {};
	bool bPressed[256] = {};
	bool bReleased[256] = {};
	//이번 프레임 마우스 이동량
	long MouseDX = 0, MouseDY = 0;
	int  CursorX = 0, CursorY = 0;

	//이번 프레임 마우스휠(중간) 이동량
	float MouseWheelDelta = 0;

	void BeginFrame()
	{
		memset(bPressed, 0, sizeof(bPressed));
		memset(bReleased, 0, sizeof(bReleased));
		MouseDX = MouseDY = 0;
		MouseWheelDelta = 0;
	}

	void OnKeyDown(int vk) { bPressed[vk] = true; bDown[vk] = true; }
	void OnKeyUp(int vk) { bReleased[vk] = true; bDown[vk] = false; }
	void OnRawMouse(long dx, long dy) { MouseDX += dx; MouseDY += dy; }
	void OnMouseWheel(float Delta) { MouseWheelDelta = Delta; }

	void OnFocusLost() { memset(bDown, 0, sizeof(bDown)); MouseDX = MouseDY = 0; }

	bool IsDown(int vk)      const { return bDown[vk]; }
	bool WasPressed(int vk)  const { return bPressed[vk]; }
	bool WasReleased(int vk) const { return bReleased[vk]; }
};
