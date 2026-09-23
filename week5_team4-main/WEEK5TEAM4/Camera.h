#pragma once
#include "Transform.h"
#include <cmath>
#include "Vector.h"

class FCamera
{
public:
	FCamera() : Transform(FTransform({ -2.0f, 1.0f, 1.0f }, { 0, 30, 0 }, { 1, 1, 1 }))
	{
		LookAt({ 0, 0, 0 });
	}

	FCamera(FTransform _FTransform) : Transform(_FTransform) {}

	FMatrix GetViewMatrix() const
	{
		// 카메라에는 스케일이 없다. 위치를 되돌리고, 회전을 되돌리고, 축을 교환한다.
		return FMatrix::Translation(FVector(-Transform.Location.x,
			-Transform.Location.y,
			-Transform.Location.z))
			* FMatrix::Rotate(Transform.Rotation).Transpose()
			* FMatrix::UEToDX;
	}

	// 특정 지점을 바라보도록 회전을 맞춘다.
	void LookAt(const FVector& Target)
	{
		Transform.Rotation = FRotator::LookAt(Transform.Location, Target);
	}

	FMatrix GetProjectionMatrix() const
	{
		//fov 단위는 라디안
		FMatrix result = FMatrix::Zero; //영벡터
		float yScale = 1.0f / tanf((mFovDegree / 2) * PI / 180); //xScale
		float xScale = yScale / mAspect;

		result.M[0][0] = xScale; //xScale
		result.M[1][1] = yScale; //yScale
		result.M[2][2] = mFar / (mFar - mNear); //A 임시
		result.M[3][2] = -mNear * mFar / (mFar - mNear); //B 임시
		result.M[2][3] = 1;

		return result;
	}

	// Transpose matrix for perspective projection.
	FMatrix GetProjectionT_pMatrix() const
	{
		FMatrix result = FMatrix::Zero;

		result.M[0][0] = 1.0f;
		result.M[1][1] = 1.0f;
		result.M[2][2] = mFar / (mFar - mNear);
		result.M[2][3] = 1.0f;
		result.M[3][2] = -mNear * mFar / (mFar - mNear);

		return result;
	}

	// Transpose matrix for orthographic projection.
	FMatrix GetProjectionT_oMatrix(float d) const
	{
		FMatrix result = FMatrix::Zero;

		result.M[0][0] = 1.0f / d;
		result.M[1][1] = 1.0f / d;
		result.M[2][2] = 1.0f / (mFar - mNear);
		result.M[3][2] = -mNear / (mFar - mNear);
		result.M[3][3] = 1.0f;

		return result;
	}

	// Transpose matrix for both perspective and orthographic projection.
	// T_unified = (1 - t) * T_orthographic + t * T_perspective / d
	FMatrix GetProjectionT_uMatrix(float d, float t) const
	{
		FMatrix result = FMatrix::Zero;

		result.M[0][0] = 1.0f / d;
		result.M[1][1] = 1.0f / d;
		result.M[2][2] = ((1 - t) + t * mFar / d) / (mFar - mNear);
		result.M[2][3] = t / d;
		result.M[3][2] = -mNear * ((1 - t) + t * mFar / d) / (mFar - mNear);
		result.M[3][3] = 1.0f - t;

		return result;
	}

	// Scale matrix for projection
	FMatrix GetProjectionSMatrix() const
	{
		float yScale = 1.0f / tanf((mFovDegree / 2) * PI / 180); //yScale
		float xScale = yScale / mAspect; //xScale

		FMatrix result = FMatrix::Zero;

		result.M[0][0] = xScale;
		result.M[1][1] = yScale;
		result.M[2][2] = 1;
		result.M[3][3] = 1;

		return result;
	}

	// Unified matrix for projection
	FMatrix GetUnifiedProjectionMatrix(float d, float t) const
	{
		const float sy = 1.0f / tanf((mFovDegree / 2) * PI / 180); //yScale
		const float sx = sy / mAspect;

		const float A = ((1.0f - t) + t * mFar / d) / (mFar - mNear);

		FMatrix result = FMatrix::Zero;
		result.M[0][0] = sx / d;
		result.M[1][1] = sy / d;
		result.M[2][2] = A;
		result.M[2][3] = t / d;
		result.M[3][2] = -mNear * A;
		result.M[3][3] = 1.0f - t;

		return result;
	}

	// Inverse matrix for unified projection matrix
	FMatrix GetInverseUnifiedProjectionMatrix(float d, float t) const
	{
		const FMatrix P = GetUnifiedProjectionMatrix(d, t);

		const float A = P.M[2][2];
		const float B = P.M[2][3];
		const float C = P.M[3][2];
		const float D = P.M[3][3];

		const float degt = A * D - B * C;

		FMatrix result = FMatrix::Zero;
		result.M[0][0] = 1.0f / P.M[0][0];
		result.M[1][1] = 1.0f / P.M[1][1];
		result.M[2][2] = D / degt;
		result.M[2][3] = -B / degt;
		result.M[3][2] = -C / degt;
		result.M[3][3] = A / degt;

		return result;
	}

	FMatrix GetOrthographicMatrix(float width, float height) const
	{
		FMatrix result = FMatrix::Zero; // 영벡터

		result.M[0][0] = 2.0f / width;
		result.M[1][1] = 2.0f / height;
		result.M[2][2] = 1.0f / (mFar - mNear);
		result.M[3][2] = -mNear / (mFar - mNear);
		result.M[3][3] = 1.0f;

		return result;
	}

	void Rotate(long Dx, long Dy)
	{
		Transform.Rotation.Yaw += FMath::Fmod(Dx * Sensitivity, 360.f);
		Transform.Rotation.Pitch -= FMath::Fmod(Dy * Sensitivity, 360.f);
	}

	void Update();

	void SetSensitivity(float _v) { Sensitivity = _v; }
	FVector GetForwardVector() const { return FMatrix::Rotate(Transform.Rotation).GetUnitAxis(EAxis::X); }
	FVector GetRightVector()   const { return FMatrix::Rotate(Transform.Rotation).GetUnitAxis(EAxis::Y); }
	FVector GetUpVector()      const { return FMatrix::Rotate(Transform.Rotation).GetUnitAxis(EAxis::Z); }

	FTransform Transform;

	float mAspect = 1.0f;
	float mNear = 0.1f;
	float mFar = 1000.0f;

	//속력
	float Speed = 5.f;

	//속도
	FVector Velocity = FVector(0);

	//카메라 이동 민감도
	float Sensitivity = 0.1f;

	//카메라 시야각
	float mFovDegree = 60.f;

	// 직교 투영에서 카메라와 화면 사이의 거리
	float mOrthoDistance = 5.0f;

	// 직교 투영에서 화면이 담는 월드 높이. 폭은 여기에 Aspect를 곱해서 얻는다.
	// 렌더와 피킹이 같은 값을 봐야 하므로 카메라가 들고 있는다
	// Todo: Check value
	// 직교 투영에서 화면이 담는 월드 높이.
	float mOrthoHeight = 2.f;
	//float mOrthoHeight = 5.f;

	//감속 계수(1/초). 클수록 빨리 멈춘다
	float Damping = 6.f;
};
