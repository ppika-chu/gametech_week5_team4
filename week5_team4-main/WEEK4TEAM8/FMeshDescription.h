#pragma once

#include "Core.h"
#include "TArray.h"
#include "Vector.h"
#include "FGuid.h"

using FVertexID = uint32;  // Position
using FVertexInstanceID = uint32;
using FPolygonGroupID = uint32;

struct FMeshVertexPosition
{
    FVector Position;
};

struct FMeshVertexInstance  // VertexID + Normal + Color + UV
{
    FVertexID VertexID = 0;

    FVector Normal;
    FVector4 Color = FVector4(1, 1, 1, 1);
    FVector2 TexCoord;
};

struct FMeshTriangle
{
    FVertexInstanceID Corners[3]{};
    FPolygonGroupID PolygonGroupID = 0;
};

struct FPolygonGroup  // Material 그룹
{
    FString MaterialName;
};

struct FMeshDescription
{
    TArray<FMeshVertexPosition> Vertices;
    TArray<FMeshVertexInstance> VertexInstances;
    TArray<FMeshTriangle> Triangles;
    TArray<FPolygonGroup> PolygonGroups;
};

struct FVertex
{
    FVector Pos;
    FVector Normal;
    FVector4 Color;
    FVector2 Tex;

    FVector GetPosition() const { return Pos; }
};

struct FStaticMeshSection
{
    uint32 FirstIndex = 0;
    uint32 IndexCount = 0;
    FString MaterialName;
    FGuid MaterialAssetID;
};

struct FStaticMeshBuildData
{
    TArray<FVertex> Vertices;
    TArray<uint32> Indices;
    TArray<FStaticMeshSection> Sections;
};