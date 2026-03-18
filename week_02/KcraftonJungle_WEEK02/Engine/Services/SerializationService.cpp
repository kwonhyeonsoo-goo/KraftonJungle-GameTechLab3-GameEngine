#include "SerializationService.h"
#include "../../AppContext.h"
#include "../World/UPrimitiveComponent.h"
#include "../World/World.h"
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
    JSON root = json::Object();
    root["SceneName"] = std::string(ctx.CurrentWorld.GetName());

    root["Version"] = 1;
	root["NextUUID"] = (uint32)ctx.UUIDs.GetNext();//TODO: 이거 UUID로 바꿔야함

    JSON camera = json::Object();

    FEditorCameraState Camera = ctx.Editor.GetActiveCamera();
    FVector Pos = Camera.Position;
    camera["Position"] = json::Array(Pos.x, Pos.y, Pos.z);

    camera["Yaw"] = Camera.Yaw;
    camera["Pitch"] = Camera.Pitch;
    camera["MoveSpeed"] = Camera.MoveSpeed;
    camera["RotSpeed"] = Camera.RotSpeed;

    root["Camera"] = camera;

    JSON primitives = json::Object();
    for (const auto& obj_uptr : ctx.CurrentWorld.GetAllObjects()) {
        UObject* obj = obj_uptr.get();
        if (!obj->IsA<UPrimitiveComponent>()) continue;
        UPrimitiveComponent* prim = static_cast<UPrimitiveComponent*>(obj);
        if (!prim) continue;

        std::string key = std::to_string(prim->GetUUID());

        JSON entry = json::Object();
        entry["Type"] = PrimitiveShapeToString(prim->Shape);

        Transform trans = prim->GetTransform();

        FVector loc = trans.Location;
        entry["Location"] = json::Array(loc.x, loc.y, loc.z);

        const FQuat rot = trans.Rotation;
        entry["RotationQuat"] = json::Array(rot.x, rot.y, rot.z, rot.w);

        FVector scl = trans.Scale;
        entry["Scale"] = json::Array(scl.x, scl.y, scl.z);

        primitives[key] = entry;
    }
    root["Primitives"] = primitives;

	CreateDirectoryA("saves", nullptr); // Windows API

    std::ofstream file(GetScenePath(ctx));
    if (!file.is_open()) return false;
    file << root.dump();
    return file.good();
}

bool SerializationService::Load(AppContext& ctx) {
    std::ifstream file(GetScenePath(ctx));
    if (!file.is_open()) return false;

    std::stringstream ss;
    ss << file.rdbuf();
    FString jsonStr = ss.str();

    JSON root = JSON::Load(jsonStr);
	//ctx.Editor.Selection.Clear();

    TArray<uint32> uuidsToDelete;
    for (const auto& obj_uptr : ctx.CurrentWorld.GetAllObjects())
        uuidsToDelete.push_back(obj_uptr->GetUUID());
    for (uint32 uuid : uuidsToDelete)
        ctx.Dispatch(std::make_unique<DeleteObjectCommand>(ctx, uuid));

    ctx.Objects.Clear();

    auto toFloat = [](const JSON& j) -> float {
        if (j.JSONType() == JSON::Class::Floating)
            return (float)j.ToFloat();
        return (float)j.ToInt();
        };

    ctx.UUIDs.SyncNextUUID((uint32)root["NextUUID"].ToInt());

    //auto& camera = root["Camera"];

    //FEditorCameraState& Camera = ctx.Editor.GetActiveCamera();
    //Camera.Position.x = toFloat(camera["Position"][0]);
    //Camera.Position.y = toFloat(camera["Position"][1]);
    //Camera.Position.z = toFloat(camera["Position"][2]);
    //Camera.Yaw = toFloat(camera["Yaw"]);
    //Camera.Pitch = toFloat(camera["Pitch"]);
    //Camera.MoveSpeed = toFloat(camera["MoveSpeed"]);
    //Camera.RotSpeed = toFloat(camera["RotSpeed"]);

    auto& primitives = root["Primitives"];
    ctx.CurrentWorld.SetName(root["SceneName"].ToString());

    for (auto& kv : primitives.ObjectRange()) {
		uint32 uuid = (uint32)std::stoul(kv.first);
        std::string typeName = kv.second["Type"].ToString();

        FVector loc;
        loc.x = toFloat(kv.second["Location"][0]);
        loc.y = toFloat(kv.second["Location"][1]);
        loc.z = toFloat(kv.second["Location"][2]);

        FQuat rot;
        rot.x = toFloat(kv.second["RotationQuat"][0]);
        rot.y = toFloat(kv.second["RotationQuat"][1]);
        rot.z = toFloat(kv.second["RotationQuat"][2]);
        rot.w = toFloat(kv.second["RotationQuat"][3]);

        FVector scl;
        scl.x = toFloat(kv.second["Scale"][0]);
        scl.y = toFloat(kv.second["Scale"][1]);
        scl.z = toFloat(kv.second["Scale"][2]);

        ctx.Dispatch(std::make_unique<SpawnObjectCommand>(
            ctx,
            StringToPrimitiveShape(typeName),
            Transform(loc, rot, scl)
        ));
    }

    return true;
}
