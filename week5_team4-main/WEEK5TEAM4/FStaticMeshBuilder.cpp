#include "FStaticMeshBuilder.h"
#include <cassert>

bool FStaticMeshBuilder::Build (const FMeshDescription& MeshDescription,
    FStaticMeshBuildData& OutCookedData)
{
    OutCookedData.Vertices.Empty();
    OutCookedData.Indices.Empty();
    OutCookedData.Sections.Empty();

    for (uint32 PolygonGroupIndex = 0; 
        PolygonGroupIndex < static_cast<uint32>(MeshDescription.PolygonGroups.Num());
        ++PolygonGroupIndex)
    {
        FStaticMeshSection Section;

        Section.FirstIndex = static_cast<uint32>(OutCookedData.Indices.Num());
        Section.MaterialName =  MeshDescription.PolygonGroups[PolygonGroupIndex].MaterialName;

        for (const FMeshTriangle& Triangle : MeshDescription.Triangles)
        {
            if (Triangle.PolygonGroupID != PolygonGroupIndex)
            {
                continue;
            }

            for (uint32 CornerIndex = 0; CornerIndex < 3; ++CornerIndex)
            {
                const FVertexInstanceID VertexInstanceID = Triangle.Corners[CornerIndex];

                if (VertexInstanceID >= static_cast<uint32>(MeshDescription.VertexInstances.Num()))
                {
                    return false;
                }

                const FMeshVertexInstance& VertexInstance = MeshDescription.VertexInstances[VertexInstanceID];
                const FVertexID VertexID = VertexInstance.VertexID;

                if (VertexID >= static_cast<uint32>(MeshDescription.Vertices.Num()))
                {
                    return false;
                }

                const FMeshVertexPosition& MeshVertex =  MeshDescription.Vertices[VertexID];

                FVertex RenderVertex;
                RenderVertex.Pos = MeshVertex.Position;
                RenderVertex.Normal = VertexInstance.Normal;
                RenderVertex.Color = VertexInstance.Color;
                RenderVertex.Tex = VertexInstance.TexCoord;

                const uint32 NewVertexIndex = static_cast<uint32>(OutCookedData.Vertices.Num());

                OutCookedData.Vertices.Add(RenderVertex);
                OutCookedData.Indices.Add(NewVertexIndex);
            }
        }

        Section.IndexCount = static_cast<uint32>(OutCookedData.Indices.Num()) - Section.FirstIndex;
		if (Section.IndexCount > 0)
		{
			OutCookedData.Sections.Add(Section);
		}
	}

	return true;
}

