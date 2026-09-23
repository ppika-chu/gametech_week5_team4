#pragma once

#include "Core.h"

class FSceneManager;

class FControlWindow
{
public:
	void Render(FSceneManager& SceneManager);

private:
	int32 mSelectedTargetSpawnIndex = 1;
	int32 mSpawnCount = 1;
};