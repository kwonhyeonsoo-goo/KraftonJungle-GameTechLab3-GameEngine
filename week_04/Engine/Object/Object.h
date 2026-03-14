#pragma once
#include "CoreMinimal.h"
#include "ObjectTypes.h"

class UClass;

class ENGINE_API UObject
{
public:
	UObject(UClass* InClass, FString InName, UObject* InOuter = nullptr);
	virtual ~UObject() = default;

    UClass* GetClass() const;
    const std::string& GetName() const;
    UObject* GetOuter() const;

    bool IsA(const UClass* InClass) const;

    template <typename T>
    T* GetTypedOuter() const
    {
        static_assert(std::is_base_of_v<UObject, T>, "T must derive from UObject");

        UObject* Current = Outer;
        while (Current)
        {
            if (Current->IsA(T::StaticClass()))
            {
                return static_cast<T*>(Current);
            }
            Current = Current->GetOuter();
        }
        return nullptr;
    }

    FString GetPathName() const;

    bool HasAnyFlags(EObjectFlags InFlags) const;
    bool HasAllFlags(EObjectFlags InFlags) const;
    void AddFlags(EObjectFlags InFlags);
    void ClearFlags(EObjectFlags InFlags);

    void MarkPendingKill();
    bool IsPendingKill() const;

    static UClass* StaticClass();

private:
	UClass* Class = nullptr;
	FString Name;
	UObject* Outer = nullptr;					// 소속 컨테이너
	EObjectFlags Flags = EObjectFlags::None;	// 상태
};

