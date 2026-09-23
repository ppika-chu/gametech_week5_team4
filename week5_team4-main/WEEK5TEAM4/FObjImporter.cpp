#include "Core.h"
#include "FObjImporter.h"
#include "FLogManager.h"

#include <fstream>
#include <sstream>
#include <filesystem>

bool FObjImporter::ParseObj(const FString& FilePath, FObjInfo& OutObjInfo)
{
    std::ifstream File(FilePath.CStr());

    if (!File.is_open())
    {
        return false;
        UE_LOG_ERROR("Failed to convert OBJ mesh");
    }

    int32 CurrentMaterialIndex = -1;

    OutObjInfo.SourcePath = FilePath;
    std::string Line;

    while (std::getline(File, Line))
    {
        std::istringstream Stream(Line);
        std::string Command;
        Stream >> Command;

        if (Command == "v")
        {
            float X, Y, Z;
            Stream >> X >> Y >> Z;
            OutObjInfo.Positions.Add(FVector(X, Y, Z));
        }
        else if (Command == "vt")
        {
            float U, V;
            Stream >> U >> V;

            // .obj vt는 관례상 v=0이 텍스처 하단이나,
            // directx는 v=0이 텍스처 상단
            OutObjInfo.UVs.Add(FVector2(U, 1.0f-V));
        }
        else if (Command == "vn")
        {
            float X, Y, Z;
            Stream >> X >> Y >> Z;
            OutObjInfo.Normals.Add(FVector(X, Y, Z));
        }
        else if (Command == "f")
        {
            FObjFace Face;
            Face.MaterialIndex = CurrentMaterialIndex;

            std::string Token;
            while (Stream >> Token)
            {
                FObjVertexIndex VertexIndex;

                if (!ParseFaceVertex(Token, OutObjInfo, VertexIndex))
                {
                    return false;

                    UE_LOG_ERROR("Failed to convert OBJ mesh");
                }

                Face.Vertices.Add(VertexIndex);
            }

            if (Face.Vertices.Num() < 3)
            {
                return false;

                UE_LOG_ERROR("Failed to convert OBJ mesh");
            }
            OutObjInfo.Faces.Add(Face);
        }
        else if (Command == "usemtl")
        {
            std::string MaterialName;

            Stream >> MaterialName;

            for (int32 MaterialIndex = 0; MaterialIndex < OutObjInfo.Materials.Num(); ++MaterialIndex)
            {
                if (OutObjInfo.Materials[MaterialIndex].Name == FString(MaterialName))
                {
                    CurrentMaterialIndex = MaterialIndex;

                    break;
                }
            }
        }
        else if (Command == "mtllib")
        {
            std::string MtlFileName;
            Stream >> MtlFileName;

            const std::filesystem::path ObjPath(FilePath.CStr());
            const std::filesystem::path MtlPath = ObjPath.parent_path() / MtlFileName;

            ParseMtl(FString(MtlPath.string()), OutObjInfo);
        }
    }

    return true;
}

bool FObjImporter::ConvertToMeshDescription(const FObjInfo& ObjInfo, FMeshDescription& OutMeshDescription)
{
    OutMeshDescription.Vertices.Empty();
    OutMeshDescription.VertexInstances.Empty();
    OutMeshDescription.Triangles.Empty();
    OutMeshDescription.PolygonGroups.Empty();

    FPolygonGroup DefaultPolygonGroup;
    DefaultPolygonGroup.MaterialName = FString("DefaultMaterial");
    OutMeshDescription.PolygonGroups.Add(DefaultPolygonGroup);

    for (const FObjMaterialInfo& ObjMaterial : ObjInfo.Materials)
    {
        FPolygonGroup PolygonGroup;
        PolygonGroup.MaterialName = ObjMaterial.Name;
        OutMeshDescription.PolygonGroups.Add(PolygonGroup);
    }

    for (const FVector& Position : ObjInfo.Positions)
    {
        FMeshVertexPosition MeshVertex;
        MeshVertex.Position = Position;

        OutMeshDescription.Vertices.Add(MeshVertex);
    }

    for (const FObjFace& Face : ObjInfo.Faces)
    {
        const int32 FaceVertexCount = Face.Vertices.Num();
        if (FaceVertexCount < 3)
        {
            return false;
        }

        const FObjVertexIndex& FirstIndex = Face.Vertices[0];
        const FObjVertexIndex& SecondIndex = Face.Vertices[1];
        const FObjVertexIndex& ThirdIndex = Face.Vertices[2];
        if (FirstIndex.PositionIndex < 0 || SecondIndex.PositionIndex < 0 || ThirdIndex.PositionIndex < 0
            || FirstIndex.PositionIndex >= ObjInfo.Positions.Num()
            || SecondIndex.PositionIndex >= ObjInfo.Positions.Num()
            || ThirdIndex.PositionIndex >= ObjInfo.Positions.Num())
        {
            return false;
        }

        FVector GeneratedNormal = FVector::cross(
            ObjInfo.Positions[SecondIndex.PositionIndex] - ObjInfo.Positions[FirstIndex.PositionIndex],
            ObjInfo.Positions[ThirdIndex.PositionIndex] - ObjInfo.Positions[FirstIndex.PositionIndex]);
        if (!GeneratedNormal.IsNearlyZero())
        {
            GeneratedNormal.Normalize();
        }
        else
        {
            GeneratedNormal = FVector(0, 0, 1);
        }

        TArray<FVertexInstanceID> FaceInstanceIDs;

        for (const FObjVertexIndex& ObjIndex : Face.Vertices)
        {
            if (ObjIndex.PositionIndex < 0 || ObjIndex.PositionIndex >= ObjInfo.Positions.Num())
            {
                return false;
            }
            FMeshVertexInstance VertexInstance;
            VertexInstance.VertexID = static_cast<FVertexID>(ObjIndex.PositionIndex);

            if (ObjIndex.UVIndex >= 0)
            {
                if (ObjIndex.UVIndex >= ObjInfo.UVs.Num())
                {
                    return false;
                }

                VertexInstance.TexCoord = ObjInfo.UVs[ObjIndex.UVIndex];
            }
            else
            {
                VertexInstance.TexCoord = FVector2(0, 0);
            }

            if (ObjIndex.NormalIndex >= 0)
            {
                if (ObjIndex.NormalIndex >= ObjInfo.Normals.Num())
                {
                    return false;
                }

                VertexInstance.Normal = ObjInfo.Normals[ObjIndex.NormalIndex];
            }
            else
            {
                VertexInstance.Normal = GeneratedNormal;
            }

            VertexInstance.Color = FVector4(1, 1, 1, 1);

            const FVertexInstanceID NewInstanceID = OutMeshDescription.VertexInstances.Add(VertexInstance);
            FaceInstanceIDs.Add(NewInstanceID);
        }

        for (int32 TriangleIndex = 1; TriangleIndex < FaceVertexCount - 1; ++TriangleIndex)
        {
            FMeshTriangle Triangle;

            Triangle.Corners[0] = FaceInstanceIDs[0];
            Triangle.Corners[1] = FaceInstanceIDs[TriangleIndex];
            Triangle.Corners[2] = FaceInstanceIDs[TriangleIndex + 1];

            Triangle.PolygonGroupID = Face.MaterialIndex >= 0
                ? static_cast<FPolygonGroupID>(Face.MaterialIndex + 1)
                : 0;

            OutMeshDescription.Triangles.Add(Triangle);
        }
    }
    return true;
}


bool FObjImporter::ParseFaceVertex(const FString& Token, const FObjInfo& ObjInfo, FObjVertexIndex& OutIndex) const
{
    const std::string Value = Token.CStr();
    const size_t FirstSlash = Value.find('/');
    const size_t SecondSlash = FirstSlash == std::string::npos ? std::string::npos : Value.find('/', FirstSlash + 1);

    auto ParseIndex = [](const std::string& Text, int32 ElementCount, int32& OutValue) -> bool
    {
        if (Text.empty())
        {
            OutValue = -1;
            return true;
        }

        try
        {
            size_t ParsedCharacters = 0;
            const int32 RawIndex = std::stoi(Text, &ParsedCharacters);
            if (ParsedCharacters != Text.size() || RawIndex == 0)
            {
                return false;
            }

            OutValue = RawIndex > 0 ? RawIndex - 1 : ElementCount + RawIndex;
            return OutValue >= 0 && OutValue < ElementCount;
        }
        catch (const std::exception&)
        {
            return false;
        }
    };

    const std::string PositionText = Value.substr(0, FirstSlash);
    const std::string UVText = FirstSlash == std::string::npos
        ? ""
        : Value.substr(FirstSlash + 1, (SecondSlash == std::string::npos ? Value.size() : SecondSlash) - FirstSlash - 1);
    const std::string NormalText = SecondSlash == std::string::npos ? "" : Value.substr(SecondSlash + 1);

    // UE_LOG_ERROR("UV : %f %f", ObjInfo.UVs[0].X, ObjInfo.UVs[0].Y);

    return ParseIndex(PositionText, ObjInfo.Positions.Num(), OutIndex.PositionIndex)
        && ParseIndex(UVText, ObjInfo.UVs.Num(), OutIndex.UVIndex)
        && ParseIndex(NormalText, ObjInfo.Normals.Num(), OutIndex.NormalIndex);
}


bool FObjImporter::ParseMtl(const FString& FilePath, FObjInfo& OutObjInfo)
{
    std::ifstream File(FilePath.CStr());

    if (!File.is_open())
    {
        return false;
    }

    FObjMaterialInfo* CurrentMaterial = nullptr;
    std::string Line;

    while (std::getline(File, Line))
    {
        std::istringstream Stream(Line);
        std::string Command;
        Stream >> Command;

        if (Command.empty())
        {
            continue;
        }

        if (Command[0] == '#')
        {
            continue;
        }

        if (Command == "newmtl")
        {
            std::string MaterialName;
            Stream >> MaterialName;
            if (MaterialName.empty())
            {
                CurrentMaterial = nullptr;

                continue;
            }

            FObjMaterialInfo NewMaterial;
            NewMaterial.Name = FString(MaterialName);
            OutObjInfo.Materials.Add(NewMaterial);

            CurrentMaterial = &OutObjInfo.Materials[OutObjInfo.Materials.Num() - 1];
            continue;
        }
        if (!CurrentMaterial)
        {
            continue;
        }

        if (Command == "Kd")
        {
            float R = 1.0f;
            float G = 1.0f;
            float B = 1.0f;
            if (Stream >> R >> G >> B)
            {
                CurrentMaterial->DiffuseColor = FVector(R, G, B);
            }
        }
        else if (Command == "d")
        {
            float Opacity = 1.0f;
            if (Stream >> Opacity)
            {
                CurrentMaterial->Opacity = Opacity;
            }
        }
        else if (Command == "Tr")
        {
            float Transparency = 0.0f;
            if (Stream >> Transparency)
            {
                CurrentMaterial->Opacity = 1.0f - Transparency;
            }
        }
        else if (Command == "map_Kd")
        {
            std::string TexturePath;
            Stream >> TexturePath;
            CurrentMaterial->DiffuseTexturePath = FString(TexturePath);
        }
    }

    return true;
}

int32 FObjImporter::ResolveObjIndex(int32 ObjIndex, int32 ElementCount) const
{
    if (ObjIndex > 0) 
    {
        return ObjIndex - 1;
    }

    if (ObjIndex < 0)
    {
        return ElementCount + ObjIndex;
    }

    return -1;
}
