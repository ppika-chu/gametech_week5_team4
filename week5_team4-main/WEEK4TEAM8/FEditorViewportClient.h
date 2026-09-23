#pragma once

#include "Vector.h"
#include <d3d11.h>
#include "World.h"
#include "Camera.h"
#include "RenderInfo.h"
#include "Gizmo.h"
#include "Enum.h"

class AActor;
class FSceneManager;
class URenderer;
struct FRenderTarget2D;
struct FDepthStencil;


struct SWindow
{
	FRect Rect;

	SWindow() : Rect() {}

	virtual void SetRect(const FRect& InRect)
	{
		Rect = InRect;
		Rect.X = FMath::Max(Rect.X, 0.0f);
		Rect.Y = FMath::Max(Rect.Y, 0.0f);
		Rect.Width = FMath::Max(Rect.Width, 0.0f);
		Rect.Height = FMath::Max(Rect.Height, 0.0f);
	}
};

struct SSplitter : public SWindow
{
	TSharedPtr<SWindow> SideLT; // 좌 또는 상
	TSharedPtr<SWindow> SideRB; // 우 또는 하
	float SplitterRatio;

	SSplitter() 
	{ 
		SplitterRatio = 0.5f;
	}
};

struct SSplitterH : public SSplitter
{
	void SetRect(const FRect& InRect) override
	{
		SWindow::SetRect(InRect);
		if (SideLT && SideRB)
		{
			float OtherRatio = 1.0f - SplitterRatio;
			SideLT->SetRect(FRect(Rect.X, Rect.Y, Rect.Width, Rect.Height * SplitterRatio));
			SideRB->SetRect(FRect(Rect.X, Rect.Y + Rect.Height * SplitterRatio, Rect.Width, Rect.Height * OtherRatio));
		}
	}
};

class SSplitterV : public SSplitter
{
	void SetRect(const FRect& InRect) override
	{
		SWindow::SetRect(InRect);
		if (SideLT && SideRB)
		{
			float OtherRatio = 1.0f - SplitterRatio;
			SideLT->SetRect(FRect(Rect.X, Rect.Y, Rect.Width * SplitterRatio, Rect.Height));
			SideRB->SetRect(FRect(Rect.X + Rect.Width * SplitterRatio, Rect.Y, Rect.Width * OtherRatio, Rect.Height));
		}
	}
};

struct FEditorViewportClient
{
public:
	FEditorViewportClient(URenderer& InRenderer);

	// 이번 프레임에 수집된 픽킹 대상(RenderCollector.PickTargets)만 훑는다.
	// 월드의 액터 계층을 다시 내려가지 않는다.
	// 광선은 ImGui 뷰포트 이미지 기준으로 만든다. 렌더러의 D3D11_VIEWPORT(백버퍼 전체)가 아니다.
	AActor* PerformMousePicking(const FRect& ViewportRect, float perspectiveRatio, const FRenderCollector& RenderCollector);
	float GetFov() const { return mCamera.mFovDegree; }
	void Update(float deltaTime, float perspectiveRatio, FRenderCollector& RenderCollector);

	void Reset();

	const EViewModeIndex GetViewMode() const { return mViewModeIndex; }
	void SetViewMode(EViewModeIndex InViewMode) { mViewModeIndex = InViewMode; }

	const EViewportType GetViewportType() const { return mViewportType; }
	void SetViewportType(EViewportType InViewportType);
	bool IsOrtho() { return mViewportType != EViewportType::Perspective; }

	inline void SetActive(bool bActive) { mbActive = bActive; }
	inline bool IsActive() const { return mbActive; }

	FCamera& GetCamera() { return mCamera; }

	FCamera mCamera;
	FGizmo mGizmo;

private:
	// 선택된 액터의 RenderInfo는 캐시하지 않는다. 필요할 때 ClickedActor->GetRenderInfos()로 그때그때 뽑는다.
	//마우스 밑 무언가가 Actor이면 저장. RayCast 에서 채워야 함 (아직 미구현)
	// INFO: mClickedActor moved to FSceneManager::mSelectedActor.
	//AActor* mClickedActor = nullptr;

	void DeprojectScreenToWorld(int32 MouseX, int32 MouseY,
		float ScreenW, float ScreenH, float NearZ, float FarZ,
		FVector& OutNearPoint, FVector& OutFarPoint);

	void DeprojectScreenToWorldForOrtho(int32 MouseX, int32 MouseY,
		float ScreenW, float ScreenH, float NearZ, float FarZ,
		FVector& OutNearPoint, FVector& OutFarPoint);

	void DeprojectScreenToWorldForUnified(int32 MouseX, int32 MouseY,
		float ScreenW, float ScreenH, float NearZ, float FarZ,
		float orthoDistance, float perspectiveRatio,
		FVector& OutNearPoint, FVector& OutFarPoint
	);

	bool mbActive;
	
	// RayCast가 이번 프레임에 쏜 광선. 기즈모 드래그가 같은 광선을 다시 쓴다
	FVector mRayNear;
	FVector mRayFar;

	FTransform PrevCameraTransform = mCamera.Transform;

	EViewModeIndex mViewModeIndex = EViewModeIndex::VMI_Lit;
	EViewportType mViewportType = EViewportType::Perspective;
};

struct FViewport
{
	TSharedPtr<FRenderTarget2D> RenderTarget;
	TSharedPtr<FDepthStencil> DepthStencil;

	void Resize(URenderer& Renderer, uint32 Width, uint32 Height);
};

