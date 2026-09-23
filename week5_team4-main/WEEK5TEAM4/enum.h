#pragma once

enum class EAxis : int { X = 0, Y = 1, Z = 2 };

enum class EPrimitive
{
	EP_Sphere,
	EP_Cube,
	EP_Triangle,
	EP_GizmoArrow,
	EP_Circle,
	EP_Plane,
	EP_StaticMesh,
};

enum EGIZMO_AXIS //어떤축이 선택되었는지
{
	NONE,
	X,
	Y,
	Z
};

enum EGIZMO_TYPE {
	TRANSLATE,
	ROTATE,
	SCALE,
};

enum class EViewModeIndex
{
	VMI_Lit,
	VMI_Unlit,
	VMI_Wireframe,

	// 실제 뷰 모드가 아니다. 뷰 모드별 배열 크기를 잡는 데 쓴다.
	// 모드를 추가하면 이 앞에 넣을 것.
	VMI_Max,
};

enum class ECoordinateSpace
{
	World,
	Screen
};

enum class EViewportType
{
	Perspective,
	Top,
	Front,
	Side,

	// ViewportType을 세기 위한 변수 뷰포트 타입 추가는 위에 작성
	Max
};

