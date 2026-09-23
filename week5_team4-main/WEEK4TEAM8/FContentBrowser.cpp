#include "FContentBrowser.h"
#include "ImGui/imgui_internal.h"
#include "Assets.h"

void FContentBrowser::Initialize(const std::filesystem::path& InitDirectory)
{
	RootDirectory = InitDirectory;
	CurrentDirectory = InitDirectory;
	RefreshCache();
}

void FContentBrowser::SetEventHandler(FContentBrowserEventHandler* InEventHandler)
{
	EventHandler = InEventHandler;
}

void FContentBrowser::Render(const float BottomBarHeight)
{
	if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl) && ImGui::IsKeyPressed(ImGuiKey_Space, false))
	{
		ToggleDrawer();
	}

	//RenderBottomBar();

	if (bIsDrawerOpen)
	{
		RenderDrawer(BottomBarHeight);
	}
}

// Static Mesh 같은 경로, 같은 이름 파일 다시 import 하면
// 이름 뒤에 _2, _3 붙여서 반환하는 helper
static std::filesystem::path ResolveImportPath(const std::filesystem::path& InPath)
{
	// 같은 이름이 존재하지 않으면 그냥 그 경로로 진행
	if (!std::filesystem::exists(InPath))
	{
		return InPath;
	}

	// 같은 이름이 존재하는 경우 Suffix 1씩 늘려가며 check
	int32 Suffix = 1;
	std::filesystem::path Candidate;

	do
	{
		Candidate = InPath.parent_path() / (InPath.stem().string() + "_" + std::to_string(Suffix) + InPath.extension().string());
		++Suffix;

	} while (std::filesystem::exists(Candidate));

	return Candidate;
}

void FContentBrowser::RenderDrawer(const float BottomBarHeight)
{
	static bool bWasWindowFocused = true;

	HWND ActiveHWND = ::GetActiveWindow();

	bool bIsFocusedNow = (ActiveHWND != nullptr);
	bool bFoucusGained = (!bWasWindowFocused && bIsFocusedNow);
	bWasWindowFocused = bIsFocusedNow;

	// 프로그램 외부에서 .uasset을 수정/삭제한 경우 파일 캐쉬 갱신
	if (bFoucusGained)
	{
		std::error_code ec;

		EventHandler->RefreshContentBrowser(CurrentDirectory);

		if (!std::filesystem::exists(CurrentDirectory, ec) || !std::filesystem::is_directory(CurrentDirectory))
		{
			UE_LOG_WARN("Current directory not found: '%s', Reverting to RootDirectory.", CurrentDirectory);
			CurrentDirectory = CurrentDirectory.parent_path();
			RefreshCache();
		}
	}

	const ImGuiViewport* Viewport = ImGui::GetMainViewport();


	const ImVec2 DrawerPos = { Viewport->WorkPos.x, Viewport->WorkPos.y + Viewport->WorkSize.y - BottomBarHeight - DrawerHeight };
	const ImVec2 DrawerSize = { Viewport->WorkSize.x, DrawerHeight };

	ImGui::SetNextWindowPos(DrawerPos, ImGuiCond_Always);
	ImGui::SetNextWindowSize(DrawerSize, ImGuiCond_Appearing);
	ImGui::SetNextWindowViewport(Viewport->ID);

	const ImGuiWindowFlags DrawerFlags =
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoDocking;

	ImGui::PushStyleColor(ImGuiCol_WindowBg, IM_COL32(32, 32, 32, 245));

	if (ImGui::Begin("Content Drawer", &bIsDrawerOpen, DrawerFlags))
	{
		DrawerHeight = ImGui::GetWindowHeight();
		if (ImGui::Button("Import Texture2D"))
		{
			std::filesystem::path TargetPath;
			FAssetFileHeader Header;
			if (FNativeFileDialog::OpenFileDialog(CurrentDirectory, { FFileFilter{ L"Image Files", L"*.png;*.jpg;" } }, L"", TargetPath))
			{
				std::filesystem::path NewFilePath = CurrentDirectory / TargetPath.filename();
				NewFilePath.replace_extension(".uasset");   // CurrentDirectory 대신 원본 옆에 저장
				
				// 경로 중복되는지 확인
				NewFilePath = ResolveImportPath(NewFilePath);

				if (FTexture2DImporter::Import(TargetPath, NewFilePath, Header))
				{
					//NewAssetFiles.Emplace(FAssetFileEntry{ Header, NewFilePath });
					if (EventHandler)
					{
						EventHandler->OnNewAssetFile(Header, NewFilePath);
					}
					RefreshCache();
				}
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Import StaticMesh"))
		{
			std::filesystem::path TargetPath;
			FAssetFileHeader Header;

			if (FNativeFileDialog::OpenFileDialog(CurrentDirectory, { FFileFilter{ L"Obj Files", L"*.obj;" } }, L"", TargetPath))
			{
				std::filesystem::path NewFilePath = CurrentDirectory / TargetPath.filename();
				NewFilePath.replace_extension(".uasset");   // CurrentDirectory 대신 원본 옆에 저장

				// 경로 중복되는지 확인
				NewFilePath = ResolveImportPath(NewFilePath);

				if (FStaticMeshImporter::Import(TargetPath, NewFilePath, Header))
				{
					//NewAssetFiles.Emplace(FAssetFileEntry{ Header, NewFilePath });
					if (EventHandler)
					{
						EventHandler->OnNewAssetFile(Header, NewFilePath);
					}
					RefreshCache();
				}
			}

		}
		ImGui::SameLine();
		ImGui::Text(" | Current: %s", CurrentDirectory.string().c_str());

		ImGui::SameLine(ImGui::GetWindowWidth() - 70.0f);
		if (ImGui::Button("Refresh"))
		{
			EventHandler->RefreshContentBrowser(CurrentDirectory);
		}

		ImGui::Separator();

		if (ImGui::BeginChild("FolderTreePanel", ImVec2(220.0f, 0.0f), true))
		{
			ImGui::TextDisabled("FOLDERS");
			ImGui::Separator();

			RenderFolderNode(RootDirectory);
		}
		ImGui::EndChild();

		ImGui::SameLine();

		if (ImGui::BeginChild("AssetContentPanel", ImVec2(0.0f, 0.0f), true))
		{
			ImGui::Text("Current Path: %s", CurrentDirectory.string().c_str());
			ImGui::Separator();

			const float TileWidth = 80.0f;
			const float TileHeight = 80.0f;

			std::error_code ec;
			bool bIsRoot = std::filesystem::equivalent(CurrentDirectory, RootDirectory, ec);

			if (!bIsRoot && !ec)
			{
				ImGui::BeginGroup();
				{
					ImGui::InvisibleButton("##UpFolderBtn", ImVec2(TileWidth, TileHeight));

					bool bHovered = ImGui::IsItemHovered();
					bool bActive = ImGui::IsItemActive();
					ImVec2 PMin = ImGui::GetItemRectMin();
					ImVec2 PMax = ImGui::GetItemRectMax();
					ImDrawList* DrawList = ImGui::GetWindowDrawList();

					if (ImGui::IsItemClicked())
					{
						std::error_code clickEc;
						if (!std::filesystem::equivalent(CurrentDirectory, RootDirectory, clickEc))
						{
							CurrentDirectory = CurrentDirectory.parent_path();
							RefreshCache();
						}
					}

					if (bHovered || bActive)
					{
						ImU32 BgColor = bActive ? IM_COL32(255, 255, 255, 40) : IM_COL32(255, 255, 255, 20);
						DrawList->AddRectFilled(PMin, PMax, BgColor, 4.0f);
					}

					FEditorIconUtils::DrawFolderIcon(PMin, PMax, DrawList, bHovered, bActive);

					ImGui::SetCursorPosX(ImGui::GetCursorPosX() + (TileWidth - ImGui::CalcTextSize("..").x) * 0.5f);
					ImGui::TextUnformatted("..");
				}
				ImGui::EndGroup();

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("Go to parent directory (%s)", CurrentDirectory.parent_path().filename().string().c_str());
				}

				ImGui::SameLine();
			}

			const ImGuiStyle& Style = ImGui::GetStyle();

			const float WindowVisibleX2 = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;

			int ItemIndex = 0;

			for (const auto& Item : CachedItems)
			{
				const std::filesystem::path& Path = Item.Path;
				bool bIsDirectory = Item.bIsDirectory;
				const FString& DisplayName = Item.DisplayName;
				EAssetType AssetType = Item.AssetType;

				ImGui::PushID(ItemIndex++);

				ImGui::BeginGroup();
				{
					ImGui::InvisibleButton("##TileBtn", ImVec2(TileWidth, TileHeight));

					bool bHovered = ImGui::IsItemHovered();
					bool bActive = ImGui::IsItemActive();
					ImVec2 PMin = ImGui::GetItemRectMin();
					ImVec2 PMax = ImGui::GetItemRectMax();
					ImDrawList* DrawList = ImGui::GetWindowDrawList();

					if (bHovered || bActive)
					{
						ImU32 BgColor = bActive ? IM_COL32(255, 255, 255, 40) : IM_COL32(255, 255, 255, 20);
						DrawList->AddRectFilled(PMin, PMax, BgColor, 4.0f);
					}

					if (bIsDirectory)
					{
						if (ImGui::IsItemClicked())
						{
							std::filesystem::path NextDirectory = Path;
							CurrentDirectory = NextDirectory;
							RefreshCache();
							ImGui::EndGroup();
							ImGui::PopID();
							break;
						}

						FEditorIconUtils::DrawFolderIcon(PMin, PMax, DrawList, bHovered, bActive);
					}
					else
					{
						switch (AssetType)
						{
						case EAssetType::StaticMesh:
						{
							FEditorIconUtils::DrawObjIcon(PMin, PMax, DrawList, bHovered, bActive);
							break;
						}
						case EAssetType::Texture2D:
						{
							bool bDrawn = false;
							if (AssetManager)
							{
								std::string CanonicalKey = Path.lexically_normal().generic_string();
								FName AssetKey(CanonicalKey.c_str());

								TSharedPtr<FTexture2DAsset> TextureAsset = AssetManager->GetAssetAs<FTexture2DAsset>(FName(AssetKey), true);
								if (TextureAsset && TextureAsset->GetSRV())
								{
									ImTextureID TexID = (ImTextureID)TextureAsset->GetSRV().Get();
									ImVec2 ImgMin = ImVec2(PMin.x + 8.0f, PMin.y + 8.0f);
									ImVec2 ImgMax = ImVec2(PMax.x - 8.0f, PMax.y - 8.0f);
									DrawList->AddImage(TexID, ImgMin, ImgMax);
									DrawList->AddRect(ImgMin, ImgMax, 0x44FFFFFF, 2.0f);
									bDrawn = true;
								}
							}

							if (!bDrawn)
							{
								DrawList->AddRectFilled(ImVec2(PMin.x + 8.0f, PMin.y + 8.0f), ImVec2(PMax.x - 8.0f, PMax.y - 8.0f), 0xFF444444, 4.0f);
							}
							break;
						}
						case EAssetType::Material:
						{
							FEditorIconUtils::DrawMaterialIcon(PMin, PMax, DrawList, bHovered, bActive);
							break;
						}
						default:
						{
							FEditorIconUtils::DrawDocumentIcon(PMin, PMax, DrawList);
							break;
						}
						}
					}

					if (!bIsDirectory && ImGui::BeginDragDropSource())
					{
						/*std::string FullPath = Path.string();
						std::filesystem::path Test = std::filesystem::weakly_canonical(Path);*/
						std::string FullPath = Path.lexically_normal().generic_string();

						const auto& MetaInfo = FAssetManager::Get().GetMetaInfo(FName(FullPath));

						if (AssetType == EAssetType::StaticMesh)
						{
							//ImGui::SetDragDropPayload(AssetPayloadTags::StaticMesh, FullPath.c_str(), (FullPath.length() + 1) * sizeof(char));
							ImGui::SetDragDropPayload("ASSET_GUID_MESH", &MetaInfo.AssetID, sizeof(FGuid));
							ImGui::Text("Mesh: %s", DisplayName.c_str());
						}
						else if (AssetType == EAssetType::Texture2D)
						{
							ImGui::SetDragDropPayload("ASSET_GUID_TEXTURE", &MetaInfo.AssetID, sizeof(FGuid));
							ImGui::Text("Texture2D: %s", DisplayName.c_str());
						}
						else if (AssetType == EAssetType::Material)
						{
							ImGui::SetDragDropPayload("ASSET_GUID_MATERIAL", &MetaInfo.AssetID, sizeof(FGuid));
							ImGui::Text("Material: %s", DisplayName.c_str());
						}

						ImGui::EndDragDropSource();
					}

					std::string TruncatedName = DisplayName.ToString();
					if (TruncatedName.length() > 9)
					{
						TruncatedName = TruncatedName.substr(0, 7) + "..";
					}
					ImGui::TextWrapped("%s", TruncatedName.c_str());
				}
				ImGui::EndGroup();

				if (ImGui::IsItemHovered())
				{
					ImGui::SetTooltip("%s", DisplayName.c_str());
				}

				float LastItemX2 = ImGui::GetItemRectMax().x;
				float NextItemX2 = LastItemX2 + Style.ItemSpacing.x + TileWidth;

				if (NextItemX2 < WindowVisibleX2)
				{
					ImGui::SameLine();
				}

				ImGui::PopID();
			}
		}
		ImGui::EndChild();
	}
	ImGui::End();

	ImGui::PopStyleColor();
}

void FContentBrowser::RenderFolderNode(const std::filesystem::path& DirectoryPath)
{
	bool bHasSubDirectories = false;

	try
	{
		for (const auto& Entry : std::filesystem::directory_iterator(DirectoryPath))
		{
			if (Entry.is_directory())
			{
				bHasSubDirectories = true;
				break;
			}
		}
	}
	catch (...)
	{
		//UELOG 추가
	}

	ImGuiTreeNodeFlags NodeFlags =
		ImGuiTreeNodeFlags_OpenOnArrow |
		ImGuiTreeNodeFlags_OpenOnDoubleClick |
		ImGuiTreeNodeFlags_SpanAvailWidth;

	if (CurrentDirectory == DirectoryPath)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Selected;
	}

	if (!bHasSubDirectories)
	{
		NodeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}

	FString FolderName = DirectoryPath.filename().string();
	if (FolderName.IsEmpty())
	{
		FolderName = DirectoryPath.string();
	}

	FString PathString = DirectoryPath.string();

	std::string NodeHiddenID = FString("##").Append(PathString);
	bool bNodeOpen = ImGui::TreeNodeEx(NodeHiddenID.c_str(), NodeFlags);

	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		CurrentDirectory = DirectoryPath;
		RefreshCache();
	}

	ImGui::SameLine();

	const float IconSize = 14.0f;
	ImGui::Dummy(ImVec2(IconSize, IconSize));

	ImVec2 IMin = ImGui::GetItemRectMin();
	ImVec2 IMax = ImGui::GetItemRectMax();
	ImDrawList* DrawList = ImGui::GetWindowDrawList();

	ImU32 TabColor = 0xFF80C0E0;
	ImU32 BodyColor = 0xFF90D0F8;

	DrawList->AddRectFilled(
		ImVec2(IMin.x + 1.0f, IMin.y + 1.0f),
		ImVec2(IMin.x + 7.0f, IMin.y + 4.0f),
		TabColor, 1.0f
	);

	DrawList->AddRectFilled(
		ImVec2(IMin.x + 1.0f, IMin.y + 3.5f),
		ImVec2(IMax.x - 1.0f, IMax.y - 1.0f),
		BodyColor, 2.0f
	);
	DrawList->AddRect(
		ImVec2(IMin.x + 1.0f, IMin.y + 3.5f),
		ImVec2(IMax.x - 1.0f, IMax.y - 1.0f),
		0x44000000, 2.0f, 0, 1.0f
	);

	ImGui::SameLine();
	ImGui::TextUnformatted(FolderName.c_str());

	if (bNodeOpen && bHasSubDirectories)
	{
		try
		{
			for (const auto& Entry : std::filesystem::directory_iterator(DirectoryPath))
			{
				if (Entry.is_directory())
				{
					RenderFolderNode(Entry.path());
				}
			}
		}
		catch (...) {}

		ImGui::TreePop();
	}
}

void FContentBrowser::RefreshCache()
{
	CachedItems.Empty();

	std::error_code ec;

	if (!std::filesystem::exists(CurrentDirectory, ec) || !std::filesystem::is_directory(CurrentDirectory))
	{
		UE_LOG_WARN("Current directory not found: '%s', Reverting to RootDirectory.", CurrentDirectory);
		CurrentDirectory = RootDirectory;

		if (!std::filesystem::exists(RootDirectory, ec))
		{
			std::filesystem::create_directories(RootDirectory, ec);
		}
	}

	try
	{
		for (const auto& Entry : std::filesystem::directory_iterator(CurrentDirectory))
		{
			const std::filesystem::path& Path = Entry.path();
			bool bIsDirectory = Entry.is_directory();

			if (!bIsDirectory && Path.extension() != ".uasset")
			{
				continue;
			}

			FContentItem Item;
			Item.Path = Path;
			Item.bIsDirectory = bIsDirectory;
			Item.DisplayName = bIsDirectory ? Path.filename().string() : Path.stem().string();

			if (!bIsDirectory)
			{
				try
				{
					FWindowsBinReader Reader(Path);
					FAssetFileHeader Header;
					Reader << Header;
					Item.AssetType = Header.AssetType;
				}
				catch (...)
				{
					continue;
				}
			}

			CachedItems.Add(Item);
		}
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("Failed to iterate directory: %s", e.what());
	}
}
