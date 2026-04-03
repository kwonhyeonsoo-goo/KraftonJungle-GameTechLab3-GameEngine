#pragma once

#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

template <typename T>
using TComPtr = Microsoft::WRL::ComPtr<T>;
