#pragma once

#include "Core.h"
#include "FAsset.h"
#include "TMap.h"
#include "TArray.h"
#include "FGuid.h"

class URenderer;

struct FAssetStats
{
	uint32 RegisteredCount = 0;
	uint32 LoadedCount = 0;

	uint32 RegisteredStaticMesh = 0;
	uint32 RegisteredTexture2D = 0;
	uint32 RegisteredMaterial = 0;

	uint32 LoadedStaticMesh = 0;
	uint32 LoadedTexture2D = 0;
	uint32 LoadedMaterial = 0;
};

struct FAssetMetaInfo
{
	FGuid AssetID;
	EAssetType AssetType;
	FName AssetName;
	uint64 PayloadOffset;
	TSharedPtr<FAssetLoader> AssetLoader;
	TSharedPtr<FAssetSource> AssetSource;
};

class FAssetManager
{
public:
	static FAssetManager& Get();

	void RegisterAsset(const FName& AssetName, const TSharedPtr<FAssetLoader>& AssetLoader, const TSharedPtr<FAssetSource>& AssetSource);
	void RegisterAsset(const FGuid& AssetID, const FName& AssetName, const TSharedPtr<FAssetLoader>& AssetLoader, const TSharedPtr<FAssetSource>& AssetSource);
	void RegisterAsset(const TSharedPtr<FAsset>& Asset);
	void UnregisterAsset(const FName& AssetName);

	void PurgeStaleAssetsInDirectory(const std::filesystem::path& Directory);

	// 프로그램 시작 시에 호출하여 Directory 스캔하는 함수
	void ScanDirectory(const std::filesystem::path& RootDir, URenderer& Renderer);

	TSharedPtr<FAsset> LoadAsset(const FName& AssetName);
	TSharedPtr<FAsset> LoadAsset(const FGuid& AssetID);

	TSharedPtr<FAsset> GetAsset(const FName& AssetName, bool loadIfNotLoaded = false);
	TSharedPtr<FAsset> GetAsset(const FGuid& AssetID, bool loadIfNotLoaded = false);

	template <typename T>
	TSharedPtr<T> GetAssetAs(const FName& AssetName, bool loadIfNotLoaded = false)
	{
		TSharedPtr<FAsset> asset = GetAsset(AssetName, loadIfNotLoaded);
		if (asset)
		{
			return std::static_pointer_cast<T>(asset);
		}

		return nullptr;
	}

	template <typename T>
	TSharedPtr<T> GetAssetAs(const FGuid& AssetID, bool loadIfNotLoaded = false)
	{
		TSharedPtr<FAsset> asset = GetAsset(AssetID, loadIfNotLoaded);
		if (asset)
		{
			return std::static_pointer_cast<T>(asset);
		}
		return nullptr;
	}

	void UnloadAsset(const FName& AssetName);
	void UnloadAsset(const FGuid& AssetID);

	template <typename Func>
	void ForEachMetaInfo(Func&& func)
	{
		for (auto& pair : AssetMetaInfos)
		{
			func(pair.second);
		}
	}

	FAssetStats GetStats() const { return CachedStats; }
	void RebuildStats();

	inline const FAssetMetaInfo& GetMetaInfo(const FGuid& InGuid)
	{
		if (!AssetMetaInfos.Contains(InGuid))
		{
			throw std::runtime_error("No invalid guid");
		}

		return AssetMetaInfos[InGuid];
	}

	inline const FAssetMetaInfo& GetMetaInfo(const FName& InName)
	{
		if (!NameToAssetID.Contains(InName))
		{
			throw std::runtime_error("No invalid name");
		}

		return GetMetaInfo(NameToAssetID[InName]);
	}

private:
	TMap<FName, FGuid> NameToAssetID;
	TMap<FGuid, FAssetMetaInfo> AssetMetaInfos;
	TMap<FGuid, TSharedPtr<FAsset>> LoadedAssets;

	mutable FAssetStats CachedStats;
};

inline FString NormalizeAssetPath(const std::filesystem::path& InPath)
{
	std::string GenericPath = InPath.generic_string();

	size_t AssetsIndex = GenericPath.find("Assets");
	if (AssetsIndex != std::string::npos)
	{
		GenericPath = GenericPath.substr(AssetsIndex);
	}

	std::filesystem::path CleanPath(GenericPath);
	CleanPath.replace_extension("");

	return FString(CleanPath.generic_string());
}