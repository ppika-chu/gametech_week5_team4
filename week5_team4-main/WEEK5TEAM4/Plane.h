#pragma once

#include "FMeshDescription.h"
#include "Core.h"

// Centered unit plane in YZ, normal -X. Full [0,1] UVs, with (0,0) at texture top-left.
// Indexed triangle list. Vertex layout: position, normal, color, UV.
// Vertices are shared only when position, color and UV all match.
inline FVertex Plane_vertices[] =
{
    { { 0.000000f, -0.500000f, 0.500000f }, { 0.f, 0.f, 0.f }, { 1.000000f, 1.000000f, 1.000000f, 1.000000f }, { 0.000000f, 0.000000f } },
    { { 0.000000f, 0.500000f, 0.500000f }, { 0.f, 0.f, 0.f }, { 1.000000f, 1.000000f, 1.000000f, 1.000000f }, { 1.000000f, 0.000000f } },
    { { 0.000000f, 0.500000f, -0.500000f }, { 0.f, 0.f, 0.f }, { 1.000000f, 1.000000f, 1.000000f, 1.000000f }, { 1.000000f, 1.000000f } },
    { { 0.000000f, -0.500000f, -0.500000f }, { 0.f, 0.f, 0.f }, { 1.000000f, 1.000000f, 1.000000f, 1.000000f }, { 0.000000f, 1.000000f } },
};

inline const uint32 Plane_indices[] =
{
    0, 1, 2, 0, 2, 3,
};
