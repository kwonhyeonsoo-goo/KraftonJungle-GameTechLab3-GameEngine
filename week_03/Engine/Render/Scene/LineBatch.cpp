#include "LineBatch.h"

void FLineBatch::Create(ID3D11Device* InDevice)
{
	LineBatchData.CreateDynamic(InDevice, 1 << 16);
}

void FLineBatch::Release()
{
	LineBatchData.Release();

}

void FLineBatch::InsertLineDatas(TArray<FVertex>& Datas)
{
	Lines.insert(Lines.end(), Datas.begin(), Datas.end());
}

void FLineBatch::AddLine(const FVector& StartPoint, const FVector& EndPoint, const FVector4& Color)
{
	Lines.push_back({ StartPoint, Color });
	Lines.push_back({ EndPoint, Color });
}


void FLineBatch::DrawLineBatch(ID3D11DeviceContext* InDeviceContext)
{
	if (Lines.empty()) return;

	LineBatchData.GetFVertexBuffer().Update(InDeviceContext, Lines);

	uint32 Offset = 0;
	uint32 Stride = LineBatchData.GetFVertexBuffer().GetStride();
	ID3D11Buffer* VertexBuffer = LineBatchData.GetFVertexBuffer().GetBuffer();
	if (!VertexBuffer) return;

	InDeviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &Stride, &Offset);
	InDeviceContext->Draw(LineBatchData.GetFVertexBuffer().GetVertexCount(), 0);
}

void FLineBatch::Clear()
{
	Lines.clear();
}
