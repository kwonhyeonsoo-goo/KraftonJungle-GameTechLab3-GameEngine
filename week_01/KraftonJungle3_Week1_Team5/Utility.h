#include <d3dcompiler.h>
#include <string>

template <typename T>
static void SafeRelease(T*& ptr)
{
    if (ptr)
    {
        ptr->Release();
        ptr = nullptr;
    }
}

template <typename T>
static void SafeDelete(T*& ptr)
{
    if (ptr)
    {
        delete ptr;
        ptr = nullptr;
    }
}

template <typename T>
static void SafeReleaseAndDelete(T*& ptr)
{
    if (ptr)
    {
        ptr->Release();
        delete ptr;
        ptr = nullptr;
    }
}

static bool CompileShaderFromFile(const std::wstring& filePath, const char* entryPoint, const char* shaderModel, ID3DBlob** shaderBlob)
{
    if (shaderBlob == nullptr)
    {
        return false;
    }

    *shaderBlob = nullptr;

    UINT compileFlags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
    compileFlags |= D3DCOMPILE_DEBUG;
    compileFlags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
    compileFlags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompileFromFile(
        filePath.c_str(),
        nullptr,
        D3D_COMPILE_STANDARD_FILE_INCLUDE,
        entryPoint,
        shaderModel,
        compileFlags,
        0,
        shaderBlob,
        &errorBlob
    );

    if (FAILED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((const char*)errorBlob->GetBufferPointer());
        }

        SafeRelease(errorBlob);
        return false;
    }

    SafeRelease(errorBlob);
    return true;
}