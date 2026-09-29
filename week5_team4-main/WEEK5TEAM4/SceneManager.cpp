#include "SceneManager.h"

#include <algorithm>
#include <format>

#include "FileManager.h"
#include "NativeFileDialog.h"
#include "EngineStatics.h"
#include "JsonUtil.h"
#include "ObjectFactory.h"
#include "PrimitiveComponent.h"
#include "TArray.h"
#include "World.h"
#include "FEditorViewportClient.h"
#include "Camera.h"
#include "Console.h"
#include "FLogManager.h"

#include "ImGui/imgui.h"
#include "ImGui/imgui_internal.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"

#include "FrameTimer.h"
#include "CubeComponent.h"
#include "UAtlasAnimationComponent.h"
#include "ActorComponent.h"
#include "WindowApplication.h"

#include "Cube.h"
#include "Assets.h"
#include "UTextComponent.h"
#include "ShowFlags.h"
#include "UStaticMeshComponent.h"
#include "LaunchEngineLoop.h"
#include "FAssetManager.h"
#include "FTextBuilder.h"
#include "FSceneConverter.h"

#include "OptimizationFlags.h"

FSceneManager::FSceneManager()
{
	ImGuiIO& io = ImGui::GetIO();
	mPanelWidth = io.DisplaySize.x * MIN_WIDTH_RATIO;
	mViewportX = 0;
	mViewportY = 0;
	mViewportWidth = WindowApplication.PendingWidth;
	mViewportHeight = WindowApplication.PendingHeight;
	mBottomBarHeight = 30.0f;

	mContentBrowser.Initialize(kDefaultAssetsPath);
	mContentBrowser.SetEventHandler(this);
}

FSceneManager::~FSceneManager()
{
	FObjectFactory::DestroyObject(mCurrentWorld);
}

void FSceneManager::OnNewAssetFile(const FAssetFileHeader& Header, const std::filesystem::path& FilePath)
{
	FAssetManager& AssetManager = FAssetManager::Get();

	if (Header.AssetType == EAssetType::Texture2D ||
		Header.AssetType == EAssetType::Material ||
		Header.AssetType == EAssetType::StaticMesh)
	{

		FAssetManager::Get().ScanDirectory("Assets", *mRenderer);

		// 현재 자신이 있는 폴더만 refresh 하는 문제가 있어서 위와 같이 수정함.
		// (mesh면 mesh 폴더만 refresh. texture 폴더는 안 하는 문제)
		// guid 검사하여 이미 있는 건 Register pass 하기 때문에 등록 비용 거의 없음.
		#if 0
		FAssetManager::Get().ScanDirectory(
			FilePath.parent_path(),
			*mRenderer);
		#endif
	}
	else
	{
		UE_LOG_ERROR("Unsupported asset type");
	}
}

void FSceneManager::OnDeleteAssetFile(const std::filesystem::path& FilePath)
{
	FAssetManager& AssetManager = FAssetManager::Get();

	std::string CanonicalPath = FilePath.lexically_normal().generic_string();
	AssetManager.UnregisterAsset(FName(CanonicalPath.c_str()));
}

void FSceneManager::RefreshContentBrowser(const std::filesystem::path& TargetDirectory)
{
	if (!std::filesystem::exists(TargetDirectory) || !std::filesystem::is_directory(TargetDirectory))
	{
		UE_LOG_WARN("Current directory not found: '%s', Reverting to RootDirectory.", TargetDirectory);
		return;
	}

	FAssetManager& AssetManager = FAssetManager::Get();

	AssetManager.PurgeStaleAssetsInDirectory(TargetDirectory);

	AssetManager.ScanDirectory(TargetDirectory, *mRenderer);

	mContentBrowser.RefreshCache();
}

void FSceneManager::Tick(float deltaTime)
{
	mCurrentWorld->Tick(deltaTime);
}

void FSceneManager::Render(float deltaTime, FRenderCollector& outCollector)
{
	mCurrentWorld->Render(deltaTime, outCollector);
}

void FSceneManager::UpdateGUI(const FGuiReference& guiReference)
{
	mRenderer = guiReference.GraphicsManager->GetRenderer();

	//ImGui
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	{
		// Docking
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		const ImGuiID dockspaceID = ImGui::GetID("EditorDockSpace");

		const ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_PassthruCentralNode;

		// 저장된 도킹 노드가 없을 때만 기본 배치 생성
		if (!ImGui::DockBuilderGetNode(dockspaceID))
		{
			ImGui::DockBuilderAddNode(dockspaceID, ImGuiDockNodeFlags_DockSpace | flags);
			ImGui::DockBuilderSetNodeSize(dockspaceID, viewport->WorkSize);

			ImGuiID center = dockspaceID;
			ImGuiID left;
			ImGuiID bottom;

			// 왼쪽 패널 2%
			ImGui::DockBuilderSplitNode(center, ImGuiDir_Left, 0.2f, &left, &center);

			ImGuiID leftTop;
			ImGuiID leftRest;
			ImGui::DockBuilderSplitNode(left, ImGuiDir_Up, 0.4f, &leftTop, &leftRest);

			ImGuiID leftMiddle;
			ImGuiID leftBottom;
			ImGui::DockBuilderSplitNode(leftRest, ImGuiDir_Up, 0.5f, &leftMiddle, &leftBottom);

			ImGui::DockBuilderDockWindow("Viewport", center);
			ImGui::DockBuilderDockWindow("Jungle Control Panel", leftTop);
			ImGui::DockBuilderDockWindow("Jungle Property Window", leftMiddle);
			ImGui::DockBuilderDockWindow("Object List Panel", leftBottom);

			ImGui::DockBuilderFinish(dockspaceID);
		}

		ImGui::DockSpaceOverViewport(dockspaceID, viewport, flags);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

		const ImGuiWindowFlags ViewportWindowFlags =
			ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoScrollWithMouse;

		if (ImGui::Begin("Viewport", nullptr, ViewportWindowFlags))
		{
			const ImVec2 Origin = ImGui::GetCursorScreenPos();
			const ImVec2 TotalSize = ImGui::GetContentRegionAvail();

			mViewportX = Origin.x;
			mViewportY = Origin.y;
			mViewportWidth = TotalSize.x;
			mViewportHeight = TotalSize.y;

			ImDrawList* DrawList = ImGui::GetWindowDrawList();

			ImGuiIO& IO = ImGui::GetIO();

			const ImGuiViewport* MainVP = ImGui::GetMainViewport();

			const float DrawerBoundary = MainVP->WorkPos.y + MainVP->WorkSize.y - mContentBrowser.GetDrawerHeight() - mBottomBarHeight;

			bool bIsMouseOnDrawerBoundary = (IO.MousePos.y >= DrawerBoundary) && mContentBrowser.IsDrawerOpen();

			for (int32 i = 0; i < guiReference.ViewportCount; ++i)
			{
				const int32 CurrentViewportIndex = guiReference.EditorLayout->bIsSplitView ? i : guiReference.EditorLayout->MaximizedViewportIndex;

				FEditorViewport* EditorViewport = &guiReference.Viewports[i];

				FRect DrawRect = EditorViewport->Window->Rect;
				if (DrawRect.Width > 0 && DrawRect.Height > 0)
				{
					ImGui::SetCursorScreenPos(ImVec2(DrawRect.X, DrawRect.Y));

					bool bHovered = ImGui::IsMouseHoveringRect(ImVec2(DrawRect.X, DrawRect.Y), 
									ImVec2(DrawRect.X + DrawRect.Width, DrawRect.Y + DrawRect.Height))
								&& !ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId)		// 팝업창, 콤보 드롭다운 등 열리면 false
								&& !bIsMouseOnDrawerBoundary;								// 콘텐츠 브라우저 영역에서 피킹 X
					EditorViewport->Client->SetActive(bHovered);

					const TSharedPtr<FRenderTarget2D>& RenderTarget = EditorViewport->Viewport->RenderTarget;
					DrawList->AddImage((ImTextureID)(intptr_t)RenderTarget->SRV.Get(), ImVec2(DrawRect.X, DrawRect.Y), ImVec2(DrawRect.X + DrawRect.Width, DrawRect.Y + DrawRect.Height));

					ImGui::PushID(CurrentViewportIndex);

					const float ViewportTypeWidth = 95.0f;
					const float ViewModeWidth = 85.0f;
					const float MaximizeButtonWidth = 28.0f;
					const float SplitButtonWidth = 28.0f;

					const float Spacing = ImGui::GetStyle().ItemSpacing.x;
					const float MarginX = 8.0f;
					const float MarginY = 4.0f;
					const float ToolBarHeight = 28.0f;

					const float ToolBarWidth = ViewportTypeWidth + ViewModeWidth + MaximizeButtonWidth + SplitButtonWidth + (Spacing * 3.0f) + MarginX;
					const float StartCursorPos = DrawRect.X + DrawRect.Width - ToolBarWidth;
					const float ItemHeight = 22.0f;

					DrawList->AddRectFilled(ImVec2(DrawRect.X, DrawRect.Y), ImVec2(DrawRect.X + DrawRect.Width, DrawRect.Y + ToolBarHeight), IM_COL32(30, 30, 30, 180));

					ImGui::SetCursorScreenPos(ImVec2(StartCursorPos, DrawRect.Y + MarginY));

					ImGui::PushStyleColor(ImGuiCol_Text, IM_COL32(255, 255, 255, 255));
					ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(15, 15, 15, 230));
					ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, IM_COL32(45, 45, 45, 240));
					ImGui::PushStyleColor(ImGuiCol_FrameBgActive, IM_COL32(60, 60, 60, 255));

					ImGui::PushStyleColor(ImGuiCol_PopupBg, IM_COL32(20, 20, 20, 250)); 
					ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(50, 50, 50, 255));
					ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(75, 75, 75, 255));

					ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(15, 15, 15, 230));
					ImGui::PushStyleColor(ImGuiCol_ButtonHovered, IM_COL32(55, 55, 55, 240));
					ImGui::PushStyleColor(ImGuiCol_ButtonActive, IM_COL32(80, 80, 80, 255));

					ImGui::SetNextItemWidth(100.0f);

					const char* ViewportTypeNames[] = { "Perspective", "Top", "Front", "Side" };
					int32 CurrentTypeIndex = static_cast<int32>(EditorViewport->Client->GetViewportType());

					if (ImGui::Combo("##ViewportType", &CurrentTypeIndex, ViewportTypeNames, IM_ARRAYSIZE(ViewportTypeNames)))
					{
						EditorViewport->Client->SetViewportType(static_cast<EViewportType>(CurrentTypeIndex));
					}

					ImGui::SameLine();

					ImGui::SetNextItemWidth(80.0f);

					const char* ViewModeNames[] = { "Lit", "UnLit", "Wireframe" };
					int32 CurrentModeIndex = static_cast<int32>(EditorViewport->Client->GetViewMode());

					if (ImGui::Combo("##ViewMode", &CurrentModeIndex, ViewModeNames, IM_ARRAYSIZE(ViewModeNames)))
					{
						EditorViewport->Client->SetViewMode(static_cast<EViewModeIndex>(CurrentModeIndex));
					}

					ImGui::SameLine();

					if (ImGui::Button("##Maximize", ImVec2(MaximizeButtonWidth, ItemHeight)))
					{
						guiReference.EditorLayout->MaximizedViewportIndex = CurrentViewportIndex;
						guiReference.EditorLayout->bIsSplitView = false;
					}
					FEditorIconUtils::DrawMaximizeButtonIcon(DrawList);

					ImGui::SameLine();

					if (ImGui::Button("##Split", ImVec2(MaximizeButtonWidth, ItemHeight)))
					{
						guiReference.EditorLayout->bIsSplitView = true;
					}
					FEditorIconUtils::DrawSplitButtonIcon(DrawList);

					ImGui::PopStyleColor(10);
					ImGui::PopID();
					ImGui::Dummy(ImVec2(DrawRect.Width, DrawRect.Height));
				}
			}

			if (guiReference.EditorLayout->bIsSplitView)
			{
				float HorizontalRatio, VerticalRatio;
				guiReference.EditorLayout->GetSplitRatios(HorizontalRatio, VerticalRatio);

				bool bOnSplitBarCursor = false;

				const float SplitThickness = 1.0f;
				const float SplitHandleThickness = 8.0f;

				float SplitX = mViewportX + mViewportWidth * VerticalRatio;
				float SplitY = mViewportY + mViewportHeight * HorizontalRatio;

				ImGui::SetCursorScreenPos(ImVec2(SplitX - SplitHandleThickness * 0.5f, mViewportY));
				ImGui::InvisibleButton("##SplitVertical", ImVec2(SplitHandleThickness, mViewportHeight));

				if (ImGui::IsItemHovered())
				{
					bOnSplitBarCursor = true;
				}
				if (ImGui::IsItemActive())
				{
					VerticalRatio = (IO.MousePos.x - mViewportX) / mViewportWidth;
					VerticalRatio = std::clamp(VerticalRatio, 0.1f, 0.9f);
				}

				ImGui::SetCursorScreenPos(ImVec2(mViewportX, SplitY - SplitHandleThickness * 0.5f));
				ImGui::InvisibleButton("##SplitHorizontal", ImVec2(mViewportWidth, SplitHandleThickness));
				if (ImGui::IsItemHovered())
				{
					bOnSplitBarCursor = true;
				}
				if (ImGui::IsItemActive())
				{
					HorizontalRatio = (IO.MousePos.y - mViewportY) / mViewportHeight;
					HorizontalRatio = std::clamp(HorizontalRatio, 0.1f, 0.9f);
				}

				SplitX = mViewportX + mViewportWidth * VerticalRatio;
				SplitY = mViewportY + mViewportHeight * HorizontalRatio;

				DrawList->AddLine(ImVec2(SplitX, mViewportY), ImVec2(SplitX, mViewportY + mViewportHeight), ImColor(0.8f, 0.8f, 0.8f, 1.0f), SplitThickness);
				DrawList->AddLine(ImVec2(mViewportX, SplitY), ImVec2(mViewportX + mViewportWidth, SplitY), ImColor(0.8f, 0.8f, 0.8f, 1.0f), SplitThickness);

				guiReference.EditorLayout->SetSplitRatios(HorizontalRatio, VerticalRatio);
				if (bOnSplitBarCursor)
					GEngineLoop.SetMouseCursor(ImGuiMouseCursor_ResizeAll);
				else
					GEngineLoop.SetMouseCursor(ImGuiMouseCursor_Arrow);
			}
		}
		ImGui::End();

		const ImGuiViewport* Viewport = ImGui::GetMainViewport();

		ImGui::SetNextWindowPos(ImVec2(Viewport->WorkPos.x, Viewport->WorkPos.y + Viewport->WorkSize.y - mBottomBarHeight));
		ImGui::SetNextWindowSize(ImVec2(Viewport->WorkSize.x, mBottomBarHeight));
		ImGui::SetNextWindowViewport(Viewport->ID);

		const ImGuiWindowFlags BottomBarFlags =
			ImGuiWindowFlags_NoDecoration |
			ImGuiWindowFlags_NoMove |
			ImGuiWindowFlags_NoSavedSettings |
			ImGuiWindowFlags_NoDocking;

		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(6.0f, 3.0f));
		ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(24, 24, 24, 255));

		if (ImGui::Begin("##EditorBottomBar", nullptr, BottomBarFlags))
		{
			ImGui::PushStyleColor(ImGuiCol_Button, IM_COL32(45, 45, 48, 255));

			if (ImGui::Button("[ Content Drawer] (Ctrl+Space)"))
			{
				ConsoleWindow::Get().SetIsDrawerOpen(false);
				mContentBrowser.ToggleDrawer();
			}

			ImGui::SameLine();

			if (ImGui::Button("[ Console ]"))
			{
				ConsoleWindow::Get().ToggleDrawer();
				mContentBrowser.SetIsDrawerOpen(false);
			}

			ImGui::PopStyleColor();
		}

		ImGui::End();

		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);

		ConsoleWindow& console = ConsoleWindow::Get();
		if (console.bShowStatFPS || console.bShowStatMemory || console.bShowStatRender)
		{
			// Viewport 창 안쪽 좌상단에 붙는 입력을 받지 않는 오버레이 창
			ImGui::SetNextWindowPos(ImVec2(mViewportX + 12.0f, mViewportY + 12.0f), ImGuiCond_Always);
			ImGui::SetNextWindowBgAlpha(0.55f);

			const ImGuiWindowFlags overlayFlags =
				ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_NoNav |
				ImGuiWindowFlags_NoInputs;

			ImGui::Begin("##StatOverlay", nullptr, overlayFlags);
			if (console.bShowStatFPS)
			{
				if (console.bShowStatMemory || console.bShowStatRender)
				{
					ImGui::Separator();
				}

				ImGui::TextColored(ImVec4(0.35f, 1.0f, 0.35f, 1.0f), "FPS");
				ImGui::Text("FPS: %.1f", guiReference.FrameTimer->GetFPS());
				ImGui::Text("Frame: %.2f ms", guiReference.FrameTimer->GetDeltaTime() * 1000.0f);
			}

			if (console.bShowStatMemory)
			{
				if (console.bShowStatFPS || console.bShowStatRender)
				{
					ImGui::Separator();
				}

				ImGui::TextColored(ImVec4(0.35f, 0.8f, 1.0f, 1.0f), "Memory");
				ImGui::Text("Total allocated memory count: %d", UEngineStatics::sTotalAllocationCount);
				ImGui::Text("Total allocated memory size: %d bytes", UEngineStatics::sTotalAllocationBytes);

				const FAssetStats Stats = guiReference.AssetManager->GetStats();

				ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.35f, 1.0f), "Assets");
				ImGui::Text("Registered: %u", Stats.RegisteredCount);
				ImGui::Text("Loaded: %u", Stats.LoadedCount);

				ImGui::Text("Registered Static Mesh: %u", Stats.RegisteredStaticMesh);
				ImGui::Text("Loaded Static Mesh: %u", Stats.LoadedStaticMesh);
				ImGui::Text("Registered Static Texture 2D: %u", Stats.RegisteredTexture2D);
				ImGui::Text("Loaded Static Texture 2D: %u", Stats.LoadedTexture2D);
				ImGui::Text("Registered Static Material: %u", Stats.RegisteredMaterial);
				ImGui::Text("Loaded Static Material: %u", Stats.LoadedMaterial);
			}

			if (console.bShowStatRender)
			{
				if (console.bShowStatFPS || console.bShowStatMemory)
				{
					ImGui::Separator();
				}
				ImGui::TextColored(ImVec4(0.35f, 0.8f, 0.5f, 1.0f), "Render");
				ImGui::Text("Draw Calls: %u", guiReference.GraphicsManager->GetRenderer()->GetDrawCallCount());

				ImGui::Text("GPU Render: %.3f ms", guiReference.GraphicsManager->GetGpuRenderTime());

				UINT PrimitiveCount = 0;
				for (TObjectIterator<UPrimitiveComponent> It(true); It; ++It)
				{
					++PrimitiveCount;
				}
				ImGui::Text("Primitives: %u", PrimitiveCount);

				UINT SpotLightCount = 0;
				for (TObjectIterator<USpotLightComponent> It(false); It; ++It)
				{
					++SpotLightCount;
				}
				ImGui::Text("Spot Lights: %u", SpotLightCount);
			}
			ImGui::End();
		}
		ImGui::PopStyleVar();

		// 최적화 전용 STAT
		if (console.bShowOptimization)
		{
			const ImGuiWindowFlags OptFlags =
				ImGuiWindowFlags_NoDecoration |
				ImGuiWindowFlags_NoBackground |
				ImGuiWindowFlags_AlwaysAutoResize |
				ImGuiWindowFlags_NoSavedSettings |
				ImGuiWindowFlags_NoFocusOnAppearing |
				ImGuiWindowFlags_NoNav |
				ImGuiWindowFlags_NoInputs;

			// Viewport 창 안쪽 좌상단에 붙는 입력을 받지 않는 오버레이 창
			ImGui::SetNextWindowPos(ImVec2(mViewportX + 15.0f, mViewportY + 15.0f), ImGuiCond_Always);
			
			ImGui::Begin("##Optimization", nullptr, OptFlags);
			ImGui::PushFont(nullptr, 20.0f);
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.8f, 0.2f, 1.0f));

			// FPS
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.0f, 0.35f, 1.0f));
			ImGui::SeparatorText("FPS");
			ImGui::PopStyleColor();

			const float FrameTimeMs = guiReference.FrameTimer->GetFrameTimeMs();
			const float GPUTimeMs = guiReference.GraphicsManager->GetGpuRenderTime();
			const float TrueFrameTimeMs = (std::max)(FrameTimeMs, GPUTimeMs);

			// 지수 이동 평균. Alpha가 작을수록 더 부드럽고 더 느리게 따라온다.
			static constexpr float Alpha = 0.2f;
			static float SmoothFrameMs = TrueFrameTimeMs;
			static float SmoothGpuMs = GPUTimeMs;

			SmoothFrameMs += (TrueFrameTimeMs - SmoothFrameMs) * Alpha;
			SmoothGpuMs   += (GPUTimeMs       - SmoothGpuMs)   * Alpha;

			const float ShownFPS = SmoothFrameMs > 0.f ? 1000.0f / SmoothFrameMs : 0.f;
			const float ShownCpuMs = (std::max)(SmoothFrameMs - SmoothGpuMs, 0.f);

			ImGui::Text("FPS: %.1f", ShownFPS);
			ImGui::Text("Frame Time: %.2f ms", SmoothFrameMs);
			// ImGui::Text("Frame: %.2f ms", guiReference.FrameTimer->GetDeltaTime() * 1000.0f);
			
			// CPU / GPU
			// ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.0f, 0.35f, 1.0f));
			// ImGui::SeparatorText("Bottleneck");
			// ImGui::PopStyleColor();

			// ImGui::Text("GPU Time: %.2f ms", SmoothGpuMs);
			// ImGui::Text("CPU Time (est.): %.2f ms", ShownCpuMs);

			// if (SmoothFrameMs > 0.f)
			// {
			// 	const float GPURatio = SmoothGpuMs / SmoothFrameMs;
			// 	const bool bGPUBound = GPURatio > 0.8f;

			// 	ImGui::TextColored(
			// 		bGPUBound? ImVec4(1.0f, 0.3f, 0.3f, 1.0f) : ImVec4(1.0f, 0.8f, 0.2f, 1.0f),
			// 		"GPU Ratio: %.2f %%", GPURatio*100.0f);
	
			// }

			// Picking
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.0f, 0.35f, 1.0f));
			ImGui::SeparatorText("Picking");
			ImGui::PopStyleColor();
			
			ImGui::Text("Picking Count: %llu", guiReference.ViewportClient->GetPickAttemptCount());
			ImGui::Text("Ray Test Count: %llu", guiReference.ViewportClient->GetPickTestCount());
			ImGui::Text("Last Picking Time: %.2f ms", guiReference.ViewportClient->GetPickLastTimeMs());
			ImGui::Text("Accumulated Picking Time: %.2f ms", guiReference.ViewportClient->GetPickAccumulatedTimeMs());
			
			// Draw Calls / Culling
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.35f, 1.0f, 0.35f, 1.0f));
			ImGui::SeparatorText("Draw Calls / Culling");
			ImGui::PopStyleColor();
			ImGui::Text("GPU call Count: %llu", guiReference.GraphicsManager->GetRenderer()->GetDrawCallCount());
			ImGui::Text("Drawn Obj Count: %u", guiReference.GraphicsManager->GetRenderCollector().GetDrawnObjCount());
			ImGui::Text("Culled Obj Count: %u", guiReference.GraphicsManager->GetRenderCollector().GetCulledObjCount());
			
			ImGui::PopStyleColor();
			ImGui::PopFont();
			ImGui::End();
		}
	}

#if IS_OBJ_VIEWER
	ConsoleWindow::Get().Process(mBottomBarHeight);
	mContentBrowser.Render(mBottomBarHeight);
#else
	updateControlPanelGUI(guiReference);
	updatePropertyWindowGUI(guiReference);
	// updateObjectListPanelGUI(guiReference);
	ConsoleWindow::Get().Process(mBottomBarHeight);
	mContentBrowser.SetAssetManager(guiReference.AssetManager);
	mContentBrowser.Render(mBottomBarHeight);
#endif
}

void FSceneManager::updateControlPanelGUI(const FGuiReference& guiReference)
{
	ImGuiIO& io = ImGui::GetIO();

	float panelHeight = io.DisplaySize.y * CONTROL_PANEL_HEIGHT_RATIO;

	ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(mPanelWidth, panelHeight), ImGuiCond_FirstUseEver);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
	ImGui::Begin("Jungle Control Panel", nullptr, flags);
	mPanelWidth = ImGui::GetWindowWidth();

	ImGui::Text("Hello Jungle World!");

	/* Spawn Actor */
	// NOTE: This name array must be edited when adding new primitive types to EPrimitive enum.
	ImGui::SeparatorText("Spawn Actor");

	const char* ActorTypeNames[] = {
		"Sphere",
		"Cube",
		"Triangle",
		"GizmoArrow",
		"Circle",
		"SpotLight",
		"Explosion",
		"Apple"
	};

	int32 ActorTypeIndex = static_cast<int32>(mGuiInputField.PrimitiveType);
	int32 SpawnCount = mGuiInputField.SpawnCount;
	if (ImGui::Combo("Actor Type", &ActorTypeIndex, ActorTypeNames, IM_ARRAYSIZE(ActorTypeNames)))
	{
		mGuiInputField.PrimitiveType = static_cast<EPrimitive>(ActorTypeIndex);
	}

	const char* CurrentMeshName = mGuiInputField.SelectedStaticMesh
		? mGuiInputField.SelectedStaticMesh->GetAssetPathFileName().CStr()
		: "None";
	
	if (ImGui::Button("Spawn"))
	{
		for (int32 i = 0; i < mGuiInputField.SpawnCount; ++i)
		{
			const char* ActorTypeName = ActorTypeNames[ActorTypeIndex];

			AActor* NewActor = nullptr;
			if (strcmp(ActorTypeName, "Explosion") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();

				TSharedPtr<FSpriteAtlasAsset> ExplosionAtlas = FAssetManager::Get().GetAssetAs<FSpriteAtlasAsset>(FName("ExplosionSpriteAtlas"));

				UAtlasAnimationComponent* AnimComponent = FObjectFactory::ConstructObject<UAtlasAnimationComponent>(EPrimitive::EP_Plane, ExplosionAtlas);
				AnimComponent->SetRelativeLocation(FVector(0, 0, 0));
				AnimComponent->SetRelativeRotation(FRotator(0, 0, 0));
				AnimComponent->SetRelativeScale3D(FVector(1, 1, 1));
				AnimComponent->SetBillboard(true);
				AnimComponent->SetDepthState(true, false);
				AnimComponent->Play();

				NewActor->AddRootSceneComponent(AnimComponent);
			}
			else if (strcmp(ActorTypeName, "Sphere") == 0 || strcmp(ActorTypeName, "Cube") == 0 || strcmp(ActorTypeName, "Triangle") == 0 || strcmp(ActorTypeName, "GizmoArrow") == 0 || strcmp(ActorTypeName, "Circle") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();

				UStaticMeshComponent* MeshComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));
				MeshComponent->SetMesh(FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(FName(std::format("{}Mesh", ActorTypeName)), true));

				NewActor->AddRootSceneComponent(MeshComponent);
			}
			else if (strcmp(ActorTypeName, "SpotLight") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<ASpotLight>();
			}
			else if (strcmp(ActorTypeName, "StaticMesh") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();

				UStaticMeshComponent* MeshComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));

				NewActor->AddRootSceneComponent(MeshComponent);
			}
			else if (strcmp(ActorTypeName, "Apple") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();

				UStaticMeshComponent* MeshComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));
				MeshComponent->SetMesh(FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(FName("Assets/Meshes/apple_mid.uasset"), true));
				NewActor->AddRootSceneComponent(MeshComponent);
				
			}	
			else
			{
				UE_LOG_ERROR("Unknown actor class: %s", ActorTypeName);
			}

			if (NewActor)
			{
				mCurrentWorld->AddActor(NewActor);
			}
		}
	}
	ImGui::SameLine();
	if (ImGui::InputInt("Number of spawn", &SpawnCount))
	{
		if (SpawnCount < 1)
		{
			SpawnCount = 1;
		}
		mGuiInputField.SpawnCount = SpawnCount;
	}

	/*Scene Control*/
	ImGui::SeparatorText("Scene Control");

	const std::filesystem::path sceneDirectory = std::filesystem::absolute(std::filesystem::path(kDefaultAssetsPath) / std::filesystem::path(kSceneDataDir));

	void* ownerWindow = ImGui::GetMainViewport()->PlatformHandleRaw;

	if (ImGui::Button("New scene"))
	{
		guiReference.ViewportClient->Reset();
		NewScene();
	}

	ImGui::SameLine();

	if (ImGui::Button("Save scene"))
	{
		try
		{
			// 저장 대화상자의 초기 폴더가 반드시 존재하도록 한다.
			//std::filesystem::create_directories(sceneDirectory);

			const std::optional<std::filesystem::path> selectedPath = FNativeFileDialog::SaveScene(sceneDirectory);

			// 취소 버튼을 누른 경우에는 아무 작업도 하지 않는다.
			if (selectedPath.has_value())
			{
				SaveScene(guiReference.EditorCamera, selectedPath.value(), *guiReference.FileManager);

				UE_LOG("Scene saved: %s", selectedPath->string().c_str());
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR(
				"Failed to save scene: %s",
				e.what());
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Load scene"))
	{
		try
		{
			//std::filesystem::create_directories(sceneDirectory);

			const std::optional<std::filesystem::path> selectedPath =
				FNativeFileDialog::OpenScene(sceneDirectory);

			// 취소한 경우에는 현재 씬과 카메라 상태를 건드리지 않는다.
			if (selectedPath.has_value())
			{
				LoadScene(guiReference.EditorCamera, selectedPath.value(), *guiReference.FileManager);

				// 파일 로드가 실행된 뒤에만 카메라를 초기화한다.
				guiReference.ViewportClient->Reset();

				UE_LOG(
					"Scene loaded: %s",
					selectedPath->string().c_str());
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR(
				"Failed to load scene: %s",
				e.what());
		}
	}
	
	ImGui::SeparatorText("Optimization");
	{
		// 배타 선택은 콤보로
		int32 CullingIndex = static_cast<int32>(GCullingMode);
		if (ImGui::Combo("Culling", &CullingIndex, GCullingModeNames, IM_ARRAYSIZE(GCullingModeNames)))
		{
			GCullingMode = static_cast<ECullingMode>(CullingIndex);
		}

		int32 PickingIndex = static_cast<int32>(GPickingMode);
		if (ImGui::Combo("Picking", &PickingIndex, GPickingModeNames, IM_ARRAYSIZE(GPickingModeNames)))
		{
			GPickingMode = static_cast<EPickingMode>(PickingIndex);
		}

		// 항목 추가 시, OptimizationFlags.h의 GOptFlagInfos에 적으면 됨.
		for (const FOptFlagInfo& Info : GOptFlagInfos)
		{
			bool bEnabled = IsOptEnabled(Info.Flag);
			if (ImGui::Checkbox(Info.Name, &bEnabled))
			{
				GOptEnabled[static_cast<uint8>(Info.Flag)] = bEnabled;
			}
			if (ImGui::IsItemHovered())
			{
				ImGui::SetTooltip("%s", Info.Tooltip);
			}
		}

		if (ImGui::Button("All on"))
		{
			GCullingMode = ECullingMode::BVH;
			GPickingMode = EPickingMode::BVHFrustum;
			for (bool& b : GOptEnabled) { b = true; }
		}
		ImGui::SameLine();
		if (ImGui::Button("All Off"))
		{
			GCullingMode = ECullingMode::Off;
			GPickingMode = EPickingMode::BruteForce;
			for (bool& b : GOptEnabled) { b = false; }
		}
	}

	/* Camera Control */
	ImGui::SeparatorText("Camera Control");

	FCamera& camera = guiReference.ViewportClient->GetCamera();
	URenderer* renderer = guiReference.GraphicsManager->GetRenderer();

	if (ImGui::BeginCombo("##ShowFlags", "Show Flags"))
	{
		// 표시 옵션은 표를 그대로 훑어 체크박스를 만든다.
		// 옵션을 추가할 때 ShowFlags.h의 GShowFlagInfos에만 한 줄 적으면 여기 바로 나온다.
		FShowFlags& showFlags = FShowFlags::Get();
		for (const FShowFlagInfo& flagInfo : GShowFlagInfos)
		{
			bool bEnabled = showFlags.IsEnabled(flagInfo.Flag);
			if (ImGui::Checkbox(flagInfo.Name, &bEnabled))
			{
				showFlags.SetEnabled(flagInfo.Flag, bEnabled);
			}
		}

		bool bOrthographic = guiReference.GraphicsManager->IsOrthographicTarget();
		if (ImGui::Checkbox("Orthogonal", &bOrthographic))
		{
			// Preserve the camera and ortho zoom; animate only the projection ratio.
			guiReference.GraphicsManager->StartProjectionTransition(bOrthographic);
		}

		ImGui::EndCombo();
	}
	{
		static constexpr int32 GridGapValues[] = { 1, 5, 10, 50, 100, 500 };
		static constexpr const char* GridGapLabels[] = { "(1)", "(5)", "(10)", "(50)", "(100)", "(500)" };
		constexpr int StepCount = IM_ARRAYSIZE(GridGapValues);
		const int32 GridGap = guiReference.GraphicsManager->GetGridGap();
		int SelectedIndex = 0;
		for (int i = 1; i < StepCount; ++i)
		{
			if (GridGap >= (GridGapValues[i - 1] + GridGapValues[i]) / 2.0f)
			{
				SelectedIndex = i;
			}
		}

		ImGui::Text("Grid Gap: %d", GridGap);
		const ImGuiStyle& Style = ImGui::GetStyle();
		const float FontSize = ImGui::GetFontSize();
		const float LabelWidth = ImGui::CalcTextSize("(500)").x;
		const float Width = (std::max)(ImGui::GetContentRegionAvail().x,
			(LabelWidth + Style.ItemInnerSpacing.x) * StepCount);
		const float Padding = LabelWidth * 0.5f;
		const ImVec2 Origin = ImGui::GetCursorScreenPos();
		const float TrackLeft = Origin.x + Padding;
		const float TrackWidth = Width - Padding * 2.0f;
		const float TrackY = Origin.y + FontSize;
		const float TrackHeight = FontSize * 0.3f;
		const float LabelY = TrackY + TrackHeight + Style.ItemInnerSpacing.y;
		ImGui::InvisibleButton("##GridGapSelector",
			ImVec2(Width, LabelY + FontSize - Origin.y));
		const bool bActive = ImGui::IsItemActive();
		const bool bHovered = ImGui::IsItemHovered();
		bool bChanged = false;
		if (bActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			// Snap to the closest displayed step, including when dragging past either end.
			const float Position = std::clamp((io.MousePos.x - TrackLeft) / TrackWidth, 0.0f, 1.0f);
			SelectedIndex = static_cast<int>(Position * (StepCount - 1) + 0.5f);
			bChanged = true;
		}
		if (bChanged && GridGapValues[SelectedIndex] != GridGap)
		{
			guiReference.GraphicsManager->SetGridGap(GridGapValues[SelectedIndex]);
		}

		if (ImGui::IsItemVisible())
		{
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			const ImU32 TrackColor = ImGui::GetColorU32(bActive ? ImGuiCol_FrameBgActive :
				(bHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));
			const ImU32 HandleColor = ImGui::GetColorU32(bActive ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
			DrawList->AddRectFilled(ImVec2(TrackLeft, TrackY),
				ImVec2(TrackLeft + TrackWidth, TrackY + TrackHeight), TrackColor, Style.FrameRounding);
			for (int i = 0; i < StepCount; ++i)
			{
				const float X = TrackLeft + TrackWidth * i / (StepCount - 1);
				const ImU32 LabelColor = ImGui::GetColorU32(i == SelectedIndex ? ImGuiCol_Text : ImGuiCol_TextDisabled);
				DrawList->AddLine(ImVec2(X, TrackY), ImVec2(X, TrackY + TrackHeight), LabelColor);
				DrawList->AddText(ImVec2(X - ImGui::CalcTextSize(GridGapLabels[i]).x * 0.5f, LabelY),
					LabelColor, GridGapLabels[i]);
			}
			const float HandleX = TrackLeft + TrackWidth * SelectedIndex / (StepCount - 1);
			DrawList->AddTriangleFilled(ImVec2(HandleX - FontSize * 0.4f, Origin.y),
				ImVec2(HandleX + FontSize * 0.4f, Origin.y), ImVec2(HandleX, TrackY + TrackHeight), HandleColor);
		}
	}

	ImGui::Text("FOV     ");
	ImGui::SameLine();
	ImGui::SliderFloat("##FOV", &camera.mFovDegree, 0.0f, 180.0f);

	ImGui::Text("Sensitivity");
	ImGui::SameLine();
	ImGui::SliderFloat("##CameraSensitivity", &camera.Sensitivity, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

	// 1) 라벨 텍스트를 먼저 그리고 같은 줄로
	ImGui::Text("Location");
	ImGui::SameLine();

	// 2) 텍스트를 그린 "뒤"의 남은 폭을 기준으로 계산
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float itemWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f;

	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamLocX", &camera.Transform.Location.x, 0.1f, 10.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamLocY", &camera.Transform.Location.y, 0.1f, 10.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamLocZ", &camera.Transform.Location.z, 0.1f, 10.0f);

	ImGui::Text("Rotation");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamRotX", &camera.Transform.Rotation.Roll, 0.1f, 180.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamRotY", &camera.Transform.Rotation.Pitch, 0.1f, 180.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamRotZ", &camera.Transform.Rotation.Yaw, 0.1f, 180.0f);

	/* Gizmo Control */
	ImGui::SeparatorText("Gizmo Control");

	// Display the current gizmo mode dropdown
	const char* gizmoModeNames[] = { "Translate", "Rotate", "Scale" };

	EGIZMO_TYPE currentGizmoType = guiReference.ViewportClient->mGizmo.GetOperation();
	int32 currentGizmoIndex = static_cast<int32>(currentGizmoType);
	if (ImGui::Combo("Gizmo Mode", &currentGizmoIndex, gizmoModeNames, IM_ARRAYSIZE(gizmoModeNames)))
	{
		if (currentGizmoIndex == 0)
		{
			guiReference.ViewportClient->mGizmo.SetOperation(EGIZMO_TYPE::TRANSLATE);
		}
		else if (currentGizmoIndex == 1)
		{
			guiReference.ViewportClient->mGizmo.SetOperation(EGIZMO_TYPE::ROTATE);
		}
		else if (currentGizmoIndex == 2)
		{
			guiReference.ViewportClient->mGizmo.SetOperation(EGIZMO_TYPE::SCALE);
		}
	}
	if (ImGui::Button("Next Gizmo Mode"))
	{
		guiReference.ViewportClient->mGizmo.SetOperation(static_cast<EGIZMO_TYPE>((currentGizmoIndex + 1) % 3));
	}
	ImGui::End();
}

void FSceneManager::updatePropertyWindowGUI(const FGuiReference& guiReference)
{
	ImGuiIO& io = ImGui::GetIO();

	float controlPanelHeight = io.DisplaySize.y * CONTROL_PANEL_HEIGHT_RATIO;
	float propertyHeight = io.DisplaySize.y * WINDOW_PROPERTY_HEIGHT_RATIO;

	ImGui::SetNextWindowPos(ImVec2(0.0f, controlPanelHeight), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(mPanelWidth, propertyHeight), ImGuiCond_FirstUseEver);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("Jungle Property Window", nullptr, flags);

	mPanelWidth = ImGui::GetWindowWidth();

	if (mSelectedActor)
	{
		// Temporary variables to hold the values for ImGui input fields
		const FTransform& originalTransform = mSelectedActor->GetTransform();

		// Get the current transform of the clicked actor
		FVector translationInput = originalTransform.Location;
		FVector rotationInput = {
			originalTransform.Rotation.Roll,
			originalTransform.Rotation.Pitch,
			originalTransform.Rotation.Yaw
		};
		FVector scaleInput = originalTransform.Scale;

		// Display and edit the transform properties using ImGui input fields
		if (ImGui::DragFloat3("Translation", &translationInput.x, 0.1f))
		{
			mSelectedActor->SetLocation(translationInput);
			for (UActorComponent* Component : mSelectedActor->GetComponents())
			{
				if (UPrimitiveComponent* Primitive = Component->Cast<UPrimitiveComponent>())
				{
					GetCurrentWorld()->GetBVH().Move(Primitive);
				}
			}
		}

		if (ImGui::DragFloat3("Rotation", &rotationInput.x, 0.1f))
		{
			mSelectedActor->SetRotation({
				rotationInput.y, // Pitch
				rotationInput.z, // Yaw
				rotationInput.x  // Roll
				});

			for (UActorComponent* Component : mSelectedActor->GetComponents())
			{
				if (UPrimitiveComponent* Primitive = Component->Cast<UPrimitiveComponent>())
				{
					GetCurrentWorld()->GetBVH().Move(Primitive);
				}
			}
		}

		if (ImGui::DragFloat3("Scale", &scaleInput.x, 0.1f, MIN_SCALE, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			mSelectedActor->SetScale(scaleInput);
			for (UActorComponent* Component : mSelectedActor->GetComponents())
			{
				if (UPrimitiveComponent* Primitive = Component->Cast<UPrimitiveComponent>())
				{
					GetCurrentWorld()->GetBVH().Move(Primitive);
				}
			}
		}

		int componentIndex = 0;
		for (UActorComponent* component : mSelectedActor->GetComponents())
		{
			ImGui::PushID(componentIndex++);

			ImGui::SeparatorText(component->GetClass()->Name.c_str());
						
			if (component->IsA<UText3DComponent>())
			{
				UText3DComponent* text3DComponent = component->Cast<UText3DComponent>();

				char textBuffer[256] = {};
				const FString currentText = Wide2Utf(text3DComponent->GetText());
				strncpy_s(textBuffer, currentText.CStr(), sizeof(textBuffer) - 1);

				if (ImGui::InputText("Display Text", textBuffer, sizeof(textBuffer)))
				{
					text3DComponent->SetText(Utf2Wide(FString(textBuffer)));
				}
			}

			
			else if (component->IsA<USpotLightComponent>())
			{
				USpotLightComponent* spotLightComponent = component->Cast<USpotLightComponent>();

				FVector4 colorInput = spotLightComponent->GetColor();
				if (ImGui::ColorPicker3("Color", &colorInput.x,
					ImGuiColorEditFlags_DisplayRGB | ImGuiColorEditFlags_DisplayHSV | ImGuiColorEditFlags_DisplayHex))
				{
					spotLightComponent->SetColor(colorInput);
				}

				float innerAngleInput = spotLightComponent->GetInnerConeAngle();
				if (ImGui::DragFloat("InnerAngle", &innerAngleInput, 0.1f, 0.f, spotLightComponent->GetOuterConeAngle(), "%.3f", ImGuiSliderFlags_AlwaysClamp))
				{
					spotLightComponent->SetInnerConeAngle(innerAngleInput);
				}

				float outerAngleInput = spotLightComponent->GetOuterConeAngle();
				if (ImGui::DragFloat("OuterAngle", &outerAngleInput, 0.1f, 0.f, 89.f, "%.3f", ImGuiSliderFlags_AlwaysClamp))
				{
					spotLightComponent->SetOuterConeAngle(outerAngleInput);
				}
			}
			else if (component->IsA< UAtlasAnimationComponent>())
			{
				UAtlasAnimationComponent* atlasAnimationComponent = component->Cast<UAtlasAnimationComponent>();

				TArray<FString> spriteAtlasAssetNames;
				guiReference.AssetManager->ForEachMetaInfo([&spriteAtlasAssetNames](const FAssetMetaInfo& metaInfo) {
					if (metaInfo.AssetType != EAssetType::SpriteAtlas)
					{
						return;
					}
					spriteAtlasAssetNames.Add(metaInfo.AssetName.ToString());
					});

				const TSharedPtr<FSpriteAtlasAsset>& currentAtlas = atlasAnimationComponent->GetAtlas();
				FString currentAtlasName = currentAtlas ? currentAtlas->GetAssetName().ToString() : "None";
				if (ImGui::BeginCombo("Sprite Atlas", currentAtlasName.CStr()))
				{
					for (const FString& assetName : spriteAtlasAssetNames)
					{
						bool isSelected = (currentAtlasName == assetName);
						if (ImGui::Selectable(assetName.CStr(), isSelected))
						{
							atlasAnimationComponent->SetAtlas(guiReference.AssetManager->GetAssetAs<FSpriteAtlasAsset>(FName(assetName), true));
						}
						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}
					ImGui::EndCombo();
				}

				if (ImGui::Button("Play"))
				{
					atlasAnimationComponent->Play(0, atlasAnimationComponent->IsLooping(), atlasAnimationComponent->IsBackward());
				}
				ImGui::SameLine();
				if (ImGui::Button("Pause"))
				{
					atlasAnimationComponent->Pause();
				}
				ImGui::SameLine();
				if (ImGui::Button("Resume"))
				{
					atlasAnimationComponent->Resume();
				}
				ImGui::SameLine();
				if (ImGui::Button("Reset"))
				{
					atlasAnimationComponent->Reset();
				}

				ImGui::Text(atlasAnimationComponent->IsPlaying() ? "State: Playing" : "State: Stopped");

				bool loopInput = atlasAnimationComponent->IsLooping();
				if (ImGui::Checkbox("bLoop", &loopInput))
				{
					atlasAnimationComponent->SetLooping(loopInput);
				}

				int32 frameRateInput = atlasAnimationComponent->GetFrameRate();
				if (ImGui::DragInt("FrameRate", &frameRateInput, 1.f, 1, 240, "%d", ImGuiSliderFlags_AlwaysClamp))
				{
					atlasAnimationComponent->SetFrameRate(frameRateInput);
				}
			}
			else if (component->IsA<UStaticMeshComponent>())
			{
				UStaticMeshComponent* StaticMeshComponent = component->Cast<UStaticMeshComponent>();

				TSharedPtr<FStaticMeshAsset> CurrentStaticMesh = StaticMeshComponent->GetMesh();

				TArray<FString> StaticMeshAssetNames;
				TArray<FAssetMetaInfo> materialMetaInfos;
				guiReference.AssetManager->ForEachMetaInfo([&StaticMeshAssetNames, &materialMetaInfos](const FAssetMetaInfo& metaInfo) {
					if (metaInfo.AssetType == EAssetType::StaticMesh)
					{
						StaticMeshAssetNames.Add(metaInfo.AssetName.ToString());
					}
					else if (metaInfo.AssetType == EAssetType::Material)
					{
						materialMetaInfos.Add(metaInfo);
					}
				});

				const FString CurrentMeshPath = CurrentStaticMesh ? CurrentStaticMesh->GetAssetName().ToString() : "None";
				
				// Path에서 확장자 빼고 파일명만 parsing 하여 보여주기
				std::filesystem::path meshPath = std::filesystem::path(static_cast<std::string>(CurrentMeshPath)).stem();
				FString simpleMeshName = meshPath.stem().string();

				if (ImGui::BeginCombo("Static Mesh", simpleMeshName.CStr()))
				{
					for (const FString& assetName : StaticMeshAssetNames)
					{
						bool isSelected = (CurrentMeshPath == assetName);
						
						// Path에서 확장자 빼고 파일명만 parsing 하여 보여주기
						std::filesystem::path assetPath = std::filesystem::path(static_cast<std::string>(assetName)).stem();
						FString simpleAssetName = assetPath.stem().string();

						FString UniqueLabel = simpleAssetName.ToString() + "##" + assetName.ToString();

						if (ImGui::Selectable(UniqueLabel.c_str(), isSelected))
						{
							StaticMeshComponent->SetMesh(guiReference.AssetManager->GetAssetAs<FStaticMeshAsset>(FName(assetName), true));
						}
						if (ImGui::IsItemHovered())
						{
							ImGui::SetTooltip("%s", assetName.CStr());
						}	

						if (isSelected)
						{
							ImGui::SetItemDefaultFocus();
						}
					}

					ImGui::EndCombo();
				}

				if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("ASSET_GUID_MESH"))
					{
						const FGuid* DataGuid = static_cast<const FGuid*>(Payload->Data);
						FAssetMetaInfo LoadedMetaInfo = guiReference.AssetManager->GetMetaInfo(*DataGuid);
						if (LoadedMetaInfo.AssetType == EAssetType::StaticMesh)
						{

							TSharedPtr<FStaticMeshAsset> MatchedMeshAsset = guiReference.AssetManager->GetAssetAs<FStaticMeshAsset>(*DataGuid, true);

							if (MatchedMeshAsset != nullptr)
							{
								StaticMeshComponent->SetMesh(MatchedMeshAsset);
								UE_LOG("Success: StaticMesh applied: %s", MatchedMeshAsset->GetAssetName().ToString().c_str());
							}
							else
							{
								UE_LOG_ERROR("Failed to load StaticMesh Guid: %s", DataGuid->ToString().c_str());
							}
						}
					}
					ImGui::EndDragDropTarget();
				}

				// 현재 가진 Material이 있으면 그것을, 없으면 None을 콤보박스 이름으로
				const auto& Materials = StaticMeshComponent->GetMaterials();
				for (int32 i = 0; i < Materials.Num(); i++)
				{
					ImGui::PushID(i);

					TSharedPtr<FMaterialAsset> currentMaterial = Materials[i];
					FString currentMaterialName = currentMaterial ? currentMaterial->GetAssetName().ToString() : "None";
					
					// Path에서 확장자 빼고 파일명만 parsing 하여 보여주기
					std::filesystem::path materialPath = std::filesystem::path(static_cast<std::string>(currentMaterialName)).stem();
					FString simpleMaterialName = materialPath.stem().string();
					
					if (ImGui::BeginCombo("Material", simpleMaterialName.CStr()))
					{						
						for (const FAssetMetaInfo& metaInfo : materialMetaInfos)
						{
							// Path에서 확장자 빼고 파일명만 parsing 하여 보여주기
							std::filesystem::path metaPath = std::filesystem::path(metaInfo.AssetName.ToString().ToString()).stem();
							FString simpleMetaPath = metaPath.stem().string();

							bool isSelected = (currentMaterialName == metaInfo.AssetName.ToString());
							FString UniqueLabel = simpleMetaPath.ToString() + "##" + metaInfo.AssetID.ToString().ToString();

							if (ImGui::Selectable(UniqueLabel.CStr(), isSelected))
							{
								TSharedPtr<FMaterialAsset> materialAsset =
									guiReference.AssetManager->GetAssetAs<FMaterialAsset>(metaInfo.AssetID, true);
								StaticMeshComponent->SetMaterial(i, materialAsset);
							}

							// 호버링 중에 전체 경로 툴팁 뜨도록
							if (ImGui::IsItemHovered())
							{
								ImGui::SetTooltip("%s", metaInfo.AssetName.ToString().CStr());   
							}

							if (isSelected) ImGui::SetItemDefaultFocus();
						}
						ImGui::EndCombo();
					}

					if (ImGui::BeginDragDropTarget())
					{
						if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload("ASSET_GUID_MATERIAL"))
						{
							const FGuid* DataGuid = static_cast<const FGuid*>(Payload->Data);
							FAssetMetaInfo LoadedMetaInfo = guiReference.AssetManager->GetMetaInfo(*DataGuid);
							if (LoadedMetaInfo.AssetType == EAssetType::Material)
							{
								TSharedPtr<FMaterialAsset> MatchedMaterialAsset = guiReference.AssetManager->GetAssetAs<FMaterialAsset>(*DataGuid, true);

								if (MatchedMaterialAsset != nullptr)
								{
									StaticMeshComponent->SetMaterial(i, MatchedMaterialAsset);
									UE_LOG("Success: StaticMesh applied: %s", MatchedMaterialAsset->GetAssetName().ToString().c_str());
								}
								else
								{
									UE_LOG_ERROR("Failed to load StaticMesh Guid: %s", DataGuid->ToString().c_str());
								}
							}
						}
						ImGui::EndDragDropTarget();
					}

					FVector2 UVOffset = StaticMeshComponent->GetUVOffset(i);
					if (ImGui::DragFloat2("UV Offset", &UVOffset.X, 0.01f))
					{
						StaticMeshComponent->SetUVOffset(i, UVOffset);
					}
					ImGui::PopID();
				}

				/*if (ImGui::BeginDragDropTarget())
				{
					if (const ImGuiPayload* Payload = ImGui::AcceptDragDropPayload(AssetPayloadTags::StaticMesh))
					{

					}
				}*/
			}
			ImGui::PopID();

		}
	}
	ImGui::End();
}

void FSceneManager::updateObjectListPanelGUI(const FGuiReference& guiReference)
{
	ImGuiIO& io = ImGui::GetIO();

	float offsetHeight = io.DisplaySize.y * (CONTROL_PANEL_HEIGHT_RATIO + WINDOW_PROPERTY_HEIGHT_RATIO);
	float objectListPanelHeight = io.DisplaySize.y - offsetHeight;

	ImGui::SetNextWindowPos(ImVec2(0.0f, offsetHeight), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSize(ImVec2(mPanelWidth, objectListPanelHeight), ImGuiCond_FirstUseEver);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;

	ImGui::Begin("Object List Panel", nullptr, flags);
	{
		/* Object Lists */
		ImGui::SeparatorText("Object Lists");
		if (ImGui::BeginChild("ObjectList", ImVec2(0, 0),
			ImGuiChildFlags_Borders))
		{
			if (mGuiInputField.LastGUObjectRevision != UObject::GetGObjectRevision())
			{
				mGuiInputField.SortedObjectLists = UObject::GetGObjectArray().ToTArray();
				mGuiInputField.LastGUObjectRevision = UObject::GetGObjectRevision();

				// Sort the objects by UUID
				std::sort(mGuiInputField.SortedObjectLists.begin(), mGuiInputField.SortedObjectLists.end(),
					[](UObject* a, UObject* b) { return a->UUID < b->UUID; });
			}

			int32 selectedActorUUID = mSelectedActor
				? mSelectedActor->UUID
				: -1;

			// Todo: rbegin()
			//for (UObject* object : mGuiInputField.SortedObjectLists)

			UObject* bDeleteActorOrNull = nullptr;
			for (unsigned int objectsIndex = 0; objectsIndex < mGuiInputField.SortedObjectLists.Num(); ++objectsIndex)
			{
				UObject* object = mGuiInputField.SortedObjectLists[objectsIndex];

				bool bSelected = false;
				ImGui::PushID(object->UUID); // Ensure unique ID for each child

				// Highlight the frame if this object is the clicked actor
				if (object->UUID == selectedActorUUID)
				{
					bSelected = true;
					ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(255, 255, 0, 50)); // Light yellow background
				}

				if (ImGui::BeginChild("ObjectFrame", ImVec2(0, 0), ImGuiChildFlags_FrameStyle | ImGuiChildFlags_AutoResizeY))
				{
					ImGui::Text("Class: %s", object->GetClass()->Name.CStr());
					ImGui::Text("UUID: %d", object->UUID);

					// TODO: Move implement delete to where?
					if (object->IsA<AActor>())
					{
						AActor* actor = object->Cast<AActor>();

						if (ImGui::Button("Select"))
						{
							SetSelectedActor(actor);
						}
						else
						{
							ImGui::SameLine();
							if (ImGui::Button("Delete"))
							{
								bDeleteActorOrNull = object;
							}
						}


					}
				}
				ImGui::EndChild();

				if (bSelected)
				{
					ImGui::PopStyleColor(); // Pop the border color if it was pushed
				}


				ImGui::PopID();
			}

			if (bDeleteActorOrNull != nullptr)
			{
				AActor* deleteActor = bDeleteActorOrNull->Cast<AActor>();

				if (mSelectedActor != nullptr && mSelectedActor->UUID == deleteActor->UUID)
				{
					mSelectedActor = nullptr;
				}

				assert(mCurrentWorld != nullptr);
				mCurrentWorld->RemoveActor(deleteActor->UUID);

				FObjectFactory::DestroyObject(deleteActor);
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();
}


void FSceneManager::NewScene()
{
	if (mCurrentWorld != nullptr)
	{
		FObjectFactory::DestroyObject(mCurrentWorld);
	}

	//UEngineStatics::SetNextUUID(0);
	ResetSelectedActor();
	mCurrentWorld = FObjectFactory::ConstructObject<UWorld>();
}

void FSceneManager::DeleteScene()
{
	if (mCurrentWorld != nullptr)
	{
		FObjectFactory::DestroyObject(mCurrentWorld);
		mCurrentWorld = nullptr;
	}
	ResetSelectedActor();
}

void FSceneManager::SaveScene(FCamera* Camera, const std::filesystem::path& scenePath, const FFileManager& fileManager)
{
	if (mCurrentWorld == nullptr)
	{
		throw std::runtime_error("Cannot save scene because current world is null.");
	}

	uint32 version = 0;

	// 기존 파일이 있으면 Version을 유지한다.
	try
	{
		const FString previousSceneString =
			fileManager.ReadFileToString(scenePath);

		const json::JSON previousSceneJson =
			json::JSON::Load(previousSceneString);

		if (previousSceneJson.hasKey("Version") &&
			previousSceneJson.at("Version").JSONType() ==
			json::JSON::Class::Integral)
		{
			version =
				previousSceneJson.at("Version").ToInt();
		}
	}
	catch (const std::exception&)
	{
		// 새로 저장하는 파일이면 Version 0부터 시작한다.
		version = 0;
	}

	json::JSON sceneJson =
		json::JSON::Make(json::JSON::Class::Object);

	json::JSON worldJson =
		json::JSON::Make(json::JSON::Class::Object);

	mCurrentWorld->SerializeClass(worldJson);

	sceneJson["Version"] = version;
	sceneJson["NextUUID"] = UEngineStatics::GetNextUUID();
	sceneJson["World"] = worldJson;

	json::JSON& PerspectiveCameraJson = sceneJson["PerspectiveCamera"];
	PerspectiveCameraJson["Location"] = JsonUtils::ToJson(Camera->Transform.Location);
	PerspectiveCameraJson["Rotation"] = JsonUtils::ToJson(Camera->Transform.Rotation);
	PerspectiveCameraJson["FOV"] = Camera->mFovDegree;
	PerspectiveCameraJson["Near"] = Camera->mNear;
	PerspectiveCameraJson["Far"] = Camera->mFar;

	const FString jsonString(sceneJson.dump(1, "  "));

	fileManager.WriteStringToFile(
		scenePath,
		jsonString);
}

void FSceneManager::LoadScene(FCamera* Camera, const std::filesystem::path& scenePath, const FFileManager& fileManager)
{
	const FString jsonString = fileManager.ReadFileToString(scenePath);

	const json::JSON sceneJson = json::JSON::Load(jsonString);

	if (!sceneJson.hasKey("NextUUID") || sceneJson.at("NextUUID").JSONType() != json::JSON::Class::Integral)
	{
		throw std::runtime_error(std::format("Scene file '{}' does not contain valid NextUUID data.", scenePath.string()));
	}
		
	if (sceneJson.hasKey("World"))
	{
		const uint32 nextUUID = sceneJson.at("NextUUID").ToInt();
		UEngineStatics::SetNextUUID(nextUUID);
		
		const json::JSON worldJson = sceneJson.at("World");
		
		UWorld* newWorld = FObjectFactory::LoadObject<UWorld>(worldJson);
		json::JSON PerspectiveCameraJson = sceneJson.at("PerspectiveCamera");
		Camera->Transform.Location = JsonUtils::FromJson<FVector>(PerspectiveCameraJson.at("Location"));
		Camera->Transform.Rotation = JsonUtils::FromJson<FRotator>(PerspectiveCameraJson.at("Rotation"));
		Camera->mFovDegree = PerspectiveCameraJson.at("FOV").ToFloat();
		Camera->mNear = PerspectiveCameraJson.at("Near").ToFloat();
		Camera->mFar = PerspectiveCameraJson.at("Far").ToFloat();
		
		if (newWorld == nullptr)
		{
			throw std::runtime_error(std::format("Failed to deserialize world from '{}'.", scenePath.string()));
		}

		// 새 월드 생성이 성공한 경우에만 기존 월드를 교체한다.
		FObjectFactory::DestroyObject(mCurrentWorld);
		mCurrentWorld = newWorld;
		
		ResetSelectedActor();

	}
	else if (sceneJson.hasKey("Primitives"))
	{
		uint32 nextUUID = 0;
		UWorld* newWorld = FSceneConverter::BuildWorldFromJson(sceneJson, nextUUID);
		
		FSceneConverter::ApplyCamera(sceneJson, Camera);
		UEngineStatics::SetNextUUID(nextUUID);

		if (newWorld == nullptr)
		{
			throw std::runtime_error(std::format("Failed to deserialize world from '{}'.", scenePath.string()));
		}
		
		// 새 월드 생성이 성공한 경우에만 기존 월드를 교체한다.
		FObjectFactory::DestroyObject(mCurrentWorld);
		mCurrentWorld = newWorld;
		
		ResetSelectedActor();
	}
	else
	{
		throw std::runtime_error(std::format("Failed to deserialize world from '{}'.", scenePath.string()));
	}
}

void  FSceneManager::SetSelectedActor(AActor* actor)
{
	if (actor == nullptr)
	{
		UE_LOG_WARN("SetSelectedActor: Attempted to set selected actor to nullptr.");
		return;
	}

	if (actor == mSelectedActor)
	{
		UE_LOG_WARN("SetSelectedActor: Actor with UUID %d is already selected.", actor->UUID);
		return; // No change
	}

	UE_LOG_WARN("SetSelectedActor: Actor with UUID %d is now selected.", actor->UUID);
	mSelectedActor = actor;
}

float FSceneManager::GetPanelWidth() const
{
	return mPanelWidth;
}

const TArray<FRenderInfo> FSceneManager::GetAxisRenderInfos()
{
	// TODO: Implement axis render info retrieval logic
	return TArray<FRenderInfo>();
}
