#pragma once

#include "CoreMinimal.h"
#include <d3d11.h>
#include <memory>
#include <cstring>

class FVertexShader;
class FPixelShader;

// Material이 소유하는 상수 버퍼 슬롯 하나
// GPU 버퍼 생성, CPU 데이터 관리, Dirty 플래그 기반 업로드를 모두 담당
struct ENGINE_API FMaterialConstantBuffer
{
	ID3D11Buffer* GPUBuffer = nullptr;
	uint8* CPUData = nullptr; // CPU 쪽 shadow copy
	uint32 Size = 0;
	bool bDirty = false;

	FMaterialConstantBuffer() = default;
	~FMaterialConstantBuffer();

	// 복사 금지
	FMaterialConstantBuffer(const FMaterialConstantBuffer&) = delete;
	FMaterialConstantBuffer& operator=(const FMaterialConstantBuffer&) = delete;

	// Move 지원 (소유권 이전)
	FMaterialConstantBuffer(FMaterialConstantBuffer&& Other) noexcept
		: GPUBuffer(Other.GPUBuffer), CPUData(Other.CPUData), Size(Other.Size), bDirty(Other.bDirty)
	{
		Other.GPUBuffer = nullptr;
		Other.CPUData = nullptr;
		Other.Size = 0;
		Other.bDirty = false;
	}
	FMaterialConstantBuffer& operator=(FMaterialConstantBuffer&& Other) noexcept
	{
		if (this != &Other)
		{
			Release();
			GPUBuffer = Other.GPUBuffer;
			CPUData = Other.CPUData;
			Size = Other.Size;
			bDirty = Other.bDirty;
			Other.GPUBuffer = nullptr;
			Other.CPUData = nullptr;
			Other.Size = 0;
			Other.bDirty = false;
		}
		return *this;
	}

	// Device로 GPU 버퍼 생성 + CPU 메모리 할당
	bool Create(ID3D11Device* Device, uint32 InSize);

	// CPU 데이터의 특정 오프셋에 값 쓰기 (Dirty 마킹)
	void SetData(const void* Data, uint32 InSize, uint32 Offset = 0);

	// Dirty면 Map/Unmap으로 GPU에 업로드
	void Upload(ID3D11DeviceContext* DeviceContext);

	void Release();
};

// Material: VS/PS 셰이더 조합 + 추가 상수 버퍼 (b2+)
class ENGINE_API FMaterial
{
public:
	FMaterial() = default;
	~FMaterial();

	FMaterial(const FMaterial&) = delete;
	FMaterial& operator=(const FMaterial&) = delete;
	FMaterial(FMaterial&&) = default;
	FMaterial& operator=(FMaterial&&) = default;

	void SetName(const FString& InName) { Name = InName; }
	const FString& GetName() const { return Name; }

	void SetVertexShader(const std::shared_ptr<FVertexShader>& InVS) { VertexShader = InVS; }
	void SetPixelShader(const std::shared_ptr<FPixelShader>& InPS) { PixelShader = InPS; }

	FVertexShader* GetVertexShader() const { return VertexShader.get(); }
	FPixelShader* GetPixelShader() const { return PixelShader.get(); }

	// 상수 버퍼 슬롯 추가 (b2, b3, ... 순서대로)
	// InSize: 상수 버퍼 크기 (16바이트 정렬 필수)
	// 반환: 슬롯 인덱스 (0 = b2, 1 = b3, ...)
	int32 CreateConstantBuffer(ID3D11Device* Device, uint32 InSize);

	// 슬롯 인덱스로 상수 버퍼 접근
	FMaterialConstantBuffer* GetConstantBuffer(int32 Index);

	// 셰이더 바인딩 + Dirty 상수 버퍼 업로드 + 바인딩
	void Bind(ID3D11DeviceContext* DeviceContext);

	void Release();

private:
	FString Name;
	std::shared_ptr<FVertexShader> VertexShader;
	std::shared_ptr<FPixelShader> PixelShader;

	TArray<FMaterialConstantBuffer> ConstantBuffers;

	static constexpr UINT MaterialCBStartSlot = 2; // b0=Frame, b1=Object, b2+=Material
};
