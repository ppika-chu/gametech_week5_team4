#pragma once

#include <windows.h>
#include <vector>
#include "InputState.h"

//지연된 메시지
struct FDeferredMessage
{
	HWND   hWnd = nullptr;
	UINT   Message = 0;
	WPARAM wParam = 0;
	LPARAM lParam = 0;

	// WM_INPUT 은 lParam 의 HRAWINPUT 핸들이 WndProc 안에서만 유효하다.
	// 그래서 WndProc 에서 미리 풀어 여기에 담아 둔다.
	long   RawMouseDX = 0;
	long   RawMouseDY = 0;
};


class FWindowApplication
{
public:
	bool bPendingResize = false;
	UINT PendingWidth = 0, PendingHeight = 0;
	FInputState Input;

	void Defer(const FDeferredMessage& M) { Deferred.push_back(M); }

	// 게임 루프에서 프레임당 1회 — 여기가 유일한 해석 지점
	void ProcessDeferredEvents()
	{
		Input.BeginFrame();

		// 처리 도중 새 메시지가 들어올 수 있으니 통째로 넘겨받고 순회
		std::vector<FDeferredMessage> Local;
		Local.swap(Deferred);

		for (const FDeferredMessage& M : Local)
		{
			switch (M.Message)
			{
			case WM_KEYDOWN: case WM_SYSKEYDOWN:
				// lParam 30번 비트 = 이전 키 상태. 1 이면 OS 자동 반복이라 무시
				if ((M.lParam & (1 << 30)) == 0) Input.OnKeyDown((int)M.wParam);
				break;

			case WM_KEYUP: case WM_SYSKEYUP:
				Input.OnKeyUp((int)M.wParam);
				break;

			case WM_LBUTTONDOWN: Input.OnKeyDown(VK_LBUTTON); break;
			case WM_LBUTTONUP:   Input.OnKeyUp(VK_LBUTTON);   break;
			case WM_RBUTTONDOWN: Input.OnKeyDown(VK_RBUTTON); break;
			case WM_RBUTTONUP:   Input.OnKeyUp(VK_RBUTTON);   break;
			case WM_MOUSEWHEEL:
				Input.OnMouseWheel(GET_WHEEL_DELTA_WPARAM(M.wParam) / (float)WHEEL_DELTA);
				break;
			case WM_MOUSEMOVE:
				{
					Input.CursorX = (short)LOWORD(M.lParam);
					Input.CursorY = (short)HIWORD(M.lParam);
				}
				break;
			case WM_INPUT:
				Input.OnRawMouse(M.RawMouseDX, M.RawMouseDY);
				break;

			case WM_KILLFOCUS:
				Input.OnFocusLost();
				break;
			}
		}
	}

private:
	std::vector<FDeferredMessage> Deferred;
};

inline FWindowApplication WindowApplication;
