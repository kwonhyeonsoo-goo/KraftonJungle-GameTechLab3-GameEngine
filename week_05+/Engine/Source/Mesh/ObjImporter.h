#pragma once

#include "ObjInfo.h"
#include "Mesh.h"
class FStaticMeshRenderData;

class ENGINE_API FObjImporter
{
public:
	/**
	 * 엔진의 X/Y/Z 축에 OBJ의 어느 축을 대응시킬지 지정합니다.
	 */
	enum class EAxisDirection : uint8
	{
		PositiveX,
		NegativeX,
		PositiveY,
		NegativeY,
		PositiveZ,
		NegativeZ
	};

	struct FImportAxisMapping
	{
		EAxisDirection EngineX = EAxisDirection::PositiveX;
		EAxisDirection EngineY = EAxisDirection::PositiveY;
		EAxisDirection EngineZ = EAxisDirection::PositiveZ;
		float ImportScale = 1.0f;
	};

	// .obj 파싱 -> Raw Data
	static bool ParseObj(const FString& FilePath, FObjInfo& OutInfo);

	// .mtl 파싱 -> MaterialInfo 배열
	static bool ParseMtl(const FString& FilePath, TArray<FObjMaterialInfo>& OutMaterials);

	// Raw > Cooked (FStaticMesh)
	static FStaticMeshRenderData* Cook(const FObjInfo& Info);

	static FImportAxisMapping MakeDefaultImportAxisMapping();
	static void SetImportAxisMapping(const FImportAxisMapping& InMapping);
	static FImportAxisMapping GetImportAxisMapping();
	static bool IsValidImportAxisMapping(const FImportAxisMapping& InMapping);
	static FString BuildImportAxisMappingKey(const FImportAxisMapping& InMapping);

private:
	static FImportAxisMapping ImportAxisMapping;
};
