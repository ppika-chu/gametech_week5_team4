#include "FObjViewer.h"

#include "AssetFileIOs.h"
#include "ObjectFactory.h"
#include "Actor.h"
#include "SceneManager.h"
#include "World.h"
#include "UStaticMeshComponent.h"
#include "NativeFileDialog.h"
#include "FLogManager.h"
#include "GraphicsManager.h"
#include "ShowFlags.h"
#include "FAssetManager.h"
#include "FArchive.h"
#include "UStaticMesh.h"
#include "Assets.h"
#include "FStaticMeshBuilder.h"
#include "FMeshDescription.h"

void FObjViewer::Initialize(FSceneManager& InSceneManager, URenderer& Renderer, FFileManager& InFileManager)
{
	mSceneManager = &InSceneManager;
	FShowFlags::Get().SetEnabled(EShowFlag::UUIDText, false); 
	mRenderer = &Renderer;
}

void FObjViewer::UpdateObjGUI(FGraphicsManager& InGraphicsManager)
{
	ImGui::Begin("OBJ Viewer");

	if (mViewerComponent && mViewerComponent->GetMesh())
	{
		const auto& meshAsset = mViewerComponent->GetMesh();

		ImGui::Text("Vertices: %u", meshAsset->GetVertices().Num());
		ImGui::Text("Triangles: %u", meshAsset->GetIndices().Num() / 3);
	}

	if (mViewerActor)
	{
		ImGui::SeparatorText("Transform");
		const FTransform& originalTransform = mViewerActor->GetTransform();

		FVector translationInput = originalTransform.Location;
		FVector rotationInput = {
			originalTransform.Rotation.Roll,
			originalTransform.Rotation.Pitch,
			originalTransform.Rotation.Yaw
		};
		FVector scaleInput = originalTransform.Scale;

		if (ImGui::DragFloat3("Translation", &translationInput.x, 0.1f))
		{
			mViewerActor->SetLocation(translationInput);
		}
		if (ImGui::DragFloat3("Rotation", &rotationInput.x, 0.1f))
		{
			mViewerActor->SetRotation({ rotationInput.y,  rotationInput.z,  rotationInput.x });
		}
		if (ImGui::DragFloat3("Scale", &scaleInput.x, 0.1f, MIN_SCALE, FLT_MAX, "%.3f", ImGuiSliderFlags_AlwaysClamp))
		{
			mViewerActor->SetScale(scaleInput);
		}

		if (ImGui::Button("Reset Transform") && mViewerActor)
		{
			mViewerActor->SetLocation(FVector(0.0f, 0.0f, 0.0f));
			mViewerActor->SetRotation(FRotator(0.0f, 0.0f, 0.0f));
			mViewerActor->SetScale(FVector(1.0f, 1.0f, 1.0f));
		}

		ImGui::SeparatorText("Actions");
		if (ImGui::Button("Clear View") && mViewerActor)
		{
			mSceneManager->ResetSelectedActor();
			mSceneManager->GetCurrentWorld()->RemoveActor(mViewerActor->UUID);
			mViewerActor = nullptr;
		}
	
		if (bOpenedFromObj && !mLoadedFilePath.IsEmpty())
		{
			if (ImGui::Button("Export UAsset"))
			{
				std::filesystem::path TargetPath;
				FAssetFileHeader Header;

				if (FNativeFileDialog::SaveFileDialog(kDefaultOBJPath, { FFileFilter{ L"UAsset Files", L"*.uasset;" } }, L"", TargetPath))
				{
					if (FStaticMeshImporter::Export(std::filesystem::path(mLoadedFilePath.CStr()), TargetPath,
						Header, mViewerActor->GetTransform(), mViewerComponent->GetColor()))
					{
						FAssetManager::Get().ScanDirectory("Assets", *mRenderer);

					}
				}
			}
		}
	}


	ImGui::SeparatorText("File Open");
	if (ImGui::Button("Open OBJ"))
	{
		std::filesystem::path TargetPath;

		try
		{
			if (FNativeFileDialog::OpenFileDialog(kDefaultOBJPath,
				{ FFileFilter{ L"OBJ Files", L"*.obj" } }, L"obj", TargetPath))
			{
				OpenObj(TargetPath);
				UE_LOG("Successed to load: %s", TargetPath.string().c_str());
				bOpenedFromObj = true;
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR("Failed to load: %s", e.what());
		}
	}

	if (ImGui::Button("Open UAsset"))
	{
		std::filesystem::path targetPath;

		try
		{
			if (FNativeFileDialog::OpenFileDialog(kDefaultOBJPath,
				{ FFileFilter{ L"UAsset Files", L"*.uasset" } }, L"obj", targetPath))
			{
				OpenStaticMeshAsset(targetPath);
				UE_LOG("Successed to load: %s", targetPath.string().c_str());
				bOpenedFromObj = false;
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR("Failed to load: %s", e.what());
		}
	}

	ImGui::SeparatorText("View Mode");
	bool bGrid = FShowFlags::Get().IsEnabled(EShowFlag::Grid);
	if (ImGui::Checkbox("Grid", &bGrid))
	{
		FShowFlags::Get().SetEnabled(EShowFlag::Grid, bGrid);
	}

	if (mViewerActor)
	{
		ImGui::SeparatorText("Materials");
		if (mViewerComponent && mViewerComponent->GetMesh())
		{
			const TSharedPtr<FStaticMeshAsset>& MeshAsset = mViewerComponent->GetMesh();

			for (int32 SectionIndex = 0; SectionIndex < MeshAsset->GetSections().Num(); ++SectionIndex)
			{
				const FStaticMeshSection& Section = MeshAsset->GetSections()[SectionIndex];
				TSharedPtr<FMaterialAsset> Material = FAssetManager::Get().GetAssetAs<FMaterialAsset>(Section.MaterialAssetID, true);

				ImGui::Text("Section %d", SectionIndex);

				if (!Material)
				{
					ImGui::TextDisabled("Material: None");
					continue;
				}

				ImGui::Text("Material: %s", Material->GetAssetName().ToString().CStr());

				TSharedPtr<FTexture2DAsset> Texture = Material->GetDiffuseTexture();
				if (!Texture)
				{
					ImGui::TextDisabled("Diffuse Texture: None");
					continue;
				}

				ImGui::Text("Diffuse Texture: %s", Texture->GetAssetName().ToString().CStr());

				ImGui::Image(reinterpret_cast<ImTextureID>(Texture->GetSRV().Get()), ImVec2(96.0f, 96.0f));
			}
		}
	}

	ImGui::End();

}

void FObjViewer::OpenObj(const std::filesystem::path& FilePath)
{
	FStaticMeshPayload Payload;
	if (!FStaticMeshFileIO::Load(FilePath, Payload))
	{
		UE_LOG_ERROR("Failed to load mesh: %s", FilePath.string().c_str());
		return;
	}

	FStaticMeshBuildData BuildData;
	if (!FStaticMeshBuilder::Build(Payload.MeshDescription, BuildData))
	{
		UE_LOG_ERROR("Failed to build mesh: %s", FilePath.string().c_str());
		return;
	}

	if (!mRenderer || !BuildRuntimeMaterials(FilePath, Payload, BuildData))
	{
		UE_LOG_ERROR("Failed to build OBJ material assets: %s", FilePath.string().c_str());
		return;
	}

	mLoadedFilePath = FString(FilePath.string());

	if (!mSceneManager)
	{
		return;
	}

	if (mViewerActor)
	{
		mSceneManager->ResetSelectedActor();

		mSceneManager->GetCurrentWorld()->RemoveActor(mViewerActor->UUID);
		mViewerActor = nullptr;
	}

	mViewerActor = FObjectFactory::ConstructObject<AActor>();

	const FName RuntimeAssetName(std::filesystem::weakly_canonical(FilePath).string());

	TSharedPtr<FStaticMeshAsset> MeshAsset =
		MakeShared<FStaticMeshAsset>(
			FGuid::NewGuid(),
			RuntimeAssetName,
			*mRenderer,
			BuildData);

	UStaticMeshComponent* objComponent =
		FObjectFactory::ConstructObject<UStaticMeshComponent>(
			FVector(0, 0, 0),
			FRotator(0, 0, 0),
			FVector(1, 1, 1));

	objComponent->SetMesh(MeshAsset);
	mViewerComponent = objComponent;
	mViewerActor->AddRootSceneComponent(objComponent);
	mSceneManager->GetCurrentWorld()->AddActor(mViewerActor);
}

bool FObjViewer::BuildRuntimeMaterials(const std::filesystem::path& ObjPath,
	const FStaticMeshPayload& Payload, FStaticMeshBuildData& InOutBuildData)
{
	const std::filesystem::path ObjDirectory = ObjPath.parent_path();

	for (const FObjMaterialInfo& Material : Payload.Materials)
	{
		FGuid DiffuseTextureID;

		if (Material.DiffuseTexturePath.Len() != 0)
		{
			const std::filesystem::path TexturePath =
				(ObjDirectory / Material.DiffuseTexturePath.CStr()).lexically_normal().generic_string();

			const FName TextureAssetName(TexturePath.string());

			TSharedPtr<FTexture2DAsset> TextureAsset =
				FAssetManager::Get().GetAssetAs<FTexture2DAsset>(
					TextureAssetName, true);

			if (!TextureAsset)
			{
				FImagePayload ImagePayload;
				if (!FImageFileIO::Load(TexturePath, ImagePayload))
				{
					UE_LOG_ERROR("Failed to load diffuse texture: %s",TexturePath.string().c_str());
					return false;
				}

				D3D11_TEXTURE2D_DESC TextureDesc = {};
				TextureDesc.Width = static_cast<uint32>(ImagePayload.Width);
				TextureDesc.Height = static_cast<uint32>(ImagePayload.Height);
				TextureDesc.MipLevels = 1;
				TextureDesc.ArraySize = 1;
				TextureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				TextureDesc.SampleDesc.Count = 1;
				TextureDesc.Usage = D3D11_USAGE_IMMUTABLE;
				TextureDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

				auto Texture = mRenderer->CreateTexture2D(TextureDesc, ImagePayload.ImageData.Data());
				auto SRV = mRenderer->CreateShaderResourceView(Texture);

				TextureAsset = MakeShared<FTexture2DAsset>(
					FGuid::NewGuid(),
					TextureAssetName,
					Texture,
					SRV);

				FAssetManager::Get().RegisterAsset(TextureAsset);
			}

			DiffuseTextureID = TextureAsset->GetAssetID();
		}

		const FName MaterialAssetName(ObjPath.string() + "::" + Material.Name.CStr());

		TSharedPtr<FMaterialAsset> MaterialAsset =
			FAssetManager::Get().GetAssetAs<FMaterialAsset>(
				MaterialAssetName, true);

		if (!MaterialAsset)
		{
			MaterialAsset = MakeShared<FMaterialAsset>(
				FGuid::NewGuid(),
				MaterialAssetName,
				Material.DiffuseColor,
				Material.DiffuseColor,
				FVector(0.0f),
				DiffuseTextureID,
				FGuid(),
				FGuid(),
				Material.Opacity);

			FAssetManager::Get().RegisterAsset(MaterialAsset);
		}

		for (FStaticMeshSection& Section : InOutBuildData.Sections)
		{
			if (Section.MaterialName == Material.Name)
			{
				Section.MaterialAssetID = MaterialAsset->GetAssetID();
			}
		}
	}
	return true;
}

void FObjViewer::OpenStaticMeshAsset(const std::filesystem::path& FilePath)
{
	FAssetFileHeader Header;

	try
	{
		FWindowsBinReader Reader(FilePath);
		Reader << Header;
	}
	catch (const std::exception& e)
	{
		UE_LOG_ERROR("Failed to read asset: %s", FilePath.string().c_str());
		return;
	}

	if (Header.AssetType != EAssetType::StaticMesh)
	{
		UE_LOG_ERROR("Selected file is not a StaticMesh: %s", FilePath.string().c_str());
		return;
	}

	TSharedPtr<FStaticMeshAsset> MeshAsset = 
		FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(Header.AssetID, true);

	if (!MeshAsset)
	{
		UE_LOG_ERROR("Failed to load StaticMesh asset: %s", FilePath.string().c_str());
		return;
	}

	if (mViewerActor)
	{
		mSceneManager->ResetSelectedActor();
		mSceneManager->GetCurrentWorld()->RemoveActor(mViewerActor->UUID);
		mViewerActor = nullptr;
	}

	UStaticMesh* StaticMesh = FObjectFactory::ConstructUnInitializedObject<UStaticMesh>();

	StaticMesh->SetCookedStaticMeshAsset(MeshAsset, FString(FilePath.string()), {});

	UStaticMeshComponent* Component = FObjectFactory::ConstructObject<UStaticMeshComponent>(FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));
	Component->SetMesh(MeshAsset);

	mViewerActor = FObjectFactory::ConstructObject<AActor>();
	mViewerActor->AddRootSceneComponent(Component);
	mSceneManager->GetCurrentWorld()->AddActor(mViewerActor);

	mViewerComponent = Component;
	mLoadedFilePath = FString(FilePath.string());
}
