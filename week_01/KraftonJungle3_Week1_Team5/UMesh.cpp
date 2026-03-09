#include "UMesh.h"

#include "Utility.h"

UMesh::UMesh() : UPrimitive(), VertexBuffer(nullptr), IndexBuffer(nullptr), VertexStride(0), VertexCount(0), IndexCount(0),
                 PrimitiveTopology()
{
}

UMesh::~UMesh()
{
	Release();
}

bool UMesh::CreateVertexBuffer(ID3D11Device* device, const void* vertexData, UINT vertexStride, UINT vertexCount)
{
	if (device == nullptr || vertexData == nullptr || vertexStride == 0 || vertexCount == 0)
	{
		return false;
	}

	SafeRelease(VertexBuffer);

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = vertexStride * vertexCount;
	bufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = vertexData;

	HRESULT hr = device->CreateBuffer(&bufferDesc, &initData, &VertexBuffer);
	if (FAILED(hr))
	{
		VertexStride = 0;
		VertexCount = 0;
		return false;
	}

	VertexStride = vertexStride;
	VertexCount = vertexCount;

	return true;
}

bool UMesh::CreateIndexBuffer(ID3D11Device* device, const UINT* indexData, UINT indexCount)
{
	if (device == nullptr || indexData == nullptr || indexCount == 0)
	{
		return false;
	}

	SafeRelease(IndexBuffer);

	D3D11_BUFFER_DESC bufferDesc = {};
	bufferDesc.Usage = D3D11_USAGE_DEFAULT;
	bufferDesc.ByteWidth = sizeof(UINT) * indexCount;
	bufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	bufferDesc.CPUAccessFlags = 0;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA initData = {};
	initData.pSysMem = indexData;

	HRESULT hr = device->CreateBuffer(&bufferDesc, &initData, &IndexBuffer);
	if (FAILED(hr))
	{
		IndexCount = 0;
		return false;
	}

	IndexCount = indexCount;
	return true;
}

bool UMesh::Create(ID3D11Device* device, const void* vertexData, UINT vertexStride, UINT vertexCount,
	const UINT* indexData, UINT indexCount)
{
	if (!CreateVertexBuffer(device, vertexData, vertexStride, vertexCount))
	{
		return false;
	}

	if (indexData != nullptr && indexCount > 0)
	{
		if (!CreateIndexBuffer(device, indexData, indexCount))
		{
			return false;
		}
	}

	return true;
}

void UMesh::Bind(ID3D11DeviceContext* deviceContext) const
{
	if (deviceContext == nullptr)
	{
		return;
	}

	UINT offset = 0;

	if (VertexBuffer)
	{
		deviceContext->IASetVertexBuffers(0, 1, &VertexBuffer, &VertexStride, &offset);
	}

	if (IndexBuffer)
	{
		deviceContext->IASetIndexBuffer(IndexBuffer, DXGI_FORMAT_R32_UINT, 0);
	}

	deviceContext->IASetPrimitiveTopology(PrimitiveTopology);
}

void UMesh::Draw(ID3D11DeviceContext* deviceContext) const
{
	if (deviceContext == nullptr || VertexBuffer == nullptr || VertexCount == 0)
	{
		return;
	}

	Bind(deviceContext);
	deviceContext->Draw(VertexCount, 0);
}

void UMesh::DrawIndexed(ID3D11DeviceContext* deviceContext) const
{
	if (deviceContext == nullptr || VertexBuffer == nullptr || IndexBuffer == nullptr || IndexCount == 0)
	{
		return;
	}

	Bind(deviceContext);
	deviceContext->DrawIndexed(IndexCount, 0, 0);
}

void UMesh::SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology)
{
	PrimitiveTopology = topology;
}

void UMesh::Release()
{
	SafeRelease(IndexBuffer);
	SafeRelease(VertexBuffer);

	VertexStride = 0;
	VertexCount = 0;
	IndexCount = 0;
	PrimitiveTopology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}
