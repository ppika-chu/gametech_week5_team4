#include "GraphicsManager.h"
#include "Renderer.h"
#include "Camera.h"
#include "Console.h"
#include "FLogManager.h"
#include "FAssetManager.h"
#include "Assets.h"
#include "ObjectFactory.h"
#include "UTextComponent.h"
#include "FEditorViewportClient.h"

// 선분 하나당 정점 2개. 축 6개 + 앞으로 붙을 그리드까지 감당할 만큼 잡아둔다
static constexpr uint32 LINE_VERTEX_CAPACITY = 8192;

FGraphicsManager::FGraphicsManager(HWND hWindow) :
	mbPerspectiveProjection(true)
	, mProjectionRatio(1.0f)
{
	mRenderer = new URenderer;
	mRenderer->Create(hWindow);

	mAspect = mRenderer->GetWidth() / static_cast<float>(mRenderer->GetHeight());

	mMeshPipeline = mRenderer->CreateRenderPipeline();
	mMeshPipeline->SetRasterRizerState(D3D11_CULL_BACK, 0, { EViewModeIndex::VMI_Lit, EViewModeIndex::VMI_Wireframe });
	mMeshPipeline->SetDepthStencilState(true, true);
	mMeshPipeline->SetShader("Assets/Shaders/StaticMeshShader.hlsl");
	mMeshPipeline->AddConstantBuffer<FConstants>();
	mMeshPipeline->AddConstantBuffer<FMatrix>();

	mHighlightMarkPipeline = mRenderer->CreateRenderPipeline();
	mHighlightMarkPipeline->SetRasterRizerState(D3D11_CULL_BACK);
	mHighlightMarkPipeline->SetDepthStencilState(true, false, D3D11_COMPARISON_ALWAYS, D3D11_STENCIL_OP_REPLACE);
	mHighlightMarkPipeline->SetBlendState(ERenderBlendMode::Opaque, false);
	mHighlightMarkPipeline->SetShader("Assets/Shaders/StaticMeshShader.hlsl");
	mHighlightMarkPipeline->AddConstantBuffer<FConstants>();
	mHighlightMarkPipeline->AddConstantBuffer<FMatrix>();
	mHighlightMarkPipeline->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

	mHighlightDrawPipeline = mRenderer->CreateRenderPipeline();
	mHighlightDrawPipeline->SetRasterRizerState(D3D11_CULL_BACK);
	mHighlightDrawPipeline->SetDepthStencilState(false, false, D3D11_COMPARISON_NOT_EQUAL, D3D11_STENCIL_OP_KEEP);
	mHighlightDrawPipeline->SetShader("Assets/Shaders/Outline.hlsl");
	mHighlightDrawPipeline->AddConstantBuffer<FOutlineConstants>();

	mHighlightVertexBuffer = mRenderer->CreateVertexBuffer<FVertex>(nullptr, 1024, D3D11_USAGE_DYNAMIC); // 초기 용량 1024개, 필요하면 늘어난다
	mHighlightIndexBuffer = mRenderer->CreateIndexBuffer(nullptr, 1024, D3D11_USAGE_DYNAMIC); // 초기 용량 1024개, 필요하면 늘어난다
	
	GpuQueries.SetNum(3);

	for (FGpuTimerQuerySet& QuerySet : GpuQueries)
	{
		D3D11_QUERY_DESC QueryDesc{};
		QueryDesc.Query = D3D11_QUERY_TIMESTAMP_DISJOINT;
		mRenderer->GetDevice()->CreateQuery(&QueryDesc, &QuerySet.Disjoint);

		QueryDesc.Query = D3D11_QUERY_TIMESTAMP;
		mRenderer->GetDevice()->CreateQuery(&QueryDesc, &QuerySet.Begin);
		mRenderer->GetDevice()->CreateQuery(&QueryDesc, &QuerySet.End);
	}
}

FGraphicsManager::~FGraphicsManager()
{
	mHighlightVertexBuffer.reset();
	mHighlightIndexBuffer.reset();
	mHighlightMarkPipeline.reset();
	mHighlightDrawPipeline.reset();
	mMeshPipeline.reset();
	mRenderCollector.Clear();
	mRenderer->Release();
	delete mRenderer;
}

void FGraphicsManager::Prepare(const FCamera* mCamera, float viewportWidth, float viewportHeight, const FViewport& Viewport, const EViewModeIndex InViewMode, const EViewportType InViewportType)
{
	mViewportType = InViewportType;
	const bool bIsOrtho = (InViewportType != EViewportType::Perspective);

	float d = mCamera->mOrthoDistance;
	mAspect = viewportWidth / viewportHeight;

	FMatrix view = mCamera->GetViewMatrix();
	FMatrix projection_u_p = mCamera->GetUnifiedProjectionMatrix(d, 1.0f);
	FMatrix projection_u_o = mCamera->GetUnifiedProjectionMatrix(d, 0.0f);
	FMatrix projection_u = mCamera->GetUnifiedProjectionMatrix(d, bIsOrtho ? 0.0f : mProjectionRatio);

	//mViewProjectionMatrix = view * mCamera->GetProjectionMatrix(mAspect, mCamera->mFovDegree, nearZ, farZ);
	mViewMatrix = view;
	mProjectionMatrix = projection_u;
	mViewProjectionMatrix = view * projection_u_p;

	// 뷰 모드를 렌더러에 전달한다. BindPipeline이 드로우마다 이 값을 보고
	// 솔리드/와이어프레임 래스터라이저를 고른다.
	mViewModeIndex = InViewMode;
	mRenderer->SetViewModeIndex(mViewModeIndex);

	mRenderer->Prepare(view * projection_u);

	float orthoHeight = mCamera->mOrthoHeight;
	float orthoWidth = orthoHeight * mAspect;
	//mViewOrthogonalProjectionMatrix = view * mCamera->GetOrthographicMatrix(orthoWidth, orthoHeight, nearZ, farZ);
	mViewOrthogonalProjectionMatrix = view * projection_u_o;
	mViewUnifiedProjectionMatrix = view * projection_u;

	// 하이라이트 두께를 화면 픽셀 기준으로 환산할 때 쓴다
	mCameraLocation = mCamera->Transform.Location;
	mCameraForward = mCamera->GetForwardVector();
	mCameraFovDegree = mCamera->mFovDegree;
	mCameraOrthoDistance = mCamera->mOrthoDistance;

	// 그리는 순서가 중요하다: 가까운 것을 먼저, 먼 것을 나중에.
	// 깊이 테스트가 켜져 있으면 나중에 그린 FarCube 가 깊이 비교에서 탈락해
	// NearCube(주황)가 앞에 남고, 꺼져 있으면 FarCube(파랑)가 그 위를 덮어쓴다.
	//mRenderer->UpdateConstantViewProjection(viewProjection);

	mRenderer->BindRenderTarget(Viewport.RenderTarget, Viewport.DepthStencil);
}

void FGraphicsManager::RenderHighLight(const TArray<UPrimitiveComponent*>& Primitives)
{
	if (Primitives.Num() == 0)
	{
		return;
	}

	TSharedPtr<FRenderTarget2D> CurrentRenderTarget = mRenderer->GetBindedRenderTarget();
	TSharedPtr<FDepthStencil> CurrentDepthStencil = mRenderer->GetBindedDepthStencil();

	if (CurrentDepthStencil == nullptr)
	{
		UE_DEBUG_LOG_WARN("RenderHighLight: CurrentDepthStencil is nullptr. Skipping highlight rendering.");
		return;
	}

	mHighlightMarkPipeline->UpdateConstantBuffer(1, mViewUnifiedProjectionMatrix);
	mHighlightDrawPipeline->UpdateConstantBuffer(1, mViewUnifiedProjectionMatrix);

	// Mark Pass: 스텐실에 마크만 찍는다.
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		const TArray<FVertex>& Vertices = Primitive->GetMeshVertices();
		const TArray<uint32>& Indices = Primitive->GetMeshIndices();

		if (Vertices.Num() * sizeof(FVertex) > mHighlightVertexBuffer->GetBufferSize())
		{
			mHighlightVertexBuffer = mRenderer->CreateVertexBuffer<FVertex>(Vertices.Data(), Vertices.Num(), D3D11_USAGE_DYNAMIC);
		}

		if (Indices.Num() * sizeof(uint32) > mHighlightIndexBuffer->GetBufferSize())
		{
			mHighlightIndexBuffer = mRenderer->CreateIndexBuffer(Indices.Data(), Indices.Num(), D3D11_USAGE_DYNAMIC);
		}

		mHighlightVertexBuffer->UpdateBuffer(Vertices.Data(), Vertices.Num());
		mHighlightIndexBuffer->UpdateBuffer(Indices.Data(), Indices.Num());

		FTransform Transform = Primitive->GetTransformMatrix();

		FConstants Constants{};
		Constants.Matrix = Transform.MakeMatrix();
		Constants.Color = FVector4(0.f, 0.f, 0.f, 0.f);
		Constants.HasTexture = 0;
		Constants.UseVertexColor = 0;
		Constants.UVOffset = FVector2(0.f, 0.f);

		mHighlightMarkPipeline->UpdateConstantBuffer(0, Constants);

		FRenderInfo RenderInfo{};
		RenderInfo.VertexBuffer = mHighlightVertexBuffer->Buffer;
		RenderInfo.VertexCount = static_cast<uint32>(Vertices.Num());
		RenderInfo.IndexBuffer = mHighlightIndexBuffer->Buffer;
		RenderInfo.StartIndex = 0;
		RenderInfo.IndexCount = static_cast<uint32>(Indices.Num());
		RenderInfo.Model = Primitive->GetTransformMatrix().MakeMatrix();

		mRenderer->RenderPrimitiveIndexed(mHighlightMarkPipeline, RenderInfo, 1);
	}

	// Draw Pass: 잠시 DepthStencil을 해제
	mRenderer->BindRenderTarget(CurrentRenderTarget, nullptr, false);
	mHighlightDrawPipeline->SetShaderResource(0, CurrentDepthStencil->SRV);

	// Draw Pass: 스텐실에 마크가 찍힌 영역만 그린다.
	FOutlineConstants OutlineConstants{};
	OutlineConstants.OutlineColor = FVector4(1.f, 0.6f, 0.f, 1.f);
	OutlineConstants.StencilTexWidth = CurrentDepthStencil->Width;
	OutlineConstants.StencilTexHeight = CurrentDepthStencil->Height;
	OutlineConstants.OutlineRadius = 5;
	
	mHighlightDrawPipeline->UpdateConstantBuffer(0, OutlineConstants);

	mRenderer->Render(mHighlightDrawPipeline, 6);

	// Draw Pass가 끝나면 원래 DepthStencil을 복원한다.
	mRenderer->ClearAllShaderResources();
	mRenderer->BindRenderTarget(CurrentRenderTarget, CurrentDepthStencil, false);
}

void FGraphicsManager::Render()
{
	mRenderer->RenderLines(mRenderCollector.LineInfos);

	for (const FRenderInfo& RenderInfo : mRenderCollector.RenderInfos)
	{
		if (RenderInfo.Texture)
		{
			mMeshPipeline->ClearShaderResource();
			mMeshPipeline->ClearSamplerState();

			FConstants Constants{};
			Constants.Matrix = RenderInfo.Model;
			Constants.Color = RenderInfo.Color;
			Constants.UseVertexColor = RenderInfo.UseVertexColor;
			Constants.HasTexture = RenderInfo.Texture ? 1 : 0;
			Constants.UVOffset = RenderInfo.UVOffset;

			mMeshPipeline->UpdateConstantBuffer(0, Constants);
			mMeshPipeline->UpdateConstantBuffer(1, mViewUnifiedProjectionMatrix);

			mMeshPipeline->SetShaderResource(0, RenderInfo.Texture->GetSRV());
			mMeshPipeline->SetSamplerState(0, D3D11_FILTER_MIN_MAG_MIP_LINEAR, D3D11_TEXTURE_ADDRESS_WRAP, D3D11_TEXTURE_ADDRESS_WRAP);

			mRenderer->RenderPrimitiveIndexed(mMeshPipeline, RenderInfo);
		}
		else
		{
			mRenderer->RenderPrimitiveIndexed(RenderInfo);
		}
	}

	for (const FRenderQuadInfo& QuadInfo : mRenderCollector.GetOpaqueQuadInfos())
	{
		mRenderer->RenderQuad(QuadInfo);
	}

	if (FShowFlags::Get().IsEnabled(EShowFlag::Grid))
	{
		FMatrix GridWorldMatrix = FMatrix::Identity;

		if (mViewportType == EViewportType::Front)
		{
			GridWorldMatrix = FMatrix::RotateY(90);
		}
		else if (mViewportType == EViewportType::Side)
		{
			GridWorldMatrix = FMatrix::RotateX(90);
		}
		// Match the grid's world-space half-width of 0.001.
		mRenderer->RenderWorldAxis(mViewMatrix, mProjectionMatrix, FVector4(0.f, 0.f, 1.f, 1.f), FVector3(0.f, 0.f, 1.f), 0.002f);
		mRenderer->RenderWorldGrid(GridWorldMatrix * mViewUnifiedProjectionMatrix, mCameraLocation, GridGap);
	}

	for (const FRenderQuadInfo& QuadInfo : mRenderCollector.GetTransparentQuadInfos())
	{
		mRenderer->RenderQuad(QuadInfo);
	}

	for (const FRenderQuadInfo& QuadInfo : mRenderCollector.GetOverlayQuadInfos())
	{
		mRenderer->RenderQuad(QuadInfo);
	}

	for (const FRenderQuad2DInfo& Quad2DInfo : mRenderCollector.GetQuad2DInfos())
	{
		mRenderer->RenderQuad2D(Quad2DInfo);
	}
}

void FGraphicsManager::Display()
{
	mRenderer->SwapBuffer();
}

bool FGraphicsManager::IsPerspectiveProjection() const
{
	return mbPerspectiveProjection;
}

void FGraphicsManager::SetPerspectiveProjection(bool bPerspectiveProjection)
{
	mbPerspectiveProjection = bPerspectiveProjection;
}

URenderer* FGraphicsManager::GetRenderer() const
{
	assert(mRenderer != nullptr);

	return mRenderer;
}

void FGraphicsManager::OnResize(UINT width, UINT height)
{
	if (width == 0 || height == 0)
	{
		return;
	}

	mRenderer->OnResize(width, height);
}

// 월드 공간 반지름이 worldHalfExtent인 축을 worldThickness 만큼 키우는 배율
static float GetOutlineAxisScale(float worldHalfExtent, float worldThickness)
{
	if (worldHalfExtent <= SMALL_NUMBER)
	{
		return 1.0f;   // 납작하게 눌린 축은 건드리지 않는다. 안 그러면 배율이 발산한다
	}

	return 1.0f + worldThickness / worldHalfExtent;
}

void FGraphicsManager::StartProjectionTransition(bool orthographic)
{
	mProjectionStartRatio = mProjectionRatio;
	mProjectionTargetRatio = orthographic ? 0.0f : 1.0f;
	mProjectionElapsed = 0.0f;

	mbProjectionTransitioning = mProjectionStartRatio != mProjectionTargetRatio;
}

bool FGraphicsManager::IsOrthographicTarget() const
{
	return mProjectionTargetRatio == 0.0f;
}

void FGraphicsManager::UpdateProjectionTransition(float deltaTime)
{
	if (!mbProjectionTransitioning)
	{
		return;
	}

	mProjectionElapsed += deltaTime;

	const float u = FMath::Clamp(
		mProjectionElapsed / mProjectionDuration, 0.0f, 1.0f);

	// Smoothstep interpolation for a smoother transition
	const float blend = u * u * (3.0f - 2.0f * u);

	mProjectionRatio = mProjectionStartRatio + (mProjectionTargetRatio - mProjectionStartRatio) * blend;

	if (u >= 1.0f)
	{
		mProjectionRatio = mProjectionTargetRatio;
		mbProjectionTransitioning = false;
	}
}

void FGraphicsManager::SetGridGap(int32 GridGap)
{
	if (GridGap > 75000)
		GridGap = 100000;
	else if (GridGap > 30000)
		GridGap = 50000;
	else if (GridGap > 7500)
		GridGap = 10000;
	else if (GridGap > 3000)
		GridGap = 5000;
	else if (GridGap > 750)
		GridGap = 1000;
	else if (GridGap > 300)
		GridGap = 500;
	else if (GridGap > 75)
		GridGap = 100;
	else if (GridGap > 30)
		GridGap = 50;
	else if (GridGap > 7)
		GridGap = 10;
	else if (GridGap > 3)
		GridGap = 5;
	else
		GridGap = 1;
	this->GridGap = GridGap;
}

void FGraphicsManager::BeginGpuRenderTimer()
{
	bGpuTimerActive = false;

	ID3D11DeviceContext* Context = mRenderer->GetDeviceContext();
	FGpuTimerQuerySet& QuerySet = GpuQueries[GpuQueryIndex];

	if (QuerySet.bIssued)
	{
		return; // 이전 결과 미회수
	}

	Context->Begin(QuerySet.Disjoint.Get());
	Context->End(QuerySet.Begin.Get());

	bGpuTimerActive = true;
}

void FGraphicsManager::EndGpuRenderTimer()
{
	if (!bGpuTimerActive)
	{
		return;
	}

	ID3D11DeviceContext* Context = mRenderer->GetDeviceContext();
	FGpuTimerQuerySet& QuerySet = GpuQueries[GpuQueryIndex];

	Context->End(QuerySet.End.Get());
	Context->End(QuerySet.Disjoint.Get());

	QuerySet.bIssued = true;

	GpuQueryIndex = (GpuQueryIndex + 1) % GpuQueries.Num();

	bGpuTimerActive = false;
}

void FGraphicsManager::UpdateGpuRenderTime()
{
	ID3D11DeviceContext* Context = mRenderer->GetDeviceContext();
	FGpuTimerQuerySet& QuerySet = GpuQueries[GpuQueryIndex];

	if (!QuerySet.bIssued)
	{
		return;
	}

	D3D11_QUERY_DATA_TIMESTAMP_DISJOINT Disjoint{};
	UINT64 BeginTimestamp = 0;
	UINT64 EndTimestamp = 0;

	if (Context->GetData(QuerySet.Disjoint.Get(), &Disjoint, sizeof(Disjoint), D3D11_ASYNC_GETDATA_DONOTFLUSH) != S_OK)
	{
		return; 
	}

	if (Context->GetData(QuerySet.Disjoint.Get(), &Disjoint, sizeof(Disjoint), 0) != S_OK ||
		Context->GetData(QuerySet.Begin.Get(), &BeginTimestamp, sizeof(BeginTimestamp), 0) != S_OK ||
		Context->GetData(QuerySet.End.Get(), &EndTimestamp, sizeof(EndTimestamp), 0) != S_OK)
	{
		return; // 아직 GPU가 해당 프레임을 끝내지 않음
	}

	if (!Disjoint.Disjoint)
	{
		GpuRenderTime =
			static_cast<float>(EndTimestamp - BeginTimestamp) * 1000.0f /
			static_cast<float>(Disjoint.Frequency);
	}

	QuerySet.bIssued = false;
}