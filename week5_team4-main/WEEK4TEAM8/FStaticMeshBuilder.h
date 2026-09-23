#pragma once

#include "FMeshDescription.h"

class FStaticMeshBuilder
{
public:
	static bool Build (const FMeshDescription& MeshDescription,
		FStaticMeshBuildData& OutCookedData);
};