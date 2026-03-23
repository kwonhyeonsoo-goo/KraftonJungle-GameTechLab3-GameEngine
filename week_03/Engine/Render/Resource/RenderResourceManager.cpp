#pragma comment( lib, "dxguid.lib")
#include "RenderResourceManager.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include "DirectXTK/WICTextureLoader.h"
#include "Render/Device/D3DDevice.h"

void FRenderResourceManager::Create(FD3DDevice* device)
{
    this->device = device;
}

int FRenderResourceManager::CreateTexture(std::wstring filepath)
{
    TextureData data = {};

    ID3D11Resource* RawTexture = nullptr;

    HRESULT Hr = CreateWICTextureFromFile(
        device->GetDevice(),
        device->GetDeviceContext(),
        filepath.c_str(),
        &RawTexture,           // nullptr 말고 받아야 Texture2D 꺼낼 수 있음
        &data.ShaderResourceView
    );
    if (FAILED(Hr))
    {
        // 로드 실패 처리
        return -1;
    }

    // ID3D11Resource → ID3D11Texture2D 로 캐스팅
    RawTexture->QueryInterface(__uuidof(ID3D11Texture2D), (void**)&data.Texture);
    RawTexture->Release(); // QueryInterface가 레퍼런스 카운트 올리므로 원본 해제

    int NewId = static_cast<int>(TextureMap.size());
    TextureMap.emplace(NewId, data);

    return NewId;
}
