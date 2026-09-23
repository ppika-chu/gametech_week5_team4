#include "NativeFileDialog.h"

#include <Windows.h>
#include <commdlg.h>

constexpr DWORD kPathBufferSize = 1024;

HWND FNativeFileDialog::sOwnerWindow = nullptr;

namespace
{
	std::wstring MakeFilterString(const TArray<FFileFilter>& filter)
	{
		std::wstring FullFilterString;
		for (const auto& Filter : filter)
		{
			FullFilterString += Filter.Description;
			FullFilterString.push_back(L'\0');
			FullFilterString += Filter.Pattern;
			FullFilterString.push_back(L'\0');
		}
		FullFilterString.push_back(L'\0');
		return FullFilterString;
	}
}

void FNativeFileDialog::Initialize(HWND OwnerWindow)
{
	sOwnerWindow = OwnerWindow;
}

bool FNativeFileDialog::OpenFileDialog(const std::filesystem::path& initialDirectory, const TArray<FFileFilter>& filter, const std::wstring& defaultExtension, std::filesystem::path& outPath)
{
	wchar_t PathBuffer[kPathBufferSize]{};

	std::wstring FullFilterString = MakeFilterString(filter);

    OPENFILENAMEW Dialog{};
    Dialog.lStructSize = sizeof(Dialog);
    Dialog.hwndOwner = sOwnerWindow;
    Dialog.lpstrFile = PathBuffer;
    Dialog.nMaxFile = kPathBufferSize;
    Dialog.lpstrFilter = FullFilterString.c_str();
    Dialog.nFilterIndex = 1;
    Dialog.lpstrInitialDir = initialDirectory.c_str();
    Dialog.lpstrDefExt = defaultExtension.c_str();
    Dialog.Flags =
        OFN_EXPLORER |
        OFN_FILEMUSTEXIST |
        OFN_PATHMUSTEXIST |
        OFN_NOCHANGEDIR;

    if(GetOpenFileNameW(&Dialog) != FALSE)
    {
        outPath = PathBuffer;
        return true;
    }
    else
    {
        return false;
    }
}

bool FNativeFileDialog::SaveFileDialog(const std::filesystem::path& initialDirectory, const TArray<FFileFilter>& filter, const std::wstring& defaultExtension, std::filesystem::path& outPath)
{
	wchar_t PathBuffer[kPathBufferSize]{};

	std::wstring FullFilterString = MakeFilterString(filter);

	OPENFILENAMEW Dialog{};
	Dialog.lStructSize = sizeof(Dialog);
	Dialog.hwndOwner = sOwnerWindow;
	Dialog.lpstrFile = PathBuffer;
	Dialog.nMaxFile = kPathBufferSize;
	Dialog.lpstrFilter = FullFilterString.c_str();
	Dialog.nFilterIndex = 1;
	Dialog.lpstrInitialDir = initialDirectory.c_str();
	Dialog.lpstrDefExt = defaultExtension.c_str();
	Dialog.Flags =
		OFN_EXPLORER |
		OFN_PATHMUSTEXIST |
		OFN_OVERWRITEPROMPT |
		OFN_NOCHANGEDIR;

	if(GetSaveFileNameW(&Dialog) != FALSE)
	{
		outPath = PathBuffer;
		return true;
	}
	else
	{
		return false;
	}
}

std::optional<std::filesystem::path> FNativeFileDialog::OpenScene(const std::filesystem::path& initialDirectory)
{
	std::filesystem::path SelectedPath;

	if (OpenFileDialog(initialDirectory, { FFileFilter{L"Scene Files (*.Scene)", L"*.Scene"}, FFileFilter{L"All Files (*.*)", L"*.*"} }, L"Scene", SelectedPath))
	{
        return SelectedPath;
	}
	else
	{
		return std::nullopt;
	}
}

std::optional<std::filesystem::path> FNativeFileDialog::SaveScene(const std::filesystem::path& initialDirectory)
{
	std::filesystem::path SelectedPath;

	if (SaveFileDialog(initialDirectory, { FFileFilter{L"Scene Files (*.Scene)", L"*.Scene"}, FFileFilter{L"All Files (*.*)", L"*.*"} }, L"Scene", SelectedPath))
	{
		return SelectedPath;
	}
	else
	{
		return std::nullopt;
	}
}

