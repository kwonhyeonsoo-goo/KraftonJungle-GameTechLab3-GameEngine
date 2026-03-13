#include "ShaderManager.h"
#include <d3dcompiler.h>

CShaderManager::~CShaderManager()
{
    Release();
}

bool CShaderManager::LoadVertexShader(ID3D11Device* Device, const wchar_t* FilePath)
{
    // TODO: D3DCompileFromFile → CreateVertexShader → CreateInputLayout
    return true;
}

bool CShaderManager::LoadPixelShader(ID3D11Device* Device, const wchar_t* FilePath)
{
    // TODO: D3DCompileFromFile → CreatePixelShader
    return true;
}

void CShaderManager::Bind(ID3D11DeviceContext* DeviceContext)
{
    // TODO: VSSetShader, PSSetShader, IASetInputLayout
}

void CShaderManager::Release()
{
    // TODO: COM 리소스 해제
}
