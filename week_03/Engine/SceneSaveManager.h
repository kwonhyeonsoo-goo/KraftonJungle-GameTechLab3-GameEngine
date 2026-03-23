#pragma once

#include <iostream>
#include <fstream>
#include <filesystem>
#include <chrono>
#include "Engine/Scene/Camera.h"
#include "World/PrimitiveComponent.h"
#include "Object/Object.h"
#include "World.h"
#include "Object/ObjectFactory.h"

// Forward decl.
namespace json {
	class JSON;
}

enum EDialogueMode {
	FILE_SAVE,
	FILE_LOAD
};

using std::string;

// The Rule of Thumb: 1 scene, 1 save file
class FSceneSaveManager {
public:
	// Creates a .json save file at the given destination. 
	static void SaveSceneAsJSON(UScene* Scene);
	static UScene* LoadSceneFromJSON(UWorld* World);

	static FString GetFileDialoguePath(EDialogueMode Mode);
private:
	//-----------------------------------------------------------------------------------
	// Serialization
	// ----------------------------------------------------------------------------------
	// Creates a .json save file at the given destination
	static json::JSON SerializeObject(UObject* Object);

	static json::JSON SerializeVector(float X, float Y, float Z);

	//-----------------------------------------------------------------------------------
	// Desrialization
	// ----------------------------------------------------------------------------------
	// Resolves parent-child and owning references between components, actors, and world
	static UScene* LinkReferences(const TMap<uint32, UObject*>& uuidMap, json::JSON Savedata, UScene* Scene);
	
	// Generate spatial vectors for USceneCompoents from a json save
	static void DeserializeSpaceVectors(USceneComponent* SceneComp, json::JSON& Savedata);

	static void DecodeCamera(UCamera* Camera, json::JSON& Savedata);

	static void DecodePrimitiveComponents(UPrimitiveComponent* Prim, json::JSON& Savedata);

	static string GetCurrentTimeStamp();
};
