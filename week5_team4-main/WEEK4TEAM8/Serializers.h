#pragma once

#include "Core.h"
#include "FArchive.h"
#include "Vector.h"
#include "FName.h"
#include "FGuid.h"
#include "FAsset.h"
#include "FObjImporter.h"

// FVector Serializers (x, y, z)
template <>
struct FArchiveSerializer<FVector>
{
	static void Serialize(FArchive& Ar, FVector& Value)
	{
		Ar << Value.x;
		Ar << Value.y;
		Ar << Value.z;
	}
};

// FVector2 Serializers (x, y)
template <>
struct FArchiveSerializer<FVector2>
{
	static void Serialize(FArchive& Ar, FVector2& Value)
	{
		Ar << Value.X;
		Ar << Value.Y;
	}
};

template <>
struct FArchiveSerializer<FVector4>
{
	static void Serialize(FArchive& Ar, FVector4& Value)
	{
		Ar << Value.x;
		Ar << Value.y;
		Ar << Value.z;
		Ar << Value.w;
		
	}
};

template <>
struct FArchiveSerializer<FVertex>
{
	static void Serialize(FArchive& Ar, FVertex& Value)
	{
		Ar << Value.Pos;
		Ar << Value.Normal;
		Ar << Value.Color;
		Ar << Value.Tex;
		
	}
};

template <>
struct FArchiveSerializer<FStaticMeshSection>
{
	static void Serialize(FArchive& Ar, FStaticMeshSection& Value)
	{
		Ar << Value.FirstIndex;
		Ar << Value.IndexCount;
		Ar << Value.MaterialName;
		Ar << Value.MaterialAssetID;
	}
};

// FString Serializers (length 만큼)
template<>
struct FArchiveSerializer<FString>
{
	static void Serialize(FArchive& Ar, FString& Value)
	{
		uint64 Length = Ar.GetMode() == EArchiveMode::Write ? Value.Len() : 0;
		
		Ar << Length;

		if (Ar.GetMode() == EArchiveMode::Read)
		{
			Value.Resize(static_cast<int32>(Length));
		}

		if (Length > 0)
		{
			Ar.Serialize(Value.CStr(), Length);
		}
	}
};

// FName 전용 Serializers (FName <-> String)
template <>
struct FArchiveSerializer<FName>
{
	static void Serialize(FArchive& Ar, FName& Value)
	{
		FString NameString;

		if (Ar.GetMode() == EArchiveMode::Write)
		{
			NameString = Value.ToString();
		}

		Ar << NameString;

		if (Ar.GetMode() == EArchiveMode::Read)
		{
			Value = FName(NameString);
		}
	}
};

// FGuid 전용 Serializers (uint32, uint32, uint32, uint32)
template <>
struct FArchiveSerializer<FGuid>
{
	static void Serialize(FArchive& Ar, FGuid& Value)
	{
		Ar << Value.A;
		Ar << Value.B;
		Ar << Value.C;
		Ar << Value.D;
	}
};

// FAssetFileHeader 전용 Serializers (ver, type, id)
template <>
struct FArchiveSerializer<FAssetFileHeader>
{
	static void Serialize(FArchive& Ar, FAssetFileHeader& Value)
	{
		Ar << Value.Version;
		Ar << Value.AssetType;
		Ar << Value.AssetID;
	}
};

