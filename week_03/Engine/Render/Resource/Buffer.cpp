#include "Buffer.h"

#pragma region __FMESHBUFFER__

void FMeshBuffer::Create(ID3D11Device* InDevice, const FMeshData& InMeshData)
{
	Release();

	if (InMeshData.Vertices.empty()) return;

	VertexBuffer.Create(InDevice, InMeshData.Vertices, sizeof(FVertex));
	if (!InMeshData.Indices.empty())
	{
		IndexBuffer.Create(InDevice, InMeshData.Indices);
	}
}

void FMeshBuffer::CreateDynamic(ID3D11Device* InDevice, uint32 InMaxVertices)
{
	Release();
	VertexBuffer.CreateDynamic(InDevice, InMaxVertices, sizeof(FVertex));
}

void FMeshBuffer::Release()
{
	VertexBuffer.Release();
	IndexBuffer.Release();
}

#pragma endregion

#pragma region __FVERTEXBUFFER__

void FVertexBuffer::Create(ID3D11Device* InDevice, const TArray<FVertex>& InData, uint32 InStride)
{
	Release();

	if (InData.empty()) return;

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth  = static_cast<uint32>(InData.size()) * InStride;
	desc.Usage      = D3D11_USAGE_IMMUTABLE;
	desc.BindFlags  = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA srd = { InData.data() };
	HRESULT hr = InDevice->CreateBuffer(&desc, &srd, &Buffer);
	if (FAILED(hr)) { Release(); return; }

	VertexCount = static_cast<uint32>(InData.size());
	Stride = InStride;
}

void FVertexBuffer::CreateDynamic(ID3D11Device* InDevice, uint32 InMaxVertices, uint32 InStride)
{
	Release();

	D3D11_BUFFER_DESC desc = {};
	desc.ByteWidth      = InMaxVertices * InStride;
	desc.Usage          = D3D11_USAGE_DYNAMIC;
	desc.BindFlags      = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT hr = InDevice->CreateBuffer(&desc, nullptr, &Buffer);
	if (FAILED(hr)) { Release(); return; }

	VertexCount = 0;
	Stride = InStride;
}

void FVertexBuffer::Release()
{
	if (Buffer)
	{
		Buffer->Release();
		Buffer = nullptr;
	}
}

void FVertexBuffer::Update(ID3D11DeviceContext* InDeviceContext, const TArray<FVertex>& InData)
{
	if (!Buffer || InData.empty()) return;

	D3D11_MAPPED_SUBRESOURCE MSR = {};
	InDeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MSR);
	memcpy(MSR.pData, InData.data(), sizeof(FVertex) * InData.size());
	InDeviceContext->Unmap(Buffer, 0);

	VertexCount = static_cast<uint32>(InData.size());
}

void FVertexBuffer::FontUpdate(ID3D11DeviceContext* InDeviceContext, const TArray<FFontVertex>& InData)
{
	if (!Buffer || InData.empty()) return;

	D3D11_MAPPED_SUBRESOURCE MSR = {};
	InDeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &MSR);
	memcpy(MSR.pData, InData.data(), sizeof(FFontVertex) * InData.size());
	InDeviceContext->Unmap(Buffer, 0);

	VertexCount = static_cast<uint32>(InData.size());
}

ID3D11Buffer* FVertexBuffer::GetBuffer() const
{
	return Buffer;
}

#pragma endregion

#pragma region __FCONSTANTBUFFER__

//	 Constant buffer는 Dynamic으로 생성하여 업데이트가 가능하도록 구현
void FConstantBuffer::Create(ID3D11Device* InDevice, uint32 InByteWidth)
{
	D3D11_BUFFER_DESC constantBufferDesc = {};

	constantBufferDesc.ByteWidth = InByteWidth + 0xf &0xfffffff0;
	constantBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	constantBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;	
	constantBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	InDevice->CreateBuffer(&constantBufferDesc, nullptr, &Buffer);
}

void FConstantBuffer::Release()
{
	if(Buffer)
	{
		Buffer->Release();
		Buffer = nullptr;
	}
}

//	Constant buffer는 Dynamic으로 생성했으므로 업데이트가 가능. 업데이트가 필요하다면 Map/Unmap을 이용하여 업데이트
//	InData는 Constant buffer에 업데이트할 데이터의 포인터입니다. InByteWidth는 업데이트할 데이터의 총 byte 크기입니다.
//	즉, InData는 FPerObjectConstants 구조체의 포인터입니다.
void FConstantBuffer::Update(ID3D11DeviceContext* InDeviceContext, const void * InData, uint32 InByteWidth)
{
	if (Buffer)
	{
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;
		InDeviceContext->Map(Buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);

		//std::memcpy(constantbufferMSR.pData, InData, InByteWidth);
		FConstantBuffer* constants = (FConstantBuffer*)constantbufferMSR.pData;
		{
			std::memcpy(constants, InData, InByteWidth);
		}

		InDeviceContext->Unmap(Buffer, 0);
	}
}

ID3D11Buffer* FConstantBuffer::GetBuffer() 
{
	return Buffer;
}

#pragma endregion

#pragma region __FINDEXBUFFER__

void FIndexBuffer::Create(ID3D11Device* InDevice, const TArray<uint32>& InData)
{
	Release();

	if (InData.empty()) return;

	D3D11_BUFFER_DESC desc = {};
	desc.Usage     = D3D11_USAGE_IMMUTABLE;
	desc.ByteWidth = static_cast<uint32>(InData.size()) * sizeof(uint32);
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA srd = { InData.data() };
	HRESULT hr = InDevice->CreateBuffer(&desc, &srd, &Buffer);
	if (FAILED(hr)) { Release(); return; }

	IndexCount = static_cast<uint32>(InData.size());
}


void FIndexBuffer::Release()
{
	if (Buffer)
	{
		Buffer->Release();
		Buffer = nullptr;
	}
}

void FIndexBuffer::Update(ID3D11DeviceContext* InDeviceContext, const TArray<uint32>& InData, uint32 InByteWidth)
{
	//	 Do nothing
}

ID3D11Buffer * FIndexBuffer::GetBuffer() const
{
	return Buffer;
}

#pragma endregion
