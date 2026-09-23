#pragma once

#include <filesystem>

#include "Core.h"

inline constexpr std::string_view kDefaultRootPath = ".\\";
inline constexpr std::string_view kDefaultAssetsPath = ".\\Assets\\";

class FFileManager
{
public:
	FFileManager();
	FFileManager(std::string_view fileDirPath);
	FFileManager(std::string_view fileDirPath, std::string_view rootPath);

	FString ReadFileToString(std::string_view fileName) const;
	void WriteStringToFile(std::string_view fileName, std::string_view content) const;

	// 파일 탐색기에서 받은 절대 경로용
	FString ReadFileToString(const std::filesystem::path& filePath) const;

	void WriteStringToFile(const std::filesystem::path& filePath,std::string_view content) const;

private:
	std::filesystem::path mFileDirPath;
	std::filesystem::path mRootPath;

	std::filesystem::path ResolvePath(const std::filesystem::path& filePath) const;

	bool IsUnderRoot(const std::filesystem::path& filePath) const;
	bool IsUnderFileDir(const std::filesystem::path& filePath) const;
};

bool IsUnder(const std::filesystem::path& filePath, const std::filesystem::path& rootPath);
