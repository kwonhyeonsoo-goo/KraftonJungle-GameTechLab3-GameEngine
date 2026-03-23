#include "PrimitiveSky.h"
#include "Core/Paths.h"
#include <cmath>
const FString CPrimitiveSky::Key = "SkySphere";
// FString CPrimitiveSky::GetFilePath() { return FPaths::MeshDir() + "SkySphere.mesh"; }


CPrimitiveSky::CPrimitiveSky(int32 Segments, int32 Rings)
{
	auto Cached = GetCached(Key);
	if (Cached)
	{
		MeshData = Cached;
	}
	else
		Generate(Segments, Rings);
}

void CPrimitiveSky::Generate(int32 Segments, int32 Rings)
{
	auto Data = std::make_shared<FMeshData>();

	const float Radius = 0.5f;


}
