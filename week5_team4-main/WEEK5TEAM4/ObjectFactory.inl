template<typename TObject>
	requires std::derived_from<TObject, UObject>
TObject* FObjectFactory::ConstructUnInitializedObject()
{
	return static_cast<TObject*>(ConstructUnInitializedObject(TObject::GetStaticClass()));
}

template<typename TObject, typename... Args>
	requires std::derived_from<TObject, UObject>
TObject* FObjectFactory::ConstructObject(Args&& ...args)
{
	static_assert(requires(TObject * obj)
	{
		obj->Initialize(std::forward<Args>(args)...);

	}, "TObject must have an Initialize method that accepts the provided arguments.");

	TObject* instance = ConstructUnInitializedObject<TObject>();
	if (instance)
	{
		instance->Initialize(std::forward<Args>(args)...);
	}
	return instance;
}

template<typename TObject>
	requires std::derived_from<TObject, UObject>
TObject* FObjectFactory::LoadObject(const json::JSON& inJson)
{
	TObject* instance = ConstructUnInitializedObject<TObject>();
	if (instance)
	{
		instance->DeserializeClass(inJson);
	}

	return instance;
}
