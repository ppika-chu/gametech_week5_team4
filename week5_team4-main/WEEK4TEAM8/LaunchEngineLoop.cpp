#include "LaunchEngineLoop.h"

#include <windows.h>
#include "Renderer.h"
#include "WindowApplication.h"
#include "Console.h"
#include "GraphicsManager.h"
#include "CubeComponent.h"
#include "ObjectFactory.h"
#include "Cube.h"
#include "Sphere.h"
#include "Circle.h"
#include "Triangle.h"
#include "Plane.h"
#include "Object.h"
#include "GizmoArrow.h"
#include "ImGui/imgui.h"
#include "ImGui/imgui_impl_dx11.h"
#include "imGui/imgui_impl_win32.h"
#include "Actor.h"
#include "World.h"
#include <FLogManager.h>
#include "Assets.h"
#include "FMeshDescription.h"
#include "FStaticMeshBuilder.h"
#include "FObjImporter.h"
#include "UStaticMeshComponent.h"
#include "Serializers.h"
#include "NativeFileDialog.h"

#if IS_OBJ_VIEWER
#include "FObjViewer.h"
#endif

void FEngineLoop::Init(HINSTANCE hInstance, WNDPROC WndProc)
{
	// Initialize window infos
	WCHAR WindowClass[] = L"JungleWindowClass";
	WCHAR Title[] = L"Game Tech Lab";
	WNDCLASSW wndclass = { 0, WndProc, 0, 0, 0, 0, 0, 0, 0, WindowClass };
	RegisterClassW(&wndclass);

	HWND hWnd = CreateWindowExW(
		0,
		WindowClass,
		Title,
		WS_VISIBLE | WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, 600, 1024,
		nullptr, nullptr, hInstance, nullptr
	);

	// 창을 화면 크기에 맞게 최대화하여 표시
	ShowWindow(hWnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hWnd);

	// 최대화된 후의 실제 클라이언트 크기를 구해 콘솔에 전달
	RECT clientRect;
	GetClientRect(hWnd, &clientRect);
	int clientWidth = clientRect.right - clientRect.left;
	int clientHeight = clientRect.bottom - clientRect.top;

	RAWINPUTDEVICE rid = {};
	rid.usUsagePage = 0x01;		// Generic Desktop
	rid.usUsage = 0x02;			// Mouse
	rid.dwFlags = 0;		// 포커스 있을 때만 수신
	rid.hwndTarget = hWnd;
	RegisterRawInputDevices(&rid, 1, sizeof(rid));

	mGraphicsManager = new FGraphicsManager(hWnd);
	FNativeFileDialog::Initialize(hWnd);

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGui_ImplWin32_Init((void*)hWnd);
	ImGui_ImplDX11_Init(mGraphicsManager->GetRenderer()->GetDevice(), mGraphicsManager->GetRenderer()->GetDeviceContext());
	auto& IO = ImGui::GetIO();
	IO.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	IO.Fonts->AddFontFromFileTTF(
		"C:/Windows/Fonts/malgun.ttf",
		18.0f,
		nullptr,
		IO.Fonts->GetGlyphRangesKorean()
	);

	/* Console Window */
	ConsoleWindow& console = ConsoleWindow::Get();
	console.Init(clientWidth);

	FrameTimer = new FFrameTimer(120);

	mEditorLayout.Initialize(FRect(0, 0, (float)clientWidth, (float)clientHeight));

	static constexpr EViewportType DefaultLayoutTypes[FEngineLoop::MaxViewportCount] = {
		EViewportType::Perspective,
		EViewportType::Top,
		EViewportType::Front,
		EViewportType::Side
	};
	
	for (int32 i = 0; i < 4; ++i)
	{
		mViewports[i].Window = (i == MainViewportIndex) ? mEditorLayout.RootWindow : mEditorLayout.ViewportWindows[i];
		mViewports[i].Viewport = MakeShared<FViewport>();
		mViewports[i].Viewport->Resize(*mGraphicsManager->GetRenderer(), mEditorLayout.ViewportWindows[i]->Rect.Width, mEditorLayout.ViewportWindows[i]->Rect.Height);
		mViewports[i].Client = MakeShared<FEditorViewportClient>(*mGraphicsManager->GetRenderer());
		mViewports[i].Client->SetViewportType(DefaultLayoutTypes[i]);
	}

	const FVector4 NearTint(1.0f, 0.65f, 0.15f, 0.85f); // 주황 = 가까운 쪽
	const FVector4 FarTint(0.25f, 0.55f, 1.0f, 0.85f); // 파랑 = 먼 쪽

	mSceneManager = new FSceneManager();
	mFileManager = new FFileManager();
	mFontManager = new FFontManager();
	InitAssetManager();

	mComponentVisualizerManager = new FComponentVisualizerManager();

	mSceneManager->NewScene();
#ifdef IS_OBJ_VIEWER
	mObjViewer.Initialize(*mSceneManager, *mGraphicsManager->GetRenderer(), *mFileManager);
#else
#endif
	LoadEditorSettings();
}

void FEngineLoop::InitAssetManager()
{
	mAssetManager = new FAssetManager();

	URenderer* renderer = mGraphicsManager->GetRenderer();

	FAssetManager::Get().ScanDirectory("BuiltInAssets", *renderer);
	FAssetManager::Get().ScanDirectory("Assets", *renderer);

	// Register built-in asset types
	TSharedPtr<FStaticMeshAsset> cubeAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::CubeMesh, FName("CubeMesh"), *renderer, Cube_vertices, sizeof(Cube_vertices) / sizeof(FVertex), Cube_indices, sizeof(Cube_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(cubeAsset);

	TSharedPtr<FStaticMeshAsset> sphereAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::SphereMesh, FName("SphereMesh"), *renderer, Sphere_vertices, sizeof(Sphere_vertices) / sizeof(FVertex), Sphere_indices, sizeof(Sphere_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(sphereAsset);

	TSharedPtr<FStaticMeshAsset> circleAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::CircleMesh, FName("CircleMesh"), *renderer, Circle_vertices, sizeof(Circle_vertices) / sizeof(FVertex), Circle_indices, sizeof(Circle_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(circleAsset);

	TSharedPtr<FStaticMeshAsset> triangleAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::TriangleMesh, FName("TriangleMesh"), *renderer, Triangle_vertices, sizeof(Triangle_vertices) / sizeof(FVertex), Triangle_indices, sizeof(Triangle_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(triangleAsset);

	TSharedPtr<FStaticMeshAsset> gizmoArrowAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::GizmoArrowMesh, FName("GizmoArrowMesh"), *renderer, GizmoArrow_vertices, sizeof(GizmoArrow_vertices) / sizeof(FVertex), GizmoArrow_indices, sizeof(GizmoArrow_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(gizmoArrowAsset);

	TSharedPtr<FStaticMeshAsset> PlaneAsset = MakeShared<FStaticMeshAsset>(BuiltInAssetID::PlaneMesh, FName("PlaneMesh"), *renderer, Plane_vertices, sizeof(Plane_vertices) / sizeof(FVertex), Plane_indices, sizeof(Plane_indices) / sizeof(uint32));
	mAssetManager->RegisterAsset(PlaneAsset);

	TSharedPtr<FTexture2DAssetLoader> TextureLoader = MakeShared<FTexture2DAssetLoader>(*renderer);
	TSharedPtr<FFontAssetLoader> FontLoader = MakeShared<FFontAssetLoader>(*mFontManager);

	{
		// NOTE: 이 부분은 임시로 ExplosionSpriteAtlas를 고정 Guid로 등록하는 코드이므로, 후에 스프라이트 아틀라스 에셋을 만드는 기능이 나오면 제거해야할 코드임.
		TSharedPtr<FTexture2DAsset> ExplosionTexture2DAsset = mAssetManager->GetAssetAs<FTexture2DAsset>(BuiltInAssetID::ExplosionTexture, true);
		if (ExplosionTexture2DAsset)
		{
			TSharedPtr<FSpriteAtlasAsset> ExplosionSpriteAtlasAsset = MakeShared<FSpriteAtlasAsset>(BuiltInAssetID::ExplosionSpriteAtlas, FName("ExplosionSpriteAtlas"), *renderer, ExplosionTexture2DAsset, 6, 6);
			mAssetManager->RegisterAsset(ExplosionSpriteAtlasAsset);
		}
	}

	// ScanDirectory로 파일 자동 스캔하여 uasset 등록하므로 아래 줄과 중복되어 삭제해도 되나,
	// 참고하고 있는 곳이 있어서 ScanDirectory와 동일한 파일명 규칙으로 수정해 둠.

	TSharedPtr<FFileAssetSource> FontAssetSource = MakeShared<FFileAssetSource>("Assets/Fonts/BMKkubulimTTF.ttf");
	mAssetManager->RegisterAsset(FGuid::NewGuid(), FName("TestFont"), FontLoader, FontAssetSource);
	
	TSharedPtr<FFontAsset> TestFontAsset = mAssetManager->GetAssetAs<FFontAsset>(FName("TestFont"), true);
	TSharedPtr<FFontAtlasAsset> FontAtlasAsset = MakeShared<FFontAtlasAsset>(FGuid::NewGuid(), FName("TestFontAtlas"), *renderer, TestFontAsset, 512, 512, 2, 2);
	mAssetManager->RegisterAsset(FontAtlasAsset);
}

void FEngineLoop::Tick(bool bPumpMessages)
{
	if (GInTick) return;
	GInTick = true;

	FrameTimer->StartFrame();
	float deltaTime = FrameTimer->GetDeltaTime();

	FRenderCollector& RenderCollector = mGraphicsManager->GetRenderCollector();

	WindowApplication.ProcessDeferredEvents();

	if (WindowApplication.bPendingResize)
	{
		mGraphicsManager->OnResize(WindowApplication.PendingWidth, WindowApplication.PendingHeight);
		WindowApplication.bPendingResize = false;
	}

	mSceneManager->Tick(deltaTime);

	mGraphicsManager->UpdateProjectionTransition(deltaTime);

	const float NearZ = 0.1f;
	const float FarZ = 2000.0f;
	const bool bIsSplit = mEditorLayout.bIsSplitView;
	const int32 ActiveIndex = mEditorLayout.MaximizedViewportIndex;
	const int32 ViewportCount = mEditorLayout.bIsSplitView ? 4 : 1;

	if (bIsSplit)
	{
		for (int32 i = 0;i < 4;++i)
			mViewports[i].Window = mEditorLayout.ViewportWindows[i];
	}
	else
		mViewports[ActiveIndex].Window = mEditorLayout.RootWindow;
	

	mGraphicsManager->UpdateGpuRenderTime();
	if (ConsoleWindow::Get().bShowStatRender)
	{
		mGraphicsManager->BeginGpuRenderTimer();
	}
	
	mGraphicsManager->GetRenderer()->ResetDrawCallCount();
	
	for (int32 i = 0; i < ViewportCount; ++i)
	{
		int32 CurrentIndex = bIsSplit ? i : ActiveIndex;
		FEditorViewport* CurrentViewport = &mViewports[CurrentIndex];

		const FRect& ViewportRect = CurrentViewport->Window->Rect;

		FCamera& Camera = CurrentViewport->Client->GetCamera();
		Camera.mAspect = ViewportRect.Width / ViewportRect.Height;
		Camera.mNear = NearZ;
		Camera.mFar = FarZ;

		RenderCollector.Clear();
		RenderCollector.Camera = &Camera;

		bool bIsOrtho = CurrentViewport->Client->IsOrtho();
		float CurrentRatio = bIsOrtho ? 0.0f : mGraphicsManager->GetPerspectiveRatio();

		// 뷰포트가 직교 타입이면 현재 -5000 ~ 5000으로 보이게 하드코딩, 나중에 카메라 위치에 따라 랜더 거리를 늘려야 함
		Camera.mNear = bIsOrtho ? -5000.0f : Camera.mNear;
		Camera.mFar = bIsOrtho ? 5000.0f : Camera.mFar;

		CurrentViewport->Client->Update(deltaTime, CurrentRatio, RenderCollector);

		FMatrix ViewProjection = Camera.GetViewMatrix() * Camera.GetUnifiedProjectionMatrix(Camera.mOrthoDistance, CurrentRatio);
		FMatrix InvViewProjection = Camera.GetInverseUnifiedProjectionMatrix(Camera.mOrthoDistance, CurrentRatio) * Camera.GetViewMatrix().AffineInverse();

		mSceneManager->Render(deltaTime, RenderCollector);

		// 마우스 피킹 처리
		// 뷰포트가 ImGui 창이 되면서 그 위에서는 io.WantCaptureMouse 가 항상 true 다.
		// 그대로 두면 씬을 클릭해도 선택이 되지 않는다. 카메라/기즈모와 같은 기준을 쓴다.
		const FInputState& Input = WindowApplication.Input;

		// 드래그 중 마우스 피킹이 실행되어 선택된 액터가 풀리는 것 방지
		bool bIsAssetDragging = (ImGui::GetDragDropPayload != nullptr);

		if (CurrentViewport->Client->IsActive() && Input.WasPressed(VK_LBUTTON) && !CurrentViewport->Client->mGizmo.IsDragging() && !CurrentViewport->Client->mGizmo.IsMouseOverHandle() && bIsAssetDragging)
		{
			AActor* HitActor = CurrentViewport->Client->PerformMousePicking(CurrentViewport->Window->Rect, CurrentRatio, RenderCollector);
			if (HitActor)
			{
				mSceneManager->SetSelectedActor(HitActor);
			}
			else
			{
				mSceneManager->ResetSelectedActor();
			}
		}

		// 선택된 액터 처리
		TArray<UPrimitiveComponent*> HighlightedComponents;

		AActor* SelectedActor = mSceneManager->GetSelectedActor();
		if (SelectedActor)
		{
			FTransform Transform = SelectedActor->GetTransform();

			for (UActorComponent* Component : SelectedActor->GetComponents())
			{
				UPrimitiveComponent* PrimitiveComponent = Component->Cast<UPrimitiveComponent>();
				if (PrimitiveComponent)
				{
					// 선택된 액터의 AABB를 화면에 표시
					const FAABB& AABB = PrimitiveComponent->GetBoundingBox();

					AABB.ForEachCornerLines([&RenderCollector](const FVector& Start, const FVector& End) {
						FVector4 WorldStart = FVector4(Start, 1.f);
						FVector4 WorldEnd = FVector4(End, 1.f);

						FRenderLineInfo LineInfo;
						LineInfo.Start = WorldStart.ToVec3();
						LineInfo.End = WorldEnd.ToVec3();
						LineInfo.Color = FVector4(1.f, 0.f, 0.f, 1.f); // 빨간색
						LineInfo.Thickness = 5.0f;

						RenderCollector.LineInfos.Add(LineInfo);
					});

					HighlightedComponents.Add(PrimitiveComponent);
				}

				// 선택된 액터의 컴포넌트 시각화
				FComponentVisualizer* Visualizer = mComponentVisualizerManager->FindVisualizer(Component->GetClass());
				if (Visualizer)
				{
					Visualizer->VisualizeComponent(Component, RenderCollector);
				}
			}

			CurrentViewport->Client->mGizmo.Tick(SelectedActor, CurrentViewport->Window->Rect, CurrentViewport->Client->IsActive(), InvViewProjection);
		}

		//Render Threads
		{
			CurrentViewport->Viewport->Resize(*mGraphicsManager->GetRenderer(), ViewportRect.Width, ViewportRect.Height);
			mGraphicsManager->Prepare(&CurrentViewport->Client->mCamera, ViewportRect.Width, ViewportRect.Height, *CurrentViewport->Viewport, CurrentViewport->Client->GetViewMode(), CurrentViewport->Client->GetViewportType());
			mGraphicsManager->RenderHighLight(HighlightedComponents);
			mGraphicsManager->Render();

			CurrentViewport->Client->mGizmo.Render(SelectedActor, CurrentViewport->Client->mCamera.Transform.Location, CurrentViewport->Window->Rect, ViewProjection, CurrentViewport->Client->IsOrtho(), CurrentViewport->Client->GetCamera().mOrthoDistance);
		}
	}

	mGraphicsManager->EndGpuRenderTimer();

	FGuiReference GuiReference;
	GuiReference.FrameTimer = FrameTimer;
	GuiReference.GraphicsManager = mGraphicsManager;
	GuiReference.FileManager = mFileManager;
	GuiReference.AssetManager = mAssetManager;
	GuiReference.EditorLayout = &mEditorLayout;
	if (bIsSplit)
	{
		GuiReference.EditorCamera = &GetMainViewport().Client->GetCamera();
		GuiReference.ViewportClient = GetMainViewport().Client.get();
		GuiReference.Viewports = mViewports;
		GuiReference.ViewportCount = MaxViewportCount;
	}
	else
	{
		GuiReference.EditorCamera = &mViewports[MainViewportIndex].Client->GetCamera();
		GuiReference.ViewportClient = mViewports[ActiveIndex].Client.get();
		GuiReference.Viewports = &mViewports[ActiveIndex];
		GuiReference.ViewportCount = 1;
	}

	mSceneManager->UpdateGUI(GuiReference);
#if IS_OBJ_VIEWER
	mObjViewer.UpdateObjGUI(*mGraphicsManager);
#endif
	// 나중에 Ui 매니저에서 관리하도록 분리 필요
	ImGui::SetMouseCursor(mMouseCursor);

	FRect ViewportRect;
	ViewportRect.X = mSceneManager->GetViewportX();
	ViewportRect.Y = mSceneManager->GetViewportY();
	ViewportRect.Width = mSceneManager->GetViewportWidth();
	ViewportRect.Height = mSceneManager->GetViewportHeight();
	mEditorLayout.Resize(ViewportRect);

	mGraphicsManager->GetRenderer()->BindFrameBuffer();

	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	mGraphicsManager->Display();
	
	FrameTimer->EndFrame();

	GInTick = false;
}

void FEngineLoop::End()
{
	SaveEditorSettings();

	mSceneManager->DeleteScene();

	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	for (FEditorViewport& Viewport : mViewports)
	{
		Viewport.Release();
	}

	delete mComponentVisualizerManager;
	delete FrameTimer;
	delete mSceneManager;
	delete mFileManager;
	delete mAssetManager;
	delete mFontManager;

	delete mGraphicsManager;
}

void FEngineLoop::SaveEditorSettings()
{
	// Save camera sensitivity
	const std::string Value = std::format("{:.6f}", GetMainViewport().Client->GetCamera().Sensitivity);
	if (!WritePrivateProfileStringA("Camera", "Sensitivity", Value.c_str(), ".\\editor.ini"))
	{
		UE_LOG_ERROR("Failed to save camera sensitivity to editor.ini");
	}

	// Save grid gap
	const std::string ValueGrid = std::format("{:6d}", mGraphicsManager->GetGridGap());
	if (!WritePrivateProfileStringA("Grid", "Gap", ValueGrid.c_str(), ".\\editor.ini"))
	{
		UE_LOG_ERROR("Failed to save grid gap to editor.ini");
	}

	// Save split infos
	const std::string ValueH = std::format("{:.6f}", mEditorLayout.HSplitter->SplitterRatio);
	if (!WritePrivateProfileStringA("Split", "HorizontalRatio", ValueH.c_str(), ".\\editor.ini"))
	{
		UE_LOG_ERROR("Failed to save horizontal split ratio to editor.ini");
	}

	const std::string ValueV = std::format("{:.6f}", mEditorLayout.VSplitter[0]->SplitterRatio);
	if (!WritePrivateProfileStringA("Split", "VerticalRatio", ValueV.c_str(), ".\\editor.ini"))
	{
		UE_LOG_ERROR("Failed to save vertical split ratio to editor.ini");
	}

	WritePrivateProfileStringA("Layout", "IsSplitView", mEditorLayout.bIsSplitView ? "1" : "0", ".\\editor.ini");
	WritePrivateProfileStringA("Layout", "MaximizedIndex", std::to_string(mEditorLayout.MaximizedViewportIndex).c_str(), ".\\editor.ini");

	for (int32 i = 0; i < MaxViewportCount; ++i)
	{
		std::string ViewTypeKey = "ViewportType_" + std::to_string(i);
		std::string ViewTypeVal = std::to_string(static_cast<int32>(mViewports[i].Client->GetViewportType()));
		WritePrivateProfileStringA("Layout", ViewTypeKey.c_str(), ViewTypeVal.c_str(), ".\\editor.ini");

		std::string ViewModeKey = "ViewportViewMode_" + std::to_string(i);
		std::string ViewModeVal = std::to_string(static_cast<int32>(mViewports[i].Client->GetViewMode()));
		WritePrivateProfileStringA("Layout", ViewModeKey.c_str(), ViewModeVal.c_str(), ".\\editor.ini");
	}
}

void FEngineLoop::LoadEditorSettings()
{
	char Value[64] = {};

	// Load camera sensitivity
	GetPrivateProfileStringA("Camera", "Sensitivity", "", Value, sizeof(Value), ".\\editor.ini");

	float Sensitivity = 1.0f;
	sscanf_s(Value, "%f", &Sensitivity);
	GetMainViewport().Client->GetCamera().Sensitivity = Sensitivity;

	// Load grid gap
	GetPrivateProfileStringA("Grid", "Gap", "", Value, sizeof(Value), ".\\editor.ini");

	int32 GridGap = 1;
	sscanf_s(Value, "%d", &GridGap);
	mGraphicsManager->SetGridGap(GridGap);

	// Load split infos
	GetPrivateProfileStringA("Split", "HorizontalRatio", "", Value, sizeof(Value), ".\\editor.ini");

	float HorizontalRatio = 0.5f;
	sscanf_s(Value, "%f", &HorizontalRatio);

	GetPrivateProfileStringA("Split", "VerticalRatio", "", Value, sizeof(Value), ".\\editor.ini");

	float VerticalRatio = 0.5f;
	sscanf_s(Value, "%f", &VerticalRatio);
	
	mEditorLayout.SetSplitRatios(HorizontalRatio, VerticalRatio);

	mEditorLayout.bIsSplitView = (GetPrivateProfileIntA("Layout", "IsSplitView", 0, ".\\editor.ini") == 1);
	mEditorLayout.MaximizedViewportIndex = GetPrivateProfileIntA("Layout", "MaximizedIndex", 0, ".\\editor.ini");

	for (int32 i = 0; i < MaxViewportCount; ++i)
	{
		std::string ViewTypeKey = "ViewportType_" + std::to_string(i);
		int32 ViewTypeVal = GetPrivateProfileIntA("Layout", ViewTypeKey.c_str(), -1, ".\\editor.ini");
		if (ViewTypeVal >= 0 && ViewTypeVal < static_cast<int32>(EViewportType::Max))
		{
			mViewports[i].Client->SetViewportType(static_cast<EViewportType>(ViewTypeVal));
		}

		std::string ViewModeKey = "ViewportViewMode_" + std::to_string(i);
		int32 ViewModeVal = GetPrivateProfileIntA("Layout", ViewModeKey.c_str(), -1, ".\\editor.ini");
		if (ViewModeVal >= 0 && ViewModeVal < static_cast<int32>(EViewModeIndex::VMI_Max))
		{
			mViewports[i].Client->SetViewMode(static_cast<EViewModeIndex>(ViewModeVal));
		}
	}
}
