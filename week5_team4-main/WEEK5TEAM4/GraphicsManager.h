#pragma once

#include "Matrix.h"
#include "Enum.h"

#include "TArray.h"
#include "TMap.h"
#include "Renderer.h"
#include "Camera.h"
#include "RenderInfo.h"
#include "Vector.h"
#include "ShowFlags.h"
#include "HZBOcclusion.h"

class FAssetManager;
struct FViewport;

struct FBuffer
{
	ID3D11Buffer* Buffer;
	uint32 SourceNum;
};

class FGraphicsManager
{
public:
	FGraphicsManager(HWND hWindow);
	~FGraphicsManager();

	//void Prepare(const Camera* mCamera);
	void Prepare(const FCamera* Camera,float viewportWidth, float viewportHeight, const FViewport& viewport, const EViewModeIndex InViewMode, const EViewportType InViewportType);

	void RenderHighLight(const TArray<UPrimitiveComponent*>& Primitives);
	void Render();

	void Display();

	float GetAspect() const { return mAspect; }
	EViewModeIndex GetViewModeIndex() const { return mViewModeIndex; }
	void SetViewModeIndex(EViewModeIndex viewModeIndex) { mViewModeIndex = viewModeIndex; }

	bool IsPerspectiveProjection() const;
	void SetPerspectiveProjection(bool bPerspectiveProjection);

	float GetPerspectiveRatio() const { return mProjectionRatio; }
	const FMatrix& GetViewProjectionMatrix() const { return mViewUnifiedProjectionMatrix; }
	void SetPerspectiveRatio(float ratio) { mProjectionRatio = FMath::Clamp(ratio, 0.0f, 1.0f); }

	float GetCameraOrthoDistance() const { return mCameraOrthoDistance; }
	void SetCameraOrthoDistance(float distance) { mCameraOrthoDistance = distance; }

	// Todo: Change name
	URenderer* GetRenderer() const;
	void OnResize(UINT width, UINT height);

	// Projection ratio smoothing
	void StartProjectionTransition(bool orthographic);
	bool IsOrthographicTarget() const;
	void UpdateProjectionTransition(float deltaTime);

	inline FRenderCollector& GetRenderCollector() { return mRenderCollector; }
	inline TArray<FRenderInfo>& GetRenderInfos() { return mRenderCollector.RenderInfos; }

	inline int32 GetGridGap() { return GridGap; }
	void SetGridGap(int32 GridGap);

	struct FGpuTimerQuerySet
	{
		Microsoft::WRL::ComPtr<ID3D11Query> Disjoint;
		Microsoft::WRL::ComPtr<ID3D11Query> Begin;
		Microsoft::WRL::ComPtr<ID3D11Query> End;
		bool bIssued = false;
	};

	TArray<FGpuTimerQuerySet> GpuQueries;
	uint32 GpuQueryIndex = 0;

	float GpuRenderTime = 0.0f;
	void BeginGpuRenderTimer();
	void EndGpuRenderTimer();
	void UpdateGpuRenderTime();
	float GetGpuRenderTime() { return GpuRenderTime; }

private:
	struct FOutlineConstants
	{
		FVector4 OutlineColor;
		int32 StencilTexWidth;
		int32 StencilTexHeight;
		int32 OutlineRadius;
		int32 Padding;
	};

	URenderer* mRenderer;
	FMatrix mViewMatrix;
	FMatrix mProjectionMatrix;
	FMatrix mViewProjectionMatrix;
	FMatrix mViewOrthogonalProjectionMatrix;
	FMatrix mViewUnifiedProjectionMatrix;

	// Prepare에서 갱신. 하이라이트 두께의 픽셀 → 월드 환산에 쓴다
	FVector mCameraLocation;
	FVector mCameraForward;
	float mCameraFovDegree = 60.0f;
	float mCameraOrthoDistance = 10.0f;

	EViewModeIndex mViewModeIndex = EViewModeIndex::VMI_Lit;
	EViewportType mViewportType = EViewportType::Perspective;
	bool mbPerspectiveProjection;
	float mAspect;
	float mProjectionRatio; // 0.0f ~ 1.0f, 0이면 직교, 1이면 원근, 그 사이면 혼합

	// Projection ratio smoothing
	float mProjectionStartRatio = 1.0f;
	float mProjectionTargetRatio = 1.0f;
	float mProjectionElapsed = 0.0f;
	float mProjectionDuration = 1.0f;
	bool mbProjectionTransitioning = false;

	TSharedPtr<FRenderPipeline> mMeshPipeline;

	TSharedPtr<FRenderPipeline> mHighlightMarkPipeline;
	TSharedPtr<FRenderPipeline> mHighlightDrawPipeline;
	TSharedPtr<FVertexBuffer> mHighlightVertexBuffer;
	TSharedPtr<FIndexBuffer> mHighlightIndexBuffer;
	FTexture2DAsset* mLastBoundTexture = nullptr;

	FRenderCollector mRenderCollector;

	int32 GridGap = 1;
	bool bGpuTimerActive = false;

	// ---- HZB 오클루전 ----
public:
	// 뷰포트의 씬 렌더링이 끝난 뒤 호출: 깊이를 줄여 스테이징으로 복사하고, 준비된 이전 결과를 CPU 피라미드로 만든다.
	void UpdateHZB(const FViewport& Viewport);
	// CPU 판정에 쓸 수 있는 HZB가 있으면 반환 (없으면 nullptr → 오클루전 판정 안 함)
	const FHZB* GetHZB() const { return mHZB.bValid ? &mHZB : nullptr; }
	// 뷰포트 구성이 바뀌는 등 이전 깊이를 믿을 수 없을 때 호출
	void InvalidateHZB();

private:
	struct FHZBReadbackSlot
	{
		Microsoft::WRL::ComPtr<ID3D11Texture2D> Staging;
		FMatrix ViewProjection = FMatrix::Identity;
		bool bPending = false;
	};
	static constexpr int32 HZBReadbackSlotCount = 3;   // GPU 결과를 기다리지 않도록 2~3프레임 전 결과를 읽는다

	TSharedPtr<FRenderPipeline> mHZBPipeline;
	TSharedPtr<FRenderTarget2D> mHZBTarget;           // 4x4 max로 줄인 깊이 (R32_FLOAT)
	uint32 mHZBWidth = 0;
	uint32 mHZBHeight = 0;
	FHZBReadbackSlot mHZBSlots[HZBReadbackSlotCount];
	int32 mHZBWriteIndex = 0;
	FHZB mHZB;
};
