#pragma once

#include "Math/Vector.h"

/*
	Vertex 구조체들을 정의하는 Header입니다.
	추후에 다양한 Vertex 구조체들을 추가할 수 있습니다.
*/

struct FVertex
{ 
	FVector Position;
	FVector4 Color;
	int SubID;

    // TMap을 위한 연산자 오버로딩
    bool operator==(const FVertex& Other) const
    {
        return Position.X == Other.Position.X
            && Position.Y == Other.Position.Y
            && Position.Z == Other.Position.Z
            && Color.X == Other.Color.X
            && Color.Y == Other.Color.Y
            && Color.Z == Other.Color.Z
            && Color.W == Other.Color.W
            && SubID == Other.SubID;
    }
};

struct FUVVertex
{
    FVector Position;
    float u, v;

    // TMap을 위한 연산자 오버로딩
    bool operator==(const FUVVertex& Other) const
    {
        return Position.X == Other.Position.X
            && Position.Y == Other.Position.Y
            && Position.Z == Other.Position.Z
            && u == Other.u
            && v == Other.v;
    }
};


namespace std
{
    template<>
    struct hash<FVertex>
    {
        size_t operator()(const FVertex& V) const
        {
            size_t h = 0;
            h ^= std::hash<float>{}(V.Position.X) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(V.Position.Y) + 0x9e3779b9 + (h << 6) + (h >> 2);
            h ^= std::hash<float>{}(V.Position.Z) + 0x9e3779b9 + (h << 6) + (h >> 2);
            return h;
        }
    };
}

struct FOverlayVertex
{
	float X, Y;
};

struct FMeshData
{
	TArray<FVertex> Vertices;
	TArray<uint32> Indices;
};

struct FUVMeshData
{
    TArray<FUVVertex> Vertices;
    TArray<uint32> Indices;
};

struct FFontVertex
{
	FVector Position;
	float U, V;     // UV
};
