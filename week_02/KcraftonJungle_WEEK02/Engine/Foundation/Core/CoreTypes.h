#pragma once
#include <string>
#include <cstdint>
#include <functional>

using int32 = std::int32_t;
using uint32 = std::uint32_t;
using int64 = std::int64_t;
using uint64 = std::uint64_t;

using FString = std::string;

#include "../Containers/TArray.h"
#include "../Containers/TMap.h"

using DelegateHandle = uint64;

template<typename T>
class TDelegate {
public:
    DelegateHandle Bind(std::function<void(const T&)>& fn);
    void Unbind(DelegateHandle handle);
    void Clear();
    void Broadcast(const T& value) const;
    bool IsBound() const;

private:
    TMap<DelegateHandle, std::function<void(const T&)>> Listeners;
    DelegateHandle NextHandle = 1;
};

template<typename T>
inline DelegateHandle TDelegate<T>::Bind(std::function<void(const T&)>& fn)
{
    if (!fn) return 0;
    const DelegateHandle Handle = NextHandle++;
    Listeners[Handle] = std::move(fn);
    return Handle;
}

template<typename T>
inline void TDelegate<T>::Unbind(DelegateHandle handle)
{
    if (handle == 0) return;

    auto it = Listeners.find(handle);
    if (it != Listeners.end()) Listeners.erase(it);
}

template<typename T>
inline void TDelegate<T>::Clear()
{
    Listeners.clear();
}

template<typename T>
inline void TDelegate<T>::Broadcast(const T& value) const
{
    TArray<std::function<void(const T&)>> callbacks;
    callbacks.reserve(Listeners.size());
    for (const auto& pair : Listeners)
    {
        if (pair.second)
        {
            callbacks.push_back(pair.second);
        }
    }

    for (const auto& fn : callbacks)
    {
        fn(value);
    }
}

template<typename T>
inline bool TDelegate<T>::IsBound() const
{
    return !Listeners.empty();
}

// DirectX나 COM 객체뿐만 아니라 일반적인 포인터 해제에도 쓰일 수 있게 정의
namespace Engine {

    // DirectX 리소스 해제용 (COM Interface)
    template <typename T>
    inline void SafeRelease(T*& InterfacePtr) {
        if (InterfacePtr != nullptr) {
            InterfacePtr->Release();
            InterfacePtr = nullptr;
        }
    }

    // 일반 메모리 해제용 (참고용)
    template <typename T>
    inline void SafeDelete(T*& Ptr) {
        if (Ptr != nullptr) {
            delete Ptr;
            Ptr = nullptr;
        }
    }
}