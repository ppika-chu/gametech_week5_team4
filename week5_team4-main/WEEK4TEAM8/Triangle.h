#pragma once

#include "FMeshDescription.h"
#include "Core.h"

// Triangle in the YZ plane. UV: top (0.5, 0), bottom corners (1, 1) and (0, 1).
// Indexed triangle list. Vertex layout: position, normal, color, UV.
// Vertices are shared only when position, color and UV all match.
inline FVertex Triangle_vertices[] =
{
    { { 0.000000f, 0.000000f, 1.000000f }, { 0.f, 0.f, 0.f }, { 1.000000f, 0.000000f, 0.000000f, 1.000000f }, { 0.500000f, 0.000000f } },
    { { 0.000000f, 1.000000f, -1.000000f }, { 0.f, 0.f, 0.f }, { 0.000000f, 1.000000f, 0.000000f, 1.000000f }, { 1.000000f, 1.000000f } },
    { { 0.000000f, -1.000000f, -1.000000f }, { 0.f, 0.f, 0.f }, { 0.000000f, 0.000000f, 1.000000f, 1.000000f }, { 0.000000f, 1.000000f } },
};

inline const uint32 Triangle_indices[] =
{
    0, 1, 2,
};
