#pragma once
#include <d3d11.h>
#include <string>

#include "FVector3.h"
#include "UPrimitive.h"

class UShader : public UPrimitive
{
public:
	struct FConstants
	{
		FVector3 Offset;
		float Scale;
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

	void UpdateConstant(ID3D11DeviceContext* deviceContext, FVector3 offset, float scale);

private:
	bool CreateConstantBuffers(ID3D11Device* device);

private:
	ID3D11VertexShader* VertexShader;
	ID3D11PixelShader* PixelShader;
	ID3D11InputLayout* InputLayout;
	ID3D11Buffer* ConstantBuffer;
};

