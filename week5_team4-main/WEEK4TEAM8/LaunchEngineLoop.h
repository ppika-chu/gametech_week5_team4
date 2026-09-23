#pragma once

#include <Windows.h>
#include "FrameTimer.h"
#include "FEditorViewportClient.h"
#include "Camera.h"
#include "SceneManager.h"
#include "FileManager.h"
#include "Renderer.h"
#include "World.h"
#include "FAssetManager.h"
#include "FFontManager.h"
#include "FComponentVisualizer.h"
#include "FContentBrowser.h"
#include "GraphicsManager.h"

#include <d3d11.h>

class Sphere;
class FGraphicsManager;
class FObjViewer;

struct FEditorLayout
{
	bool bIsSplitView = false;
	int32 MaximizedViewportIndex = 0;

	TSharedPtr<SWindow> RootWindow;
	TSharedPtr<SSplitterH> HSplitter;
	TSharedPtr<SSplitterV> VSplitter[2];
	TSharedPtr<SWindow> ViewportWindows[4];

	void Initialize(const FRect& InRect)
	{
		// Build viewport layout tree
		TSharedPtr<SSplitterH> HRoot = MakeShared<SSplitterH>();
		TSharedPtr<SSplitterV> VSplitter0 = MakeShared<SSplitterV>();
		TSharedPtr<SSplitterV> VSplitter1 = MakeShared<SSplitterV>();

		for (int32 i = 0; i < 4; ++i)
		{
			ViewportWindows[i] = MakeShared<SWindow>();
		}

		VSplitter0->SideLT = ViewportWindows[0];
		VSplitter0->SideRB = ViewportWindows[1];

		VSplitter1->SideLT = ViewportWindows[2];
		VSplitter1->SideRB = ViewportWindows[3];

		HRoot->SideLT = VSplitter0;
		HRoot->SideRB = VSplitter1;

		HRoot->SetRect(InRect);

		RootWindow = HRoot;
		HSplitter = HRoot;
		VSplitter[0] = VSplitter0;
		VSplitter[1] = VSplitter1;
	}

	void Resize(const FRect& InRect)
	{
		RootWindow->SetRect(InRect);
	}

	void SetSplitRatios(float HorizontalRatio, float VerticalRatio)
	{
		HSplitter->SplitterRatio = HorizontalRatio;
		VSplitter[0]->SplitterRatio = VerticalRatio;
		VSplitter[1]->SplitterRatio = VerticalRatio;
	}

	void GetSplitRatios(float& HorizontalRatio, float& VerticalRatio)
	{
		HorizontalRatio = HSplitter->SplitterRatio;
		VerticalRatio = VSplitter[0]->SplitterRatio;
	}
};

struct FEditorViewport
{
	TSharedPtr<SWindow> Window;
	TSharedPtr<FViewport> Viewport;
	TSharedPtr<FEditorViewportClient> Client;

	void Release()
	{
		Viewport.reset();
		Client.reset();
		Window.reset();
	}
};

class FEngineLoop
{
public:
	FEngineLoop() = default;
	~FEngineLoop() = default;

	void Init(HINSTANCE hInstance, WNDPROC WndProc);
	void Tick(bool bPumpMessages);
	void End();

	FAssetManager* GetAssetManager() { return mAssetManager; }

	static constexpr int32 MaxViewportCount = 4;
	static constexpr int32 MainViewportIndex = 0;

	FEditorViewport& GetMainViewport() { return mViewports[MainViewportIndex]; }
	const FEditorViewport GetMainViewport() const { return mViewports[MainViewportIndex]; }

	void SetMouseCursor(ImGuiMouseCursor InMouseCursor) { mMouseCursor = InMouseCursor; }

private:
	void InitAssetManager();
	
	void SaveEditorSettings();
	void LoadEditorSettings();

private:
	// Todo: Make as pointer
	FFrameTimer* FrameTimer;
	bool GInTick = false;

	ImGuiMouseCursor mMouseCursor = ImGuiMouseCursor_Arrow;

	FEditorLayout mEditorLayout;
	FEditorViewport mViewports[4]; // 0 : MainView 1, 2, 3 : Other

	FGraphicsManager* mGraphicsManager;
	FSceneManager* mSceneManager;
	FFileManager* mFileManager;
	FAssetManager* mAssetManager;
	FFontManager* mFontManager;

	FComponentVisualizerManager* mComponentVisualizerManager;

#if IS_OBJ_VIEWER
	FObjViewer mObjViewer;
#endif
};

inline FEngineLoop GEngineLoop;
