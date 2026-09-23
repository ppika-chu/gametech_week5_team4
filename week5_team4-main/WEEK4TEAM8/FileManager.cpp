#include <fstream>
#include <sstream>
#include <algorithm>

#include "FileManager.h"

FFileManager::FFileManager()
	: FFileManager(kDefaultAssetsPath, kDefaultRootPath)
{
}

FFileManager::FFileManager(std::string_view fileDirPath)
	: FFileManager(fileDirPath, kDefaultRootPath)
{
}

FFileManager::FFileManager(std::string_view fileDirPath, std::string_view rootPath)
	: mFileDirPath(fileDirPath)
	, mRootPath(rootPath)
{
}

FString FFileManager::ReadFileToString(std::string_view fileName) const
{
    return ReadFileToString(
        std::filesystem::path(std::string(fileName)));
}

void FFileManager::WriteStringToFile(
    std::string_view fileName,
    std::string_view content) const
{
    WriteStringToFile(
        std::filesystem::path(std::string(fileName)),
        content);
}

std::filesystem::path FFileManager::ResolvePath(const std::filesystem::path& requestedPath) const
{
    // 파일 탐색기에서 받은 절대 경로는 그대로 사용한다.
    const std::filesystem::path resolvedPath =
        requestedPath.is_absolute()
        ? requestedPath
        : mFileDirPath / requestedPath;

    return std::filesystem::weakly_canonical(resolvedPath);
}

FString FFileManager::ReadFileToString(const std::filesystem::path& requestedPath) const
{
    const std::filesystem::path filePath = ResolvePath(requestedPath);

    std::ifstream fileStream(filePath, std::ios::in | std::ios::binary);

    if (!fileStream.is_open())
    {
        throw std::runtime_error(
            "Failed to open file for reading: " + filePath.string());
    }

    std::stringstream buffer;
    buffer << fileStream.rdbuf();

    return FString(buffer.str());
}

void FFileManager::WriteStringToFile(const std::filesystem::path& requestedPath,std::string_view content) const
{
    const std::filesystem::path filePath = ResolvePath(requestedPath);

    // Assets/SceneData가 없거나 하위 폴더를 선택한 경우 자동 생성
    //std::filesystem::create_directories(filePath.parent_path());

    std::ofstream fileStream(
        filePath,
        std::ios::out | std::ios::trunc);

    if (!fileStream.is_open())
    {
        throw std::runtime_error(
            "Failed to open file for writing: " + filePath.string());
    }

    fileStream << content;
}

bool FFileManager::IsUnderRoot(const std::filesystem::path& filePath) const
{
	return IsUnder(filePath, mRootPath);
}

bool FFileManager::IsUnderFileDir(const std::filesystem::path& filePath) const
{
	return IsUnder(filePath, mFileDirPath);
}

bool IsUnder(const std::filesystem::path& targetPath, const std::filesystem::path& basePath)
{
    const auto target =
        std::filesystem::weakly_canonical(targetPath);

    const auto base =
        std::filesystem::weakly_canonical(basePath);

    const auto relative =
        std::filesystem::relative(target, base);

    if (relative.empty())
    {
        return true;
    }

    const auto first = relative.begin();

    return first != relative.end() && *first != L"..";
}
