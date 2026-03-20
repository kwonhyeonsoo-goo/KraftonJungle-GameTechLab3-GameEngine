#pragma once
#include "Engine/Render/Resource/Buffer.h"

class FLineBatch
{
public:
	FLineBatch() = default;
	~FLineBatch() = default;

private:
	TArray<FVertex> Lines;
	FMeshBuffer		LineBatchData;

public:
	void Create(ID3D11Device* InDevice);
	void Release();

	void InsertLineDatas(TArray<FVertex>& Datas);
	void AddLine(const FVector& StartPoint, const FVector& EndPoint, const FVector4& Color);
	void DrawLineBatch(ID3D11DeviceContext* InDeviceContext);

	bool HasLines() const { return !Lines.empty(); }
	void Clear();
};