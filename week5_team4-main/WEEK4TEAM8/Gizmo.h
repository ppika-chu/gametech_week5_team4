#pragma once

#include "Vector.h"
#include "RenderInfo.h"
#include "TArray.h"
#include "Transform.h"
#include "enum.h"

class AActor;
class URenderer;
class FSceneManager;

enum class EAxisNumber 
{ 
    None, 
    X, 
    Y, 
    Z, 
    Cameara
};

class FGizmo
{
public:
    FGizmo(URenderer& InRenderer);
    void SetWorldMode(bool bInWorldMode);
    void SetOperation(EGIZMO_TYPE Operation);
    EGIZMO_TYPE GetOperation() const;

    void Tick(AActor* TargetActor, const FRect& ViewportRect, bool bViewportHovered, const FMatrix& InvViewProjection);
    void Render(AActor* TargetActor, const FVector& CameraPosition, const FRect& ViewportRect, const FMatrix& ViewProjection, bool bIsOrtho = false, float OrthoDistance = 10.0f);
    bool IsMouseOverHandle() const;
    bool IsDragging() const { return bIsSelected; }
    void Reset();

private:
    // Draw에서 그린 선분과 해당 축만 입력 단계에 공유한다.
    struct FHandleSegment
    {
        FVector2 Start, End;
        FVector Direction;
        EAxisNumber Axis;
    };

    static constexpr float HandleHitRadius = 5.f;

    URenderer& Renderer;

    inline static EGIZMO_TYPE CurrentOperation = EGIZMO_TYPE::TRANSLATE;
    inline static bool bWorldMode = true;
    bool bIsSelected = false;
    bool bIsHoveredAxis = false;

    TArray<FHandleSegment> HandleScreenSegments;
    FVector2 PrevMousePos;
    FVector DragStartLocation;
    FVector2 DragStartMousePosition;
	float DragStartAxisParameter = 0.f;
	FVector2 HandleScreenStart;
    FVector2 HandleScreenDirection;
    EAxisNumber HoveredAxis = EAxisNumber::None;
    EAxisNumber SelectedAxis = EAxisNumber::None;
    FVector AxisDirection;

    int32 TargetUUID = -1;
};
