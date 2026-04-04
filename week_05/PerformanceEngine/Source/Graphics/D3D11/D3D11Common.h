#pragma once

#include <d3d11_1.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

template <typename T>
using TComPtr = Microsoft::WRL::ComPtr<T>;
