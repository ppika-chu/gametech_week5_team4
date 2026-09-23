#pragma once

#include <functional>

#include "enum.h"
#include "Vector.h"
#include "Rotator.h"
#include "TMap.h"

namespace json { class JSON; }

class UObject;
class AActor;
class FClassInfo;

struct FObjectFactory
{
	static UObject* ConstructUnInitializedObject(const FClassInfo* classInfo);
	static UObject* LoadObject(const FClassInfo* classInfo, const json::JSON& inJson);

	template<typename TObject, typename... Args>
		requires std::derived_from<TObject, UObject>
	static TObject* ConstructObject(Args&& ...args);

	static void DestroyObject(UObject* object);

	template<typename TObject>
		requires std::derived_from<TObject, UObject>
	static TObject* ConstructUnInitializedObject();

	template<typename TObject>
		requires std::derived_from<TObject, UObject>
	static TObject* LoadObject(const json::JSON& inJson);

	static AActor* SpawnPrimitiveActor(EPrimitive primitiveType,
		FVector3 Location, FRotator Rotation, FVector3 Scale
	);

	static const FClassInfo* GetClassInfoByName(const FString& className);

	static bool RegisterClassInfo(FString className, const FClassInfo* classInfo);

private:
	// TODO: Automate the registration of class info for all UObject-derived classes.
	static TMap<FString, std::function<const FClassInfo* ()>> mClassInfoMap;
};


#include "ObjectFactory.inl"
