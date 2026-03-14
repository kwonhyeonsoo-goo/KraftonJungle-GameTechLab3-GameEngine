#include "SerializationService.h"
#include "UPrimitiveComponent.h"
#include "Scene.h"
#include "FVector.h"
#include "json.hpp"

#include <fstream>
#include <sstream>
#include <fileapi.h>

using json::JSON;

static FString GetScenePath(const AppContext& ctx) {
	return "saves/" + std::string(*ctx.Scene.Name) + ".scene";
}

bool SerializationService::Save(const AppContext& ctx) {
	// 루트 오브젝트 생성
	JSON root = json::Object();
	root["SceneName"] = std::string(*ctx.Scene.Name);

	root["Version"] = 1;
	root["NextUUID"] = (uint32)ctx.UUIDs.GetNextUUID();

	// Primitives 오브젝트
	JSON primitives = json::Object();
	for (UObject* obj : ctx.Objects.GetAll()) {
		UPrimitiveComponent* prim = dynamic_cast<UPrimitiveComponent*>(obj);//TODO: IsA로 RTTI구현
		if (!prim) continue;

		std::string key = std::to_string(prim->GetUUID());

		JSON entry = json::Object();
		entry["Type"] = prim->GetTypeName(); // std::string


		Transform trans = prim->GetTransform();

		// Location
		FVector loc = trans.GetLocation();
		JSON locArr = json::Array(loc.x, loc.y, loc.z);
		entry["Location"] = locArr;

		// Rotation
		FVector rot = trans.GetRotation();
		JSON rotArr = json::Array(rot.x, rot.y, rot.z);
		entry["Rotation"] = rotArr;

		// Scale
		FVector scl = trans.GetScale();
		JSON sclArr = json::Array(scl.x, scl.y, scl.z);
		entry["Scale"] = sclArr;

		primitives[key] = entry;
	}
	root["Primitives"] = primitives;

	CreateDirectoryA("saves", nullptr); // Windows API

	// 파일 쓰기
	std::ofstream file(GetScenePath(ctx));
	if (!file.is_open()) return false;
	file << root.dump();
	return file.good();
}

bool SerializationService::Load(AppContext& ctx) {
	// 파일 읽기
	std::ifstream file(GetScenePath(ctx));
	if (!file.is_open()) return false;

	std::stringstream ss;
	ss << file.rdbuf();
	FString jsonStr = ss.str();

	JSON root = JSON::Load(jsonStr);
	ctx.Editor.Selection.Clear();

	// ② 기존 UUID 복사 후 OnObjectDestroyed 발행
	TArray<uint32> oldUUIDs = ctx.UUIDs.GetAll();
	for (uint32 uuid : oldUUIDs)
		EventBus::Broadcast(OnObjectDestroyed{ uuid });

	// ③ ObjectStore 비우기
	ctx.Objects.Clear();
	ctx.UUIDs.Clear();
	
	auto toFloat = [](const JSON& j) -> float {
		if (j.JSONType() == JSON::Class::Floating)
			return (float)j.ToFloat();
		return (float)j.ToInt(); // 정수로 저장된 경우 처리
		};

	// ④ Primitives 순회 → 오브젝트 생성
	auto& primitives = root["Primitives"];
	ctx.Scene.Name = FString(root["SceneName"].ToString());

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
		rot.x = toFloat(kv.second["Location"][0]);
		rot.y = toFloat(kv.second["Location"][1]);
		rot.z = toFloat(kv.second["Location"][2]);

		// Scale
		FVector scl;
		scl.x = toFloat(kv.second["Location"][0]);
		scl.y = toFloat(kv.second["Location"][1]);
		scl.z = toFloat(kv.second["Location"][2]);

		UPrimitiveComponent* prim = ObjectFactory::Create(typeName, uuid);
		if (!prim) continue; // 알 수 없는 타입이면 스킵

		prim->SetTransform(Transform(loc, rot, scl));

		ctx.Objects.Add(prim);
		ctx.UUIDs.Register(uuid);
		EventBus::Broadcast(OnObjectCreated{ uuid }); // ← Outliner 등이 새 오브젝트 인식
	}

	// ⑤ NextUUID 복원
	ctx.UUIDs.SyncNextUUID((uint32)root["NextUUID"].ToInt());

	return true;
}