#include "Archive.h"
#include "Renderer/Material.h"
#include "ThirdParty/nlohmann/json.hpp"

using json = nlohmann::json;

FArchive::FArchive(bool bInSaving)
	: bSaving(bInSaving)
	, JsonData(new json())
{
}

FArchive::~FArchive()
{
	delete static_cast<json*>(JsonData);
}

void FArchive::Serialize(const FString& Key, FString& Value)
{
	json& Json = *static_cast<json*>(JsonData);
	if (bSaving) //save mode
		Json[Key] = Value;
	else if (Json.contains(Key)) //load mode
		Value = Json[Key].get<FString>();
}

void FArchive::Serialize(const FString& Key, uint32& Value)
{
	json& Json = *static_cast<json*>(JsonData);
	if (bSaving)
		Json[Key] = Value;
	else if (Json.contains(Key))
		Value = Json[Key].get<uint32>();
}
void FArchive::Serialize(const FString& Key, bool& Value)
{
	json& Json = *static_cast<json*>(JsonData);
	if (bSaving)
		Json[Key] = Value;
	else if (Json.contains(Key))
		Value = Json[Key].get<bool>();
}
void FArchive::Serialize(const FString& Key, FVector& Value)
{
	json& Json = *static_cast<json*>(JsonData);
	if (bSaving)
		Json[Key] = { Value.X, Value.Y, Value.Z };
	else if (Json.contains(Key))
	{
		auto& xyz = Json[Key];
		Value = { xyz[0].get<float>(), xyz[1].get<float>(), xyz[2].get<float>() };
	}
}
void FArchive::Serialize(const FString& Key, FVector4& Value)
{
	json& Json = *static_cast<json*>(JsonData);
	if (bSaving)
		Json[Key] = { Value.X, Value.Y, Value.Z, Value.W };
	else if (Json.contains(Key))
	{
		auto& xyzw = Json[Key];
		Value = { xyzw[0].get<float>(), xyzw[1].get<float>(), xyzw[2].get<float>(),xyzw[3].get<float>() };
	}
}

void FArchive::SerializeMaterialTexture(const FString& Key,
	TArray<FString>& MaterialNames,
	TArray<FString>& TextureNames)
{
	json& Json = *static_cast<json*>(JsonData);
	if (bSaving)
	{
		json Arr = json::array();
		for (int i = 0;i < MaterialNames.size();++i)
		{
			json MatObj;
			MatObj["Name"] = MaterialNames[i];
			MatObj["MapKd"] = TextureNames[i];

			Arr.push_back(MatObj);
		}
		Json[Key] = Arr;
	}
	else
	{
		if (Json.contains(Key) && Json[Key].is_array())
		{
			auto& JsonArr = Json[Key];
			for (size_t i = 0;i < JsonArr.size();++i)
			{
				FString Name = JsonArr[i]["Name"].get<FString>();
				FString MapKd = JsonArr[i]["MapKd"].get<FString>();

				MaterialNames.push_back(Name);
				TextureNames.push_back(MapKd);
			}
		}
	}
}

void FArchive::SerializeUIntArray(const FString& Key, TArray<uint32>& Values)
{
	json& Json = *static_cast<json*>(JsonData);
	if (bSaving)
	{
		json Arr = json::array();
		for (uint32 value : Values)
			Arr.push_back(value);
		Json[Key] = Arr;
	}
	else if (Json.contains(Key))
	{
		Values.clear();
		for (auto& Element : Json[Key])
			Values.push_back(Element.get<uint32>());
	}
}

bool FArchive::Contains(const FString& Key) const
{
	const json& Json = *static_cast<const json*>(JsonData);
	return Json.contains(Key);
}

void* FArchive::GetRawJson()
{
	return JsonData;
}
