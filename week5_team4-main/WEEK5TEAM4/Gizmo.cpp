#include "Gizmo.h"
#include "Actor.h"
#include "Renderer.h"
#include "ImGui/imgui.h"
#include "WindowApplication.h"
#include "EngineMathLibrary.h"
#include "SceneManager.h"
#include <cmath>

FGizmo::FGizmo(URenderer& InRenderer) 
	: Renderer(InRenderer) 
{
}

void FGizmo::Reset()
{
    bIsSelected = bIsHoveredAxis = false;
    SelectedAxis = HoveredAxis = EAxisNumber::None;
    TargetUUID = -1;
    HandleScreenSegments.Empty();
}

void FGizmo::SetWorldMode(bool bInWorldMode)
{
	if (bWorldMode != bInWorldMode)
	{
		Reset();
	}

    bWorldMode = bInWorldMode;
}

void FGizmo::SetOperation(EGIZMO_TYPE Operation)
{
    if (CurrentOperation != Operation) 
	{
		Reset();
	}

    CurrentOperation = Operation;
}

EGIZMO_TYPE FGizmo::GetOperation() const 
{ 
	return CurrentOperation; 
}

bool FGizmo::IsMouseOverHandle() const 
{ 
	return bIsHoveredAxis; 
}

void FGizmo::Tick(AActor* TargetActor, const FRect& ViewportRect, bool bViewportHovered, const FMatrix& InvViewProjection)
{
    if (!TargetActor)
    {
        Reset();
        return;
    }

    if (TargetUUID != TargetActor->UUID)
    {
        Reset();
        TargetUUID = TargetActor->UUID;
    }

    const FInputState& Input = WindowApplication.Input;

	FVector2 MousePosInScreen = {
		static_cast<float>(Input.CursorX - ViewportRect.X),
		static_cast<float>(Input.CursorY - ViewportRect.Y)
	};

    const bool bAllowMouse = bViewportHovered;
    bool bDragStarted = false;

    if (!Input.IsDown(VK_LBUTTON))
    {
        bIsSelected = false; SelectedAxis = EAxisNumber::None;
    }

    bIsHoveredAxis = false;
    HoveredAxis = EAxisNumber::None;

    if (bAllowMouse)
    {
        for (const FHandleSegment& Segment : HandleScreenSegments)
        {
            if (PointToLineSegmentDistanceSquared(MousePosInScreen, Segment.Start, Segment.End) >= HandleHitRadius * HandleHitRadius)
            {
                continue;
            }

            bIsHoveredAxis = true;
            HoveredAxis = Segment.Axis;
            if (!bIsSelected && Input.WasPressed(VK_LBUTTON))
            {
                PrevMousePos = MousePosInScreen;
                AxisDirection = Segment.Direction;
                HandleScreenStart = Segment.Start;
                HandleScreenDirection = Segment.End - Segment.Start;
                HandleScreenDirection.Normalize();
                DragStartLocation = TargetActor->GetTransform().Location;
                DragStartMousePosition = MousePosInScreen;
                bDragStarted = true;
                bIsSelected = true;
                SelectedAxis = Segment.Axis;
            }
            break;
        }
    }

    if (!bIsSelected)
    {
        return;
    }

    const FTransform Transform = TargetActor->GetTransform();
    if (CurrentOperation == EGIZMO_TYPE::TRANSLATE)
    {
        float ProjectionLength = FVector2::Dot(MousePosInScreen - HandleScreenStart, HandleScreenDirection);
        FVector2 ProjectedPoint = HandleScreenStart + HandleScreenDirection * ProjectionLength;

        // Ray
        FVector NearPoint = ScreenToWorld(ProjectedPoint, InvViewProjection, ViewportRect.Width, ViewportRect.Height, 0.1f);
        FVector FarPoint = ScreenToWorld(ProjectedPoint, InvViewProjection, ViewportRect.Width, ViewportRect.Height, 1.0f);

        FRay Ray;
        Ray.Origin = NearPoint;
        Ray.Direction = FarPoint - NearPoint;
        Ray.Direction.Normalize();

        FVector W = DragStartLocation - Ray.Origin;

        float A = FVector::dot(AxisDirection, AxisDirection);
        float B = FVector::dot(AxisDirection, Ray.Direction);
        float C = FVector::dot(Ray.Direction, Ray.Direction);
        float D = FVector::dot(AxisDirection, W);
        float E = FVector::dot(Ray.Direction, W);

        float Denominator = A * C - B * B;
        if (FMath::Abs(Denominator) <= KINDA_SMALL_NUMBER)
        {
            return;
        }
        float T = (B * E - C * D) / Denominator;

        if (bDragStarted)
        {
            DragStartAxisParameter = T;
        }
        else
        {
            FVector NewLocation = DragStartLocation + AxisDirection * (T - DragStartAxisParameter);
            TargetActor->SetLocation(NewLocation);
        }
    }
    else if (CurrentOperation == EGIZMO_TYPE::ROTATE)
    {
        const float Sensitivity = 0.01f;
        const float Amount = FVector2::Dot(MousePosInScreen - PrevMousePos, HandleScreenDirection);

        FQuaternion RotationQ = ToQuaternion(FMatrix::Rotate(Transform.Rotation));
        FQuaternion DeltaQ(AxisDirection, Amount * Sensitivity);
        FQuaternion FinalQ = DeltaQ * RotationQ;
        FinalQ.Normalize();
        const FVector Euler = ToEulerAngles(FinalQ) * (180.f / PI);
        TargetActor->SetRotation(FRotator(Euler.y, Euler.z, Euler.x));
    }
    else if (CurrentOperation == EGIZMO_TYPE::SCALE)
    {
        const float Sensitivity = 0.01f;
        const float Amount = FVector2::Dot(MousePosInScreen - PrevMousePos, HandleScreenDirection);

        FVector Scale = Transform.Scale + AxisDirection * Amount * Sensitivity;
        Scale.x = FMath::Max(Scale.x, MIN_SCALE);
        Scale.y = FMath::Max(Scale.y, MIN_SCALE);
        Scale.z = FMath::Max(Scale.z, MIN_SCALE);

        TargetActor->SetScale(Scale);
    }

    PrevMousePos = MousePosInScreen;
}

void FGizmo::Render(AActor* TargetActor, const FVector& CameraPosition, const FRect& ViewportRect, const FMatrix& ViewProjection, bool bIsOrtho, float OrthoDistance)
{
    HandleScreenSegments.Empty();

    if (!TargetActor) 
	{ 
		Reset(); 
		return; 
	}

    if (TargetUUID != TargetActor->UUID) 
	{ 
		Reset(); 
		TargetUUID = TargetActor->UUID; 
	}

    const FTransform Transform = TargetActor->GetTransform();
    const FVector CenterToCamera = CameraPosition - Transform.Location;

    // 직교 화면이면 직교 상 거리에 비례한 크기 조절
    const float AxisLength = bIsOrtho ? (0.08f * OrthoDistance) : (0.1f * CenterToCamera.Length());
    const int32 ScreenWidth = static_cast<int32>(ViewportRect.Width);
    const int32 ScreenHeight = static_cast<int32>(ViewportRect.Height);
    const FVector4 Clip = FVector4(Transform.Location, 1.f) * ViewProjection;
    const bool bDrawGizmo = !(Clip.w <= 0.00001f || Clip.z < 0.f || Clip.z > Clip.w || Clip.x < -Clip.w || Clip.x > Clip.w || Clip.y < -Clip.w || Clip.y > Clip.w);
	
	if (!bDrawGizmo)
	{
		return;
	}

    enum class EAxisEndPointStyle { 
        None, 
        Arrow, 
        Circle 
    };

    const FVector2 Center = WorldToScreen(Transform.Location, ViewProjection, ScreenWidth, ScreenHeight);
    auto AxisColor = [&](EAxisNumber Axis, const FVector4& Color)
    {
        return Axis == (bIsSelected ? SelectedAxis : HoveredAxis) ? FVector4(1,1,0,1) : Color;
    };

    auto DrawLineAxis = [&](const FVector& DrawAxis, const FVector& ApplyAxis, const FVector4& Color, EAxisEndPointStyle Style, EAxisNumber Axis)
    {
        const FVector2 End = WorldToScreen(Transform.Location + DrawAxis * AxisLength, ViewProjection, ScreenWidth, ScreenHeight);

        FVector2 ScreenAxis = End - Center;
		if (ScreenAxis.LengthSquared() < 0.01f)
		{
			return;
		}

        HandleScreenSegments.Add({ Center, End, ApplyAxis, Axis });
        
		const FVector4 Highlight = AxisColor(Axis, Color);
        Renderer.RenderLine2D(Center, End, Highlight, 5.f);
        
		if (Style == EAxisEndPointStyle::Arrow)
		{
            Renderer.RenderTriangle2D(End, Highlight, 20.f, atan2f(ScreenAxis.Y, ScreenAxis.X));
		}
		else if (Style == EAxisEndPointStyle::Circle)
		{
            Renderer.RenderCircle2D(End, Highlight, 8.f);
		}
    };

    auto DrawCircleAxis = [&](const FVector& U, const FVector& V, const FVector4& Color, bool bNoClipping, EAxisNumber Axis)
    {
        constexpr int32 NumSegments = 32;
        FVector Points[NumSegments];
        GenerateCircleVertices([&](int32 Index, const FVector2& Point)
        {
            Points[Index] = Transform.Location + U * Point.X + V * Point.Y;
        }, AxisLength, NumSegments);

        for (int32 I = 0; I < NumSegments; ++I)
        {
            const FVector& StartWorld = Points[I];
            const FVector& EndWorld = Points[(I + 1) % NumSegments];
			if (!bNoClipping && FVector::dot(Lerp(StartWorld, EndWorld, 0.5f) - Transform.Location, CenterToCamera) < 0.f)
			{
				continue;
			}
            const FVector2 Start = WorldToScreen(StartWorld, ViewProjection, ScreenWidth, ScreenHeight);
            const FVector2 End = WorldToScreen(EndWorld, ViewProjection, ScreenWidth, ScreenHeight);
            
			if (FVector2::LengthSquared(Start, End) < 0.01f) 
			{
				continue;
			}

            HandleScreenSegments.Add({ Start, End, FVector::cross(U,V), Axis });
            Renderer.RenderLine2D(Start, End, AxisColor(Axis, Color), 2.f);
        }
    };

    const FMatrix Rotation = FMatrix::Rotate(Transform.Rotation);
    const bool bLocal = !bWorldMode || CurrentOperation == EGIZMO_TYPE::SCALE;
    const FVector ForwardAxis = bLocal ? Rotation.GetUnitAxis(EAxis::X) : Front;
    const FVector RightAxis = bLocal ? Rotation.GetUnitAxis(EAxis::Y) : Right;
    const FVector UpAxis = bLocal ? Rotation.GetUnitAxis(EAxis::Z) : Up;

    if (CurrentOperation == EGIZMO_TYPE::TRANSLATE)
    {
        DrawLineAxis(ForwardAxis, ForwardAxis, FVector4(1, 0, 0, 1), EAxisEndPointStyle::Arrow, EAxisNumber::X);
        DrawLineAxis(RightAxis, RightAxis, FVector4(0, 1, 0, 1), EAxisEndPointStyle::Arrow, EAxisNumber::Y);
        DrawLineAxis(UpAxis, UpAxis, FVector4(0, 0, 1, 1), EAxisEndPointStyle::Arrow, EAxisNumber::Z);
    }
    else if (CurrentOperation == EGIZMO_TYPE::ROTATE)
    {
        DrawCircleAxis(RightAxis, UpAxis, FVector4(1, 0, 0, 1), false, EAxisNumber::X);
        DrawCircleAxis(UpAxis, ForwardAxis, FVector4(0, 1, 0, 1), false, EAxisNumber::Y);
        DrawCircleAxis(ForwardAxis, RightAxis, FVector4(0, 0, 1, 1), false, EAxisNumber::Z);

        FVector CameraAxisU = FVector::cross(CenterToCamera, Up);
        if (CameraAxisU.IsNearlyZero())
        {
            CameraAxisU = FVector::cross(CenterToCamera, Right);
        }

        if (!CameraAxisU.IsNearlyZero())
        {
            CameraAxisU.Normalize();

            FVector CameraAxisV = FVector::cross(CameraAxisU, CenterToCamera);
            CameraAxisV.Normalize();

            DrawCircleAxis(CameraAxisU, CameraAxisV, FVector4(1, 1, 1, 1), true, EAxisNumber::Cameara);
        }
    }
    else if (CurrentOperation == EGIZMO_TYPE::SCALE)
    {
        DrawLineAxis(ForwardAxis, Front, FVector4(1, 0, 0, 1), EAxisEndPointStyle::Circle, EAxisNumber::X);
        DrawLineAxis(RightAxis, Right, FVector4(0, 1, 0, 1), EAxisEndPointStyle::Circle, EAxisNumber::Y);
        DrawLineAxis(UpAxis, Up, FVector4(0, 0, 1, 1), EAxisEndPointStyle::Circle, EAxisNumber::Z);
    }

    Renderer.RenderCircle2D(Center, FVector4(0.8f, 0.8f, 0.8f, 1), 5.f);
}
