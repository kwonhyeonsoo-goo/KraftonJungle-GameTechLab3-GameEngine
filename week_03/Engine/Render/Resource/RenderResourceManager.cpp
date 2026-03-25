#pragma comment( lib, "dxguid.lib")
#include "RenderResourceManager.h"
#include <d3d11.h>
#include <d3dcompiler.h>
#include "DirectXTK/WICTextureLoader.h"
#include "Render/Device/D3DDevice.h"
#include <fstream>
#include <filesystem>
#include <iostream>

void FRenderResourceManager::Create(FD3DDevice* device)
{
    this->device = device;
}

void FRenderResourceManager::Release()
{
    OutputDebugString(L"[Shutdown] 텍스처 정리 시작\n");
    for (auto& [id, data] : TextureMap)
    {
        if (data.ShaderResourceView) { data.ShaderResourceView->Release(); data.ShaderResourceView = nullptr; }
        if (data.Texture) { data.Texture->Release(); data.Texture = nullptr; }
    }
    TextureMap.clear();
    OutputDebugString(L"[Shutdown] 텍스처 dhks\n");

    this->device = nullptr;
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

FString FRenderResourceManager::LoadResourceFilePath()
{
    FString FilePath = GetFileDialoguePath();
    if (FilePath.empty()) { return ""; }
    std::ifstream File(FilePath);
    if (!File.is_open()) {
        // Failed to open file at target destination
        std::cerr << "Failed to open file at target destination" << std::endl;
        return nullptr;
    }

    return FilePath;
}

FString FRenderResourceManager::GetFileDialoguePath()
{
    std::string FileDestination;
    char szFile[MAX_PATH] = {};

    OPENFILENAMEA ofn = {};
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = nullptr;
    ofn.lpstrFilter = "Image Files (*.png;*.jpg;*.jpeg;*.PNG;)\0*.png;*.jpg;*.jpeg;*.PNG\0All Files (*.*)\0*.*\0";
    ofn.lpstrFile = szFile;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrDefExt = "png";
    ofn.lpstrTitle = "Load Image";
    ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;

    if (!GetOpenFileNameA(&ofn)) {
        return "";
    }

    FileDestination = szFile;

    const size_t DotPos = FileDestination.rfind('.');
    if (DotPos == std::string::npos) {
        std::cerr << "Invalid file: no extension found" << std::endl;
        return "";
    }

    const std::string Extension = FileDestination.substr(DotPos);

    const std::vector<std::string> ValidExtensions = { ".png", ".jpg", ".jpeg", ".PNG"};
    const bool bValidExtension = std::any_of(ValidExtensions.begin(), ValidExtensions.end(),
        [&Extension](const std::string& Ext) { return Extension == Ext; });

    if (!bValidExtension) {
        std::cerr << "Invalid file type: got " << Extension << std::endl;
        return "";
    }
    
    return FileDestination;
}
