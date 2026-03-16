#pragma once
#include "PrimitiveBase.h"

class ENGINE_API CPrimitiveGizmo : public CPrimitiveBase
{
public:
	enum class EGizmoType : std::uint8_t
	{
		Translation,
		Rotation,
		Scale
	};

	static const FString Key;
	static const FString FilePath;

	CPrimitiveGizmo();

	void Generate(EGizmoType Type);

	void GenerateTranslationGizmoMesh();
	void GenerateRotationGizmoMesh();
	void GenerateScaleGizmoMesh();

};

