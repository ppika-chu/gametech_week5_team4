#pragma once
#include "TArray.h"
#include "FMeshDescription.h"

struct FMeshLOD;
class URenderer;

class LOD
{
public:
    FMeshLOD BuildQEMLOD(const TArray<FVertex>& SrcVertices,
                         const TArray<uint32>& SrcIndices,
                         const TArray<FStaticMeshSection>& SrcSections,
                         float TargetTriangleRatio, URenderer& InRenderer);

};

