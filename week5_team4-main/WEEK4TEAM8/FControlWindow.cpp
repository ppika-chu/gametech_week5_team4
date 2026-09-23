#include "FControlWindow.h"
#include "Camera.h"
#include "Renderer.h"
#include "FLogManager.h"
#include "MathUtility.h"
#include "ImGui/imgui.h"
#include "ShowFlags.h"
#include "NativeFileDialog.h"
#include "FileManager.h"
#include "ObjectFactory.h"
#include "UTextComponent.h"
#include "UAtlasAnimationComponent.h"
#include "UStaticMeshComponent.h"
#include "World.h"
#include "SceneManager.h"
#include "enum.h"

void FControlWindow::Render(FSceneManager& SceneManager)
{
	ImGuiIO& io = ImGui::GetIO();
	UWorld* CurrentWorld = SceneManager.GetCurrentWorld();

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoCollapse;
	ImGui::Begin("Jungle Control Panel", nullptr, flags);

	ImGui::Text("Hello Jungle World!");

	/* Spawn Actor */
	// NOTE: This name array must be edited when adding new primitive types to EPrimitive enum.
	ImGui::SeparatorText("Spawn Actor");

	const char* ActorTypeNames[] = {
		"Sphere",
		"Cube",
		"Triangle",
		"GizmoArrow",
		"Circle",
		"SpotLight",
		"Explosion",
	};

	ImGui::Combo("Actor Type", &mSelectedTargetSpawnIndex, ActorTypeNames, IM_ARRAYSIZE(ActorTypeNames));

	if (ImGui::Button("Spawn"))
	{
		for (int32 i = 0; i < mSpawnCount; ++i)
		{
			const char* ActorTypeName = ActorTypeNames[mSelectedTargetSpawnIndex];

			AActor* NewActor = nullptr;
			if (strcmp(ActorTypeName, "Explosion") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();

				TSharedPtr<FSpriteAtlasAsset> ExplosionAtlas = FAssetManager::Get().GetAssetAs<FSpriteAtlasAsset>(FName("ExplosionSpriteAtlas"));

				UAtlasAnimationComponent* AnimComponent = FObjectFactory::ConstructObject<UAtlasAnimationComponent>(EPrimitive::EP_Plane, ExplosionAtlas);
				AnimComponent->SetRelativeLocation(FVector(0, 0, 0));
				AnimComponent->SetRelativeRotation(FRotator(0, 0, 0));
				AnimComponent->SetRelativeScale3D(FVector(1, 1, 1));
				AnimComponent->SetBillboard(true);
				AnimComponent->SetDepthState(true, false);
				AnimComponent->Play();

				NewActor->AddRootSceneComponent(AnimComponent);
			}
			else if (strcmp(ActorTypeName, "Sphere") == 0 || strcmp(ActorTypeName, "Cube") == 0 || strcmp(ActorTypeName, "Triangle") == 0 || strcmp(ActorTypeName, "GizmoArrow") == 0 || strcmp(ActorTypeName, "Circle") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();

				UStaticMeshComponent* MeshComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));
				MeshComponent->SetMesh(FAssetManager::Get().GetAssetAs<FStaticMeshAsset>(FName(std::format("{}Mesh", ActorTypeName)), true));

				NewActor->AddRootSceneComponent(MeshComponent);
			}
			else if (strcmp(ActorTypeName, "SpotLight") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<ASpotLight>();
			}
			else if (strcmp(ActorTypeName, "StaticMesh") == 0)
			{
				NewActor = FObjectFactory::ConstructObject<AActor>();

				UStaticMeshComponent* MeshComponent = FObjectFactory::ConstructObject<UStaticMeshComponent>(FVector(0, 0, 0), FRotator(0, 0, 0), FVector(1, 1, 1));

				NewActor->AddRootSceneComponent(MeshComponent);
			}
			else
			{
				UE_LOG_ERROR("Unknown actor class: %s", ActorTypeName);
			}

			if (NewActor)
			{
				CurrentWorld->AddActor(NewActor);
			}
		}
	}
	ImGui::SameLine();

	if (ImGui::InputInt("Number of spawn", &mSpawnCount))
	{
		mSpawnCount = FGenericPlatformMath::Max(1, mSpawnCount);
	}

	/*Scene Control*/
	ImGui::SeparatorText("Scene Control");

	const std::filesystem::path sceneDirectory = std::filesystem::absolute(std::filesystem::path(kDefaultAssetsPath) / std::filesystem::path(kSceneDataDir));

	void* ownerWindow = ImGui::GetMainViewport()->PlatformHandleRaw;

#if 0
	if (ImGui::Button("New scene"))
	{
		guiReference.ViewportClient->Reset();
		SceneManager.NewScene();
	}

	ImGui::SameLine();

	if (ImGui::Button("Save scene"))
	{
		try
		{
			// 저장 대화상자의 초기 폴더가 반드시 존재하도록 한다.
			//std::filesystem::create_directories(sceneDirectory);

			const std::optional<std::filesystem::path> selectedPath = FNativeFileDialog::SaveScene(sceneDirectory);

			// 취소 버튼을 누른 경우에는 아무 작업도 하지 않는다.
			if (selectedPath.has_value())
			{
				SceneManager.SaveScene(guiReference.EditorCamera, selectedPath.value(), *guiReference.FileManager);

				UE_LOG("Scene saved: %s", selectedPath->string().c_str());
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR(
				"Failed to save scene: %s",
				e.what());
		}
	}

	ImGui::SameLine();

	if (ImGui::Button("Load scene"))
	{
		try
		{
			//std::filesystem::create_directories(sceneDirectory);

			const std::optional<std::filesystem::path> selectedPath =
				FNativeFileDialog::OpenScene(sceneDirectory);

			// 취소한 경우에는 현재 씬과 카메라 상태를 건드리지 않는다.
			if (selectedPath.has_value())
			{
				SceneManager.LoadScene(guiReference.EditorCamera, selectedPath.value(), *guiReference.FileManager);

				// 파일 로드가 실행된 뒤에만 카메라를 초기화한다.
				guiReference.ViewportClient->Reset();

				UE_LOG(
					"Scene loaded: %s",
					selectedPath->string().c_str());
			}
		}
		catch (const std::exception& e)
		{
			UE_LOG_ERROR(
				"Failed to load scene: %s",
				e.what());
		}
	}

	/* Camera Control */
	ImGui::SeparatorText("Camera Control");

	FCamera& camera = guiReference.ViewportClient->GetCamera();
	URenderer* renderer = guiReference.GraphicsManager->GetRenderer();

	if (ImGui::BeginCombo("##ShowFlags", "Show Flags"))
	{
		// 표시 옵션은 표를 그대로 훑어 체크박스를 만든다.
		// 옵션을 추가할 때 ShowFlags.h의 GShowFlagInfos에만 한 줄 적으면 여기 바로 나온다.
		FShowFlags& showFlags = FShowFlags::Get();
		for (const FShowFlagInfo& flagInfo : GShowFlagInfos)
		{
			bool bEnabled = showFlags.IsEnabled(flagInfo.Flag);
			if (ImGui::Checkbox(flagInfo.Name, &bEnabled))
			{
				showFlags.SetEnabled(flagInfo.Flag, bEnabled);
			}
		}

		bool bOrthographic = guiReference.GraphicsManager->IsOrthographicTarget();
		if (ImGui::Checkbox("Orthogonal", &bOrthographic))
		{
			// Preserve the camera and ortho zoom; animate only the projection ratio.
			guiReference.GraphicsManager->StartProjectionTransition(bOrthographic);
		}

		ImGui::EndCombo();
	}
	{
		static constexpr int32 GridGapValues[] = { 1, 5, 10, 50, 100, 500 };
		static constexpr const char* GridGapLabels[] = { "(1)", "(5)", "(10)", "(50)", "(100)", "(500)" };
		constexpr int StepCount = IM_ARRAYSIZE(GridGapValues);
		const int32 GridGap = guiReference.GraphicsManager->GetGridGap();
		int SelectedIndex = 0;
		for (int i = 1; i < StepCount; ++i)
		{
			if (GridGap >= (GridGapValues[i - 1] + GridGapValues[i]) / 2.0f)
			{
				SelectedIndex = i;
			}
		}

		ImGui::Text("Grid Gap: %d", GridGap);
		const ImGuiStyle& Style = ImGui::GetStyle();
		const float FontSize = ImGui::GetFontSize();
		const float LabelWidth = ImGui::CalcTextSize("(500)").x;
		const float Width = (std::max)(ImGui::GetContentRegionAvail().x,
			(LabelWidth + Style.ItemInnerSpacing.x) * StepCount);
		const float Padding = LabelWidth * 0.5f;
		const ImVec2 Origin = ImGui::GetCursorScreenPos();
		const float TrackLeft = Origin.x + Padding;
		const float TrackWidth = Width - Padding * 2.0f;
		const float TrackY = Origin.y + FontSize;
		const float TrackHeight = FontSize * 0.3f;
		const float LabelY = TrackY + TrackHeight + Style.ItemInnerSpacing.y;
		ImGui::InvisibleButton("##GridGapSelector",
			ImVec2(Width, LabelY + FontSize - Origin.y));
		const bool bActive = ImGui::IsItemActive();
		const bool bHovered = ImGui::IsItemHovered();
		bool bChanged = false;
		if (bActive && ImGui::IsMouseDown(ImGuiMouseButton_Left))
		{
			// Snap to the closest displayed step, including when dragging past either end.
			const float Position = FMath::Clamp((io.MousePos.x - TrackLeft) / TrackWidth, 0.0f, 1.0f);
			SelectedIndex = static_cast<int>(Position * (StepCount - 1) + 0.5f);
			bChanged = true;
		}
		if (bChanged && GridGapValues[SelectedIndex] != GridGap)
		{
			guiReference.GraphicsManager->SetGridGap(GridGapValues[SelectedIndex]);
		}

		if (ImGui::IsItemVisible())
		{
			ImDrawList* DrawList = ImGui::GetWindowDrawList();
			const ImU32 TrackColor = ImGui::GetColorU32(bActive ? ImGuiCol_FrameBgActive :
				(bHovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg));
			const ImU32 HandleColor = ImGui::GetColorU32(bActive ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab);
			DrawList->AddRectFilled(ImVec2(TrackLeft, TrackY),
				ImVec2(TrackLeft + TrackWidth, TrackY + TrackHeight), TrackColor, Style.FrameRounding);
			for (int i = 0; i < StepCount; ++i)
			{
				const float X = TrackLeft + TrackWidth * i / (StepCount - 1);
				const ImU32 LabelColor = ImGui::GetColorU32(i == SelectedIndex ? ImGuiCol_Text : ImGuiCol_TextDisabled);
				DrawList->AddLine(ImVec2(X, TrackY), ImVec2(X, TrackY + TrackHeight), LabelColor);
				DrawList->AddText(ImVec2(X - ImGui::CalcTextSize(GridGapLabels[i]).x * 0.5f, LabelY),
					LabelColor, GridGapLabels[i]);
			}
			const float HandleX = TrackLeft + TrackWidth * SelectedIndex / (StepCount - 1);
			DrawList->AddTriangleFilled(ImVec2(HandleX - FontSize * 0.4f, Origin.y),
				ImVec2(HandleX + FontSize * 0.4f, Origin.y), ImVec2(HandleX, TrackY + TrackHeight), HandleColor);
		}
	}

	ImGui::Text("FOV     ");
	ImGui::SameLine();
	ImGui::SliderFloat("##FOV", &camera.mFovDegree, 0.0f, 180.0f);

	ImGui::Text("Sensitivity");
	ImGui::SameLine();
	ImGui::SliderFloat("##CameraSensitivity", &camera.Sensitivity, 0.01f, 1.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);

	// 1) 라벨 텍스트를 먼저 그리고 같은 줄로
	ImGui::Text("Location");
	ImGui::SameLine();

	// 2) 텍스트를 그린 "뒤"의 남은 폭을 기준으로 계산
	const float spacing = ImGui::GetStyle().ItemSpacing.x;
	const float itemWidth = (ImGui::GetContentRegionAvail().x - spacing * 2.0f) / 3.0f;

	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamLocX", &camera.Transform.Location.x, 0.1f, 10.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamLocY", &camera.Transform.Location.y, 0.1f, 10.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamLocZ", &camera.Transform.Location.z, 0.1f, 10.0f);

	ImGui::Text("Rotation");
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamRotX", &camera.Transform.Rotation.Roll, 0.1f, 180.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamRotY", &camera.Transform.Rotation.Pitch, 0.1f, 180.0f);
	ImGui::SameLine();
	ImGui::SetNextItemWidth(itemWidth);
	ImGui::DragFloat("##CamRotZ", &camera.Transform.Rotation.Yaw, 0.1f, 180.0f);

	/* Gizmo Control */
	ImGui::SeparatorText("Gizmo Control");

	// Display the current gizmo mode dropdown
	const char* gizmoModeNames[] = { "Translate", "Rotate", "Scale" };

	EGIZMO_TYPE currentGizmoType = guiReference.ViewportClient->mGizmo.GetOperation();
	int32 currentGizmoIndex = static_cast<int32>(currentGizmoType);
	if (ImGui::Combo("Gizmo Mode", &currentGizmoIndex, gizmoModeNames, IM_ARRAYSIZE(gizmoModeNames)))
	{
		if (currentGizmoIndex == 0)
		{
			guiReference.ViewportClient->mGizmo.SetOperation(EGIZMO_TYPE::TRANSLATE);
		}
		else if (currentGizmoIndex == 1)
		{
			guiReference.ViewportClient->mGizmo.SetOperation(EGIZMO_TYPE::ROTATE);
		}
		else if (currentGizmoIndex == 2)
		{
			guiReference.ViewportClient->mGizmo.SetOperation(EGIZMO_TYPE::SCALE);
		}
	}
	if (ImGui::Button("Next Gizmo Mode"))
	{
		guiReference.ViewportClient->mGizmo.SetOperation(static_cast<EGIZMO_TYPE>((currentGizmoIndex + 1) % 3));
	}
#endif
	ImGui::End();
}
