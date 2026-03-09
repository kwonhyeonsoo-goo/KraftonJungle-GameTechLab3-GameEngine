#pragma once
#include <d3d11.h>

#include "UPrimitive.h"
class UMesh : public UPrimitive
{
public:
	UMesh();
	~UMesh() override;

    bool CreateVertexBuffer(ID3D11Device* device, const void* vertexData, UINT vertexStride, UINT vertexCount);
    bool CreateIndexBuffer(ID3D11Device* device, const UINT* indexData, UINT indexCount);

    bool Create(ID3D11Device* device, const void* vertexData, UINT vertexStride, UINT vertexCount, const UINT* indexData, UINT indexCount);

    void Bind(ID3D11DeviceContext* deviceContext) const;
    void Draw(ID3D11DeviceContext* deviceContext) const;
    void DrawIndexed(ID3D11DeviceContext* deviceContext) const;

    void SetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY topology);
    void Release();

    bool HasVertexBuffer() const { return VertexBuffer != nullptr; }
    bool HasIndexBuffer() const { return IndexBuffer != nullptr; }

    UINT GetVertexStride() const { return VertexStride; }
    UINT GetVertexCount() const { return VertexCount; }
    UINT GetIndexCount() const { return IndexCount; }

    ID3D11Buffer* GetVertexBuffer() const { return VertexBuffer; }
    ID3D11Buffer* GetIndexBuffer() const { return IndexBuffer; }

private:
    ID3D11Buffer* VertexBuffer;
    ID3D11Buffer* IndexBuffer;

    UINT VertexStride;
    UINT VertexCount;
    UINT IndexCount;

    D3D11_PRIMITIVE_TOPOLOGY PrimitiveTopology;
};

