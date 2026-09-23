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

	FRenderCollector mRenderCollector;

	int32 GridGap = 1;
	bool bGpuTimerActive = false;
};
