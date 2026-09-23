#pragma once

#include <string_view>
#include <filesystem>
#include "SceneData.h"
#include "TArray.h"
#include "RenderInfo.h"
#include "enum.h"
#include "FAssetManager.h"
#include "FContentBrowser.h"
#include "FObjViewer.h"

inline constexpr std::string_view kSceneDataDir = "SceneData\\";
inline constexpr std::string_view kSceneDataSuffix = ".Scene";

class FFileManager;
class FFrameTimer;
class FEditorViewportClient;
class FGraphicsManager;
class UWorld;
class URenderer;
struct FViewport;
struct FEditorLayout;
struct FEditorViewport;
class UStaticMesh;

struct FGuiReference
{
	FCamera* EditorCamera;
	FFrameTimer* FrameTimer;
	FGraphicsManager* GraphicsManager;
	FEditorViewportClient* ViewportClient;
	FFileManager* FileManager;
	FAssetManager* AssetManager;
	FEditorLayout* EditorLayout;
	FEditorViewport* Viewports;
	int32 ViewportCount = 0;
};

struct FGuiInputField
{
	/* Spawn Actor */
	EPrimitive PrimitiveType = EPrimitive::EP_Cube;
	int32 SpawnCount = 1;

	/* Scene Control */
	char SceneName[512] = "Default";

	/* Object Lists */
	TArray<UObject*> SortedObjectLists;
	uint64 LastGUObjectRevision = -1;

	// Spawn Actor용
	UStaticMesh* SelectedStaticMesh = nullptr; 
};

class FSceneManager : public FContentBrowserEventHandler
{
public:
	FSceneManager();
	~FSceneManager();

	void OnNewAssetFile(const FAssetFileHeader& Header, const std::filesystem::path& FilePath) override;
	void OnDeleteAssetFile(const std::filesystem::path& FilePath) override;
	void RefreshContentBrowser(const std::filesystem::path& TargetDirectory) override;

	void Tick(float deltaTime);
	void Render(float deltaTime, FRenderCollector& outCollector);
	void UpdateGUI(const FGuiReference& guiReference);

	const TArray<FRenderInfo> GetAxisRenderInfos();

	// Clear world
	void NewScene();
	void DeleteScene();

	// 파일 탐색기용 오버로드
	void SaveScene(FCamera* Camera, const std::filesystem::path& scenePath, const FFileManager& fileManager);
	void LoadScene(FCamera* Camera, const std::filesystem::path& scenePath, const FFileManager& fileManager);

	UWorld* GetCurrentWorld() const { return mCurrentWorld; }

	AActor* GetSelectedActor() const { return mSelectedActor; }
	bool IsActorSelected() const { return mSelectedActor != nullptr; }
	void SetSelectedActor(AActor* actor);
	void ResetSelectedActor() { mSelectedActor = nullptr; }

	float GetPanelWidth() const;
	float GetViewportX() const { return mViewportX; }
	float GetViewportY() const { return mViewportY; }
	float GetViewportWidth() const { return mViewportWidth; }
	float GetViewportHeight() const { return mViewportHeight; }

private:
	static constexpr float MIN_WIDTH_RATIO = 0.2f;
	static constexpr float MAX_WIDTH_RATIO = 0.6f;

	static constexpr float CONTROL_PANEL_HEIGHT_RATIO = 0.4f;
	static constexpr float WINDOW_PROPERTY_HEIGHT_RATIO = 0.3f;

	float mPanelWidth;
	float mViewportX;
	float mViewportY;
	float mViewportWidth;
	float mViewportHeight;
	float mBottomBarHeight;

	UWorld* mCurrentWorld = nullptr;
	AActor* mSelectedActor = nullptr;
	FGuiInputField mGuiInputField;
	FContentBrowser mContentBrowser;
	URenderer* mRenderer = nullptr;

	void updateControlPanelGUI(const FGuiReference& guiReference);

	//TODO: PropertyWindow에 표시하는 정보를 다루는 구조체 및 시스템이 후에 필요하다.
	//지금은 하드코딩
	void updatePropertyWindowGUI(const FGuiReference& guiReference);
	void updateObjectListPanelGUI(const FGuiReference& guiReference);
};
