#pragma once

#include "Core.h"
#include "Vector.h"
#include "TArray.h"

struct FObjVertexIndex
{
    int32 PositionIndex = -1;
    int32 UVIndex = -1;
    int32 NormalIndex = -1;
};

struct FObjFace
{
    TArray<FObjVertexIndex> Vertices; 
    int32 MaterialIndex = -1;
};

struct FObjMaterialInfo
{
    FString Name;

    FVector DiffuseColor = FVector(1.0f, 1.0f, 1.0f); // Kd
    float Opacity = 1.0f;                             // d 또는 Tr
    FString DiffuseTexturePath;                   // map_Kd
};

struct FObjInfo
{
    TArray<FVector> Positions;  // v
    TArray<FVector2> UVs;  // vt
    TArray<FVector> Normals;  // vn
    TArray<FObjFace> Faces;  // f
    TArray<FObjMaterialInfo> Materials;

    FString SourcePath;
};