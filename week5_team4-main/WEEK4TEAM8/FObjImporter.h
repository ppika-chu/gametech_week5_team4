#pragma once

#include "FObjInfo.h"
#include "FMeshDescription.h"

class FObjImporter
{
public:
    bool ParseObj(const FString& FilePath,  FObjInfo& OutObjInfo);
    bool ConvertToMeshDescription(const FObjInfo& ObjInfo, FMeshDescription& OutMeshDescription);
    
    bool ParseMtl(const FString& FilePath, FObjInfo& OutObjInfo);
private:
    bool ParseFaceVertex(const FString& Token, const FObjInfo& ObjInfo, FObjVertexIndex& OutIndex) const;
    // 음수 → 0-base
    int32 ResolveObjIndex(int32 ObjIndex, int32 ElementCount) const;
};