#pragma once

#include "CoreMinimal.h"
#include <d3d11.h>
struct FConstants;
class ENGINE_API CShaderManager
{
public:
    CShaderManager() = default;
    ~CShaderManager();

    bool LoadVertexShader(ID3D11Device* Device, const wchar_t* FilePath);
    bool LoadPixelShader(ID3D11Device* Device, const wchar_t* FilePath);
    void Bind(ID3D11DeviceContext* DeviceContext);
    void UpdateConstants(ID3D11DeviceContext* Context, const FConstants& Data);
    void Release();

private:
    ID3D11VertexShader* VertexShader = nullptr;
    ID3D11PixelShader* PixelShader = nullptr;
    ID3D11InputLayout* InputLayout = nullptr;
    ID3D11Buffer* ConstantBuffer = nullptr;
};
