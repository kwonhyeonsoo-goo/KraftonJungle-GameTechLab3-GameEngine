#include "SerializationService.h"
#include "../../AppContext.h"
#include "../World/UPrimitiveComponent.h"
#include "../World/World.h"
#include "../Foundation/Math/FVector.h"
#include "../Foundation/Math/FVector.h"
#include "../ObjectKernel/ObjectFactory.h"
#include "../Editor/Commands/DeleteObjectCommand.h"
#include "../../json.hpp"

#include <fstream>
#include <sstream>
using json::JSON;

static FString GetScenePath(const AppContext& ctx) {
	return "saves/" + std::string(ctx.CurrentWorld.GetName()) + ".scene";
}

bool SerializationService::Save(const AppContext& ctx) {
	// ��Ʈ ������Ʈ ����
	JSON root = json::Object();
	root["SceneName"] = std::string(ctx.CurrentWorld.GetName());

	root["Version"] = 1;
	root["NextUUID"] = (uint32)ctx.UUIDs.GetNext();//TODO: 이거 UUID로 바꿔야함

	// Primitives ������Ʈ
	JSON primitives = json::Object();
	for (UObject* obj : ctx.CurrentWorld.GetAllObjects()) {
		UPrimitiveComponent* prim = dynamic_cast<UPrimitiveComponent*>(obj);//TODO: IsA�� RTTI����
		if (!prim) continue;

		std::string key = std::to_string(prim->GetUUID());

		JSON entry = json::Object();
		entry["Type"] = PrimitiveShapeToString(prim->Shape);


		Transform trans = prim->GetTransform();

		// Location
		FVector loc = trans.Location;
		JSON locArr = json::Array(loc.x, loc.y, loc.z);
		entry["Location"] = locArr;

		// Rotation
		FVector rot = trans.Rotation;
		JSON rotArr = json::Array(rot.x, rot.y, rot.z);
		entry["Rotation"] = rotArr;

		// Scale
		FVector scl = trans.Scale;
		JSON sclArr = json::Array(scl.x, scl.y, scl.z);
		entry["Scale"] = sclArr;

		primitives[key] = entry;
	}
	root["Primitives"] = primitives;

	CreateDirectoryA("saves", nullptr); // Windows API

	// ���� ����
	std::ofstream file(GetScenePath(ctx));
	if (!file.is_open()) return false;
	file << root.dump();
	return file.good();
}

bool SerializationService::Load(AppContext& ctx) {
	// ���� �б�
	std::ifstream file(GetScenePath(ctx));
	if (!file.is_open()) return false;

	std::stringstream ss;
	ss << file.rdbuf();
	FString jsonStr = ss.str();

	JSON root = JSON::Load(jsonStr);
	//ctx.Editor.Selection.Clear();

	// �� ���� UUID ���� �� OnObjectDestroyed ����
	TArray<UObject*> Objects = ctx.CurrentWorld.GetAllObjects();
	for (auto* Object : Objects)
		ctx.Dispatch(new DeleteObjectCommand(
			ctx,
			Object->GetUUID()
		));

	// �� ObjectStore ����
	ctx.Objects.Clear();
	
	auto toFloat = [](const JSON& j) -> float {
		if (j.JSONType() == JSON::Class::Floating)
			return (float)j.ToFloat();
		return (float)j.ToInt();
		};

	// �� Primitives ��ȸ �� ������Ʈ ����
	auto& primitives = root["Primitives"];
	ctx.CurrentWorld.SetName(root["SceneName"].ToString());

	for (auto& kv : primitives.ObjectRange()) {
		uint32 uuid = (uint32)std::stoul(kv.first);
		std::string typeName = kv.second["Type"].ToString();

		// Location
		FVector loc;
		loc.x = toFloat(kv.second["Location"][0]);
		loc.y = toFloat(kv.second["Location"][1]);
		loc.z = toFloat(kv.second["Location"][2]);

		// Rotation
		FVector rot;
		rot.x = toFloat(kv.second["Rotation"][0]);
		rot.y = toFloat(kv.second["Rotation"][1]);
		rot.z = toFloat(kv.second["Rotation"][2]);

		// Scale
		FVector scl;
		scl.x = toFloat(kv.second["Scale"][0]);
		scl.y = toFloat(kv.second["Scale"][1]);
		scl.z = toFloat(kv.second["Scale"][2]);


		std::cout << typeName << Transform(loc, rot, scl) << std::endl;

		ctx.Dispatch(
			new SpawnObjectCommand(
				ctx,
				StringToPrimitiveShape(typeName),
				Transform(loc, rot, scl)
			)
		);
	}

	ctx.UUIDs.SyncNextUUID((uint32)root["NextUUID"].ToInt());

	return true;
}