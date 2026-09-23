#pragma once

#include "Core.h"
#include "TArray.h"
#include "Stb/stb_image.h"
#include "FArchive.h"
#include "Serializers.h"
#include "Object.h"
#include "FStaticMeshBuilder.h"
#include "FObjImporter.h"
#include <filesystem>


// Texture 전용 Payload, FileIO
struct FImagePayload
{
	int32 Width;
	int32 Height;
	int32 Channels;
	TArray<int8> ImageData;
};

class FImageFileIO
{
public:
	static bool Load(const std::filesystem::path& FilePath, FImagePayload& OutPayload)
	{
		stbi_uc* ImageDataPtr = stbi_load(FilePath.string().c_str(), &OutPayload.Width, &OutPayload.Height, &OutPayload.Channels, 4);
		if (!ImageDataPtr)
		{
			return false;
		}

		OutPayload.ImageData.SetNum(OutPayload.Width * OutPayload.Height * 4);
		std::memcpy(OutPayload.ImageData.Data(), ImageDataPtr, OutPayload.ImageData.Num());

		stbi_image_free(ImageDataPtr);

		return true;
	}

	static bool Load(FArchive& Ar, FImagePayload& OutPayload)
	{
		Ar << OutPayload.Width;
		Ar << OutPayload.Height;
		Ar << OutPayload.Channels;
		Ar << OutPayload.ImageData;

		return true;
	}

	static bool Save(FArchive& Ar, FImagePayload& InPayload)
	{
		Ar << InPayload.Width;
		Ar << InPayload.Height;
		Ar << InPayload.Channels;
		Ar << InPayload.ImageData;

		return true;
	}
};

struct FStaticMeshPayload
{
	FMeshDescription MeshDescription;
	TArray<FObjMaterialInfo> Materials;
};

// StaicMesh 전용 IO
class FStaticMeshFileIO
{
public:
	static bool Load(FArchive& Ar, FStaticMeshBuildData& OutData)
	{
		Ar << OutData.Vertices;
		Ar << OutData.Indices;
		Ar << OutData.Sections;
		return true;
	}

	static bool Load(const std::filesystem::path& FilePath,
		FStaticMeshPayload& OutPayload)
	{
		const std::filesystem::path Extension = FilePath.extension();

		if (Extension == ".obj")
		{
			FObjImporter Importer;
			FObjInfo ObjInfo;
			FMeshDescription MeshDescription;

			if (!Importer.ParseObj(FString(FilePath.string()), ObjInfo))
			{
				return false;
			}
			if (!Importer.ConvertToMeshDescription(ObjInfo, MeshDescription))
			{
				return false;
			}

			OutPayload.MeshDescription = std::move(MeshDescription);
			OutPayload.Materials = ObjInfo.Materials;
			return true;
		}

		return false;
	}

	static bool Save(FArchive& Ar, FStaticMeshBuildData& InData)
	{
		Ar << InData.Vertices;
		Ar << InData.Indices;
		Ar << InData.Sections;
		return true;
	}
	
private:
};

// Material 전용 Payload, IO
struct  FMaterialPayload
{
	FVector AmbientColor;
	FVector DiffuseColor;
	FVector SpecularColor;
	FGuid DiffuseTexture;
	FGuid SpecularTexture;
	FGuid NormalTexture;
	float Opacity;
};

class FMaterialFileIO
{
public:
	static bool Load(FArchive& Ar, FMaterialPayload& OutPayload)
	{
		Ar << OutPayload.AmbientColor;
		Ar << OutPayload.DiffuseColor;
		Ar << OutPayload.SpecularColor;
		Ar << OutPayload.DiffuseTexture;
		Ar << OutPayload.SpecularTexture;
		Ar << OutPayload.NormalTexture;
		Ar << OutPayload.Opacity;
		
		return true;
	}
	
	// mtl -> uasset
	static bool Save(FArchive& Ar, FMaterialPayload& InPayload)
	{
		Ar << InPayload.AmbientColor;
		Ar << InPayload.DiffuseColor;
		Ar << InPayload.SpecularColor;
		Ar << InPayload.DiffuseTexture;
		Ar << InPayload.SpecularTexture;
		Ar << InPayload.NormalTexture;
		Ar << InPayload.Opacity;
		
		return true;
	}
	
	private:
};
