#pragma once

#include "Object.h"
#include "ActorComponent.h"

class UWorld;
struct FRenderInfo;
struct FTransform;
class USceneComponent;
class FRenderCollector;

class AActor : public UObject
{
	REFLECT_CLASS(AActor, UObject)
public:
	AActor() = default;
	virtual ~AActor();

	void Initialize();

	virtual void SerializeClass(json::JSON& outJson) const override;
	virtual void DeserializeClass(const json::JSON& inJson) override;

	void AddComponent(UActorComponent* actorComponent);
	void AddRootSceneComponent(USceneComponent* sceneComponent);
	USceneComponent* GetRootComponent() const;
	bool RemoveComponent(uint32 componentUUID);
	inline const TArray<UActorComponent*>& GetComponents() const { return mComponents; }

	virtual void CreateEditorComponents();

	FTransform GetTransform() const;

	virtual void Tick(float deltaTime);
	virtual void Render(FRenderCollector& RenderCollector);

	void GetRenderInfos(TArray<FRenderInfo>* outRenderInfos) const;
	bool GetFirstRenderInfo(FRenderInfo& outRenderInfo) const;

	void SetLocation(FVector location);
	void SetRotation(FRotator rotation);
	void SetScale(FVector scale);

private:
	int32 getComponentIndex(int32 componentUUID) const;

private:
	
	USceneComponent* mRootComponent = nullptr;
	TArray<UActorComponent*> mComponents;
	bool mbPressed = false;
	bool mbStarted = false;
};

inline const FVector Up = FVector(0, 0, 1);
inline const FVector Right = FVector(0, 1, 0);
inline const FVector Front = FVector(1, 0, 0);

