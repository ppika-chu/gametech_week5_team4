#pragma once

#include "Json/json.hpp"
#include "Vector.h"
#include "Rotator.h"
#include "FGuid.h"
#include "enum.h"
#include "TArray.h"

namespace JsonUtils
{
	json::JSON ToJson(const FString& String);
	json::JSON ToJson(const FVector2& Vector);
	json::JSON ToJson(const FVector& Vector);
	json::JSON ToJson(const FRotator& Rotator);
	json::JSON ToJson(const EPrimitive& Primitive);
	json::JSON ToJson(const FGuid& Guid);

	template <typename T>
	inline json::JSON ToJson(const TArray<T>& Array)
	{
		json::JSON jsonArray = json::JSON::Make(json::JSON::Class::Array);
		for (const T& element : Array)
		{
			if constexpr (std::is_arithmetic_v<T>)
			{
				jsonArray.append(element);
			}
			else
			{
				jsonArray.append(ToJson(element));
			}
		}
		return jsonArray;
	}

	template <typename T>
	inline T FromJson(const json::JSON& json)
	{
		if constexpr (std::is_arithmetic_v<T>)
		{
			if (json.JSONType() == json::JSON::Class::Integral)
			{
				return static_cast<T>(json.ToInt());
			}
			else if (json.JSONType() == json::JSON::Class::Floating)
			{
				return static_cast<T>(json.ToFloat());
			}
			else
			{
				throw std::runtime_error("Json value is not a number");
			}
		}
		else
		{
			static_assert(false, "FromJson not implemented for this type");
		}
	}

	template <>
	inline FString FromJson<FString>(const json::JSON& json)
	{
		if (json.JSONType() == json::JSON::Class::String)
		{
			return FString(json.ToString());
		}
		else
		{
			throw std::runtime_error("Json value is not a string");
		}
	}

	template <>
	inline FVector2 FromJson(const json::JSON& json)
	{
		if (json.JSONType() != json::JSON::Class::Array)
		{
			throw std::runtime_error("Json Array expected for FVector2");
		}

		return FVector2(json.at(0).ToFloat(), json.at(1).ToFloat());
	}

	template <>
	inline FVector FromJson(const json::JSON& json)
	{
		if (json.JSONType() != json::JSON::Class::Array)
		{
			throw std::runtime_error("Json Array expected for FVector");
		}

		return FVector(json.at(0).ToFloat(), json.at(1).ToFloat(), json.at(2).ToFloat());
	}

	template <>
	inline FRotator FromJson(const json::JSON& json)
	{
		if (json.JSONType() != json::JSON::Class::Array)
		{
			throw std::runtime_error("Json Array expected for FRotator");
		}

		return FRotator(json.at(0).ToFloat(), json.at(1).ToFloat(), json.at(2).ToFloat());
	}

	template <>
	inline EPrimitive FromJson(const json::JSON& json)
	{
		if (json.JSONType() != json::JSON::Class::String)
		{
			throw std::runtime_error("Json String expected for EPrimitive");
		}

		std::string primitiveStr = json.ToString();
		if (primitiveStr == "Sphere")
		{
			return EPrimitive::EP_Sphere;
		}
		else if (primitiveStr == "Cube")
		{
			return EPrimitive::EP_Cube;
		}
		else if (primitiveStr == "Triangle")
		{
			return EPrimitive::EP_Triangle;
		}
		else if (primitiveStr == "GizmoArrow")
		{
			return EPrimitive::EP_GizmoArrow;
		}
		else if (primitiveStr == "Circle")
		{
			return EPrimitive::EP_Circle;
		}
		else if (primitiveStr == "Plane")
		{
			return EPrimitive::EP_Plane;
		}
		else
		{
			throw std::runtime_error("Unknown EPrimitive value in JSON");
		}
	}

	template <>
	inline FGuid FromJson(const json::JSON& json)
	{
		if (json.JSONType() != json::JSON::Class::Object)
		{
			throw std::runtime_error("Json Object expected for FGuid");
		}

		return FGuid(json.at("A").ToInt(), json.at("B").ToInt(), json.at("C").ToInt(), json.at("D").ToInt());
	}

	template <typename T>
	inline void FromJson(const json::JSON& json, TArray<T>& array)
	{
		if (json.JSONType() != json::JSON::Class::Array)
		{
			throw std::runtime_error("Json Array expected for TArray");
		}

		array.Empty();
		for (const auto& element : json.ArrayRange())
		{
			array.Add(FromJson<T>(element));
		}
	}
}