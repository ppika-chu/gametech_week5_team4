
#define REFLECT_CLASS(className, superClassName)									 \
public:																				 \
	using Super = superClassName;													 \
	static const FClassInfo* GetStaticClass()										 \
	{																				 \
		static FClassInfo classInstance = FClassInfo(								 \
			#className,																 \
			superClassName::GetStaticClass(),										 \
			[]() -> UObject* {														 \
				if constexpr (std::is_abstract_v<className>)						 \
				{																	 \
					return nullptr;													 \
				}																	 \
				else {																 \
					return new className();											 \
				}																	 \
			}																		 \
		);																			 \
		return &classInstance;														 \
	}																				 \
	virtual const FClassInfo* GetClass() const override { return GetStaticClass(); } \

template<typename TObject>
	requires std::derived_from<TObject, UObject>
bool UObject::IsA() const
{
	return IsA(TObject::GetStaticClass());
}

template<typename TObject>
	requires std::derived_from<TObject, UObject>
TObject* UObject::Cast()
{
	if (IsA<TObject>())
	{
		return static_cast<TObject*>(this);
	}
	return nullptr;
}

template<typename TObject>
	requires std::derived_from<TObject, UObject>
TObject* UObject::GetObjectByUUID(int32 uuid)
{
	UObject* object = GetObjectByUUID(uuid);
	if (object && object->IsA<TObject>())
	{
		return static_cast<TObject*>(object);
	}
	return nullptr;
}

template<typename TObject>
	requires std::derived_from<TObject, UObject>
TObject* UObject::GetObjectByInternalIndex(uint32 internalIndex)
{
	UObject* object = GetObjectByInternalIndex(internalIndex);
	if (object && object->IsA<TObject>())
	{
		return static_cast<TObject*>(object);
	}
	return nullptr;
}
