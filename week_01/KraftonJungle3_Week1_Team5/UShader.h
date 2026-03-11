#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <string>
#include <vector>

#include "FVector3.h"
#include "UPrimitive.h"

class UShader : public UPrimitive
{
public:
	static constexpr UINT ShaderStageVertex = 1u << 0;
	static constexpr UINT ShaderStagePixel = 1u << 1;

	struct FTransformConstants
	{
		FVector3 Offset;
		float Scale = 1.0f;
	};

	UShader();
	~UShader() override;

	bool Create(ID3D11Device* device, const std::wstring& shaderFilePath, const D3D11_INPUT_ELEMENT_DESC* inputElements, UINT inputElementCount, const char* vsEntry = "VSMain", const char* psEntry = "PSMain");

	void Release();

	void Bind(ID3D11DeviceContext* deviceContext);
	void UnBind(ID3D11DeviceContext* deviceContext);

	ID3D11VertexShader* GetVertexShader() const { return VertexShader; }
	ID3D11PixelShader* GetPixelShader() const { return PixelShader; }
	ID3D11InputLayout* GetInputLayout() const { return InputLayout; }

	bool UpdateConstantBuffer(ID3D11DeviceContext* deviceContext, UINT slot, const void* data, UINT dataSize);
	bool UpdateConstantBuffer(ID3D11DeviceContext* deviceContext, const std::string& bufferName, const void* data, UINT dataSize);

	template <typename T>
	bool UpdateConstantBuffer(ID3D11DeviceContext* deviceContext, UINT slot, const T& data)
	{
		return UpdateConstantBuffer(deviceContext, slot, &data, static_cast<UINT>(sizeof(T)));
	}

	template <typename T>
	bool UpdateConstantBuffer(ID3D11DeviceContext* deviceContext, const std::string& bufferName, const T& data)
	{
		return UpdateConstantBuffer(deviceContext, bufferName, &data, static_cast<UINT>(sizeof(T)));
	}

	bool UpdateTransformConstant(ID3D11DeviceContext* deviceContext, FVector3 offset, float scale);
	void UpdateConstant(ID3D11DeviceContext* deviceContext, FVector3 offset, float scale);

private:
	struct FConstantBufferBinding
	{
		std::string Name;
		UINT Slot = 0;
		UINT SizeInBytes = 0;
		UINT ShaderStages = 0;
		ID3D11Buffer* Buffer = nullptr;
	};

	bool CreateConstantBuffers(ID3D11Device* device, ID3DBlob* vsBlob, ID3DBlob* psBlob);
	bool ReflectConstantBuffers(ID3D11Device* device, ID3DBlob* shaderBlob, UINT shaderStage);
	bool CreateConstantBuffer(ID3D11Device* device, const std::string& bufferName, UINT slot, UINT sizeInBytes, UINT shaderStages);

	FConstantBufferBinding* FindConstantBuffer(UINT slot);
	const FConstantBufferBinding* FindConstantBuffer(UINT slot) const;
	FConstantBufferBinding* FindConstantBuffer(const std::string& bufferName);
	const FConstantBufferBinding* FindConstantBuffer(const std::string& bufferName) const;

	static UINT AlignConstantBufferSize(UINT sizeInBytes);

private:
	ID3D11VertexShader* VertexShader;
	ID3D11PixelShader* PixelShader;
	ID3D11InputLayout* InputLayout;
	std::vector<FConstantBufferBinding> ConstantBuffers;
};
