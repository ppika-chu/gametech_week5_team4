#pragma once

#include "Core.h"
#include "FGuid.h"
#include "FName.h"
#include "FArchive.h"

enum class EAssetType
{
	StaticMesh,
	Texture2D,
	Font,
	FontAtlas,
	SpriteAtlas,
	Material,
	None
};

class FAsset
{
public:
	FAsset() = default;
	FAsset(const FGuid& InAssetID, const FName& InAssetName, EAssetType InAssetType)
		: AssetID(InAssetID)
		, AssetName(InAssetName)
		, AssetType(InAssetType)	
	{
	}

	virtual ~FAsset() = default;

	inline const FGuid& GetAssetID() const { return AssetID; }
	inline const FName& GetAssetName() const { return AssetName; }
	inline EAssetType GetAssetType() const { return AssetType; }

protected:
	FGuid AssetID;
	FName AssetName;
	EAssetType AssetType;
};

class FAssetSource
{
public:
	virtual ~FAssetSource() = default; 

	virtual TSharedPtr<FArchive> CreateArchive() = 0;
};

class FAssetLoader
{
public:
	virtual ~FAssetLoader() = default;

	virtual TSharedPtr<FAsset> LoadAsset(const FGuid& AssetID, const FName& AssetName, FArchive& Ar) = 0;
	virtual void UnloadAsset(TSharedPtr<FAsset> Asset) = 0;
	virtual EAssetType GetAssetType() const = 0;
};

struct FAssetFileHeader
{
	uint32 Version;
	EAssetType AssetType;
	FGuid AssetID;
};

