#include "UAtlasAnimationComponent.h"
#include <cmath>

UAtlasAnimationComponent::UAtlasAnimationComponent()
{
	mBlendMode = ERenderBlendMode::Additive;
}

void UAtlasAnimationComponent::Initialize(EPrimitive PrimitiveType, const TSharedPtr<FSpriteAtlasAsset>& textureAsset)
{
	UPrimitiveComponent::Initialize(PrimitiveType);

	SetAtlas(textureAsset);
}

void UAtlasAnimationComponent::SetAtlas(const TSharedPtr<FSpriteAtlasAsset>& InAtlas)
{
	Asset = InAtlas;
	SetTexture(InAtlas);

	Frame = 0;
	FrameAccumulator = 0.f;
	mSubUV = InAtlas ? InAtlas->GetFrameSubUV(0) : FVector4(0.f, 0.f, 1.f, 1.f);
}

void UAtlasAnimationComponent::Play(int32 StartFrame, bool bIsLooping, bool bBackwardAnimate)
{
	bPlaying = true;
	bLooping = bIsLooping;
	Frame = StartFrame;
	bBackward = bBackwardAnimate;
}

void UAtlasAnimationComponent::Pause()
{
	bPlaying = false;
}

void UAtlasAnimationComponent::Resume()
{
	bPlaying = true;
}

void UAtlasAnimationComponent::Reset()
{
	Frame = 0;
	mSubUV = Asset->GetFrameSubUV(0);
	FrameAccumulator = 0.f;
	Pause();
}

void UAtlasAnimationComponent::Tick(float deltaTime)
{
	Super::Tick(deltaTime);

	if (!bPlaying || !Asset)
	{
		return;
	}

	const int32 FrameCount = Asset->GetFrameCount();
	if (FrameCount <= 0)
	{
		return;
	}

	FrameAccumulator += deltaTime * FrameRate;

	//첫 줄: int32로 캐스팅하면 소수점이 잘립니다. 1.2 → 1. 지금 넘길 수 있는 온전한 프레임 수입니다.
	//둘째 줄 : 방금 쓴 만큼을 빼서 소수부만 남깁니다. 1.2 - 1 = 0.2.이 0.2가 다음 틱으로 이월됩니다.
	const int32 Advance = static_cast<int32>(FrameAccumulator);
	FrameAccumulator -= static_cast<float>(Advance);

	if (Advance > 0)
	{
		const int32 NextFrame = Frame + (bBackward ? -Advance : Advance);

		if (!bLooping && (NextFrame >= FrameCount || NextFrame < 0))
		{
			Frame = bBackward ? 0 : FrameCount - 1;
			mSubUV = Asset->GetFrameSubUV(Frame);
			bPlaying = false;
			return;
		}

		Frame = ((NextFrame % FrameCount) + FrameCount) % FrameCount;
	}

	mSubUV = Asset->GetFrameSubUV(Frame);
}
