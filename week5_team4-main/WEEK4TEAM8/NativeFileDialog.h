#pragma once

#include "TArray.h"
#include <Windows.h>
#include <filesystem>
#include <optional>

struct FFileFilter
{
	std::wstring Description; // 화면에 표시할 설명
	std::wstring Pattern; // 파일 확장자 패턴, 예: "*.txt"
};

class FNativeFileDialog
{
public:
	static void Initialize(HWND OwnerWindow);

	static bool OpenFileDialog(const std::filesystem::path& initialDirectory, const TArray<FFileFilter>& filter, const std::wstring& defaultExtension, std::filesystem::path& outPath);
	static bool SaveFileDialog(const std::filesystem::path& initialDirectory, const TArray<FFileFilter>& filter, const std::wstring& defaultExtension, std::filesystem::path& outPath);

    static std::optional<std::filesystem::path> OpenScene(const std::filesystem::path& initialDirectory);
    static std::optional<std::filesystem::path> SaveScene(const std::filesystem::path& initialDirectory);

private:
	static HWND sOwnerWindow;
};