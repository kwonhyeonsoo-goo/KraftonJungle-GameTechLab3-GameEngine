#include "Material.h"
#include "Shader.h"

// ─── FMaterialConstantBuffer ───

FMaterialConstantBuffer::~FMaterialConstantBuffer()
{
	Release();
}

bool FMaterialConstantBuffer::Create(ID3D11Device* Device, uint32 InSize)
{
	Release();

	Size = InSize;
	CPUData = new uint8[Size];
	memset(CPUData, 0, Size);

	D3D11_BUFFER_DESC Desc = {};
	Desc.ByteWidth = Size;
	Desc.Usage = D3D11_USAGE_DYNAMIC;
	Desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	Desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	HRESULT Hr = Device->CreateBuffer(&Desc, nullptr, &GPUBuffer);
	if (FAILED(Hr))
	{
		Release();
		return false;
	}

	bDirty = true; // 초기 데이터(0)도 업로드 필요
	return true;
}

void FMaterialConstantBuffer::SetData(const void* Data, uint32 InSize, uint32 Offset)
{
	if (!CPUData || Offset + InSize > Size)
	{
		return;
	}
	memcpy(CPUData + Offset, Data, InSize);
	bDirty = true;
}

void FMaterialConstantBuffer::Upload(ID3D11DeviceContext* DeviceContext)
{
	if (!bDirty || !GPUBuffer)
	{
		return;
	}

	D3D11_MAPPED_SUBRESOURCE Mapped;
	HRESULT Hr = DeviceContext->Map(GPUBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &Mapped);
	if (SUCCEEDED(Hr))
	{
		memcpy(Mapped.pData, CPUData, Size);
		DeviceContext->Unmap(GPUBuffer, 0);
	}
	bDirty = false;
}

void FMaterialConstantBuffer::Release()
{
	if (GPUBuffer)
	{
		GPUBuffer->Release();
		GPUBuffer = nullptr;
	}
	delete[] CPUData;
	CPUData = nullptr;
	Size = 0;
	bDirty = false;
}

// ─── FMaterial ───

FMaterial::~FMaterial()
{
	Release();
}

int32 FMaterial::CreateConstantBuffer(ID3D11Device* Device, uint32 InSize)
{
	FMaterialConstantBuffer CB;
	if (!CB.Create(Device, InSize))
	{
		return -1;
	}
	ConstantBuffers.push_back(std::move(CB));
	return static_cast<int32>(ConstantBuffers.size() - 1);
}

FMaterialConstantBuffer* FMaterial::GetConstantBuffer(int32 Index)
{
	if (Index < 0 || Index >= static_cast<int32>(ConstantBuffers.size()))
	{
		return nullptr;
	}
	return &ConstantBuffers[Index];
}

void FMaterial::Bind(ID3D11DeviceContext* DeviceContext)
{
	if (VertexShader)
	{
		VertexShader->Bind(DeviceContext);
	}
	if (PixelShader)
	{
		PixelShader->Bind(DeviceContext);
	}

	// Dirty 상수 버퍼 업로드 + 바인딩
	for (int32 i = 0; i < static_cast<int32>(ConstantBuffers.size()); ++i)
	{
		ConstantBuffers[i].Upload(DeviceContext);

		UINT Slot = MaterialCBStartSlot + static_cast<UINT>(i);
		ID3D11Buffer* Buf = ConstantBuffers[i].GPUBuffer;
		DeviceContext->VSSetConstantBuffers(Slot, 1, &Buf);
		DeviceContext->PSSetConstantBuffers(Slot, 1, &Buf);
	}
}

void FMaterial::Release()
{
	VertexShader.reset();
	PixelShader.reset();
	for (auto& CB : ConstantBuffers)
	{
		CB.Release();
	}
	ConstantBuffers.clear();
}
