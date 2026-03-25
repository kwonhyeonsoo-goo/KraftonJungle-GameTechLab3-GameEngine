#pragma once

#include "EngineStatics.h"
#include "Core/Name.h"
#include <memory>

#define DECLARE_CLASS(ClassName, ParentClass)                          \
    static const FTypeInfo s_TypeInfo;                                 \
    const FTypeInfo* GetTypeInfo() const override {                    \
        return &s_TypeInfo;                                            \
    }                                                                  \
    static ClassName* Cast(UObject* Obj) {                             \
        return Obj ? Obj->Cast<ClassName>() : nullptr;                 \
    }

#define DEFINE_CLASS(ClassName, ParentClass)                           \
    const FTypeInfo ClassName::s_TypeInfo = {                          \
        #ClassName,                                                    \
        &ParentClass::s_TypeInfo,                                      \
        sizeof(ClassName)                                              \
    };

enum EClassFlags : uint32_t {
	CF_None		 = 0,
	CF_Actor	 = 1 << 0,
	CF_Component = 1 << 1,
	CF_Camera	 = 1 << 2,
};

struct FTypeInfo {
	const char* name;
	const FTypeInfo* Parent;
	size_t size;
	uint32_t ClassFlags = CF_None;

	bool IsA(const FTypeInfo* Other) const {
		for (const FTypeInfo* T = this; T; T = T->Parent) {
			if (T == Other) {
				return true;
			}
		}
		return false;
	}
};

namespace json { class JSON; }

using std::make_unique;
using std::make_shared;
using std::unique_ptr;
using std::shared_ptr;
using std::weak_ptr;
class UObject : public std::enable_shared_from_this<UObject>
{
public:
	uint32 UUID;
	uint32 InternalIndex;
	uint32 AllocationSize;
	FName Name;
	bool bPendingKill;

	UObject();
	virtual ~UObject();

	static void* operator new(size_t Size)
	{
		void* Ptr = std::malloc(Size);
		if (Ptr)
		{
			EngineStatics::OnAllocated(static_cast<uint32>(Size));
		}
		return Ptr;
	}

	static void operator delete(void* Ptr, size_t Size)
	{
		if (Ptr)
		{
			EngineStatics::OnDeallocated(static_cast<uint32>(Size));
			std::free(Ptr);
;		}
	}

	void SetNewName(FString NewName) { Name = FName(NewName); };
	void SetNewName(const char* NewName) { Name = FName(NewName); };
	void SerializeHeader(json::JSON& j) const;
	void DeserializeHeader(const json::JSON& j);
	virtual void Serialize(json::JSON& j) const {}
	virtual void Deserialize(const json::JSON& j) {}

	// RTTI stuffs
	virtual const FTypeInfo* GetTypeInfo() const { return &s_TypeInfo; }

	template<typename T>
	bool IsA() const { return GetTypeInfo()->IsA(&T::s_TypeInfo); }

	template<typename T>
	T* Cast() { return IsA<T>() ? static_cast<T*>(this) : nullptr; }

	template<typename T>
	const T* Cast() const { return IsA<T>() ? static_cast<const T*>(this) : nullptr; }

	static const FTypeInfo s_TypeInfo;
};

extern TArray<shared_ptr<UObject>> GUObjectArray;


class UObjectManager {
public:
	// Singleton
	static UObjectManager& Get()
	{
		static UObjectManager instance;
		return instance;
	}

	template<typename T>
	weak_ptr<T> CreateObject() {
		auto Obj = make_shared<T>();
		Obj->Name = FName(FString(Obj->GetTypeInfo()->name) + "_" + std::to_string(Obj->UUID));
		Obj->InternalIndex = static_cast<uint32>(GUObjectArray.size());
		Obj->AllocationSize = static_cast<uint32>(sizeof(T));
		EngineStatics::OnAllocated(static_cast<uint32>(sizeof(T)));
		GUObjectArray.push_back(Obj);
		return Obj;
	}

	// Does not destroy the target object right away. Only marks for death
	void DestroyObject(weak_ptr<UObject> Obj) {
		if (auto ptr = Obj.lock()) {
			ptr->bPendingKill = true;
			if (NumPendingToKill++ > GC_Threshold) {
				//CollectGarbage();
			}
		}
	}

	void DestroyObject(UObject* Obj) {
		if (Obj) {
			Obj->bPendingKill = true;
			if (NumPendingToKill++ > GC_Threshold) {
				//CollectGarbage();
			}
		}
	}

	void CollectGarbage() {
		GUObjectArray.erase(
			std::remove_if(GUObjectArray.begin(), GUObjectArray.end(),
				[](const shared_ptr<UObject>& Obj) {
					return !Obj || Obj->bPendingKill;
				}),
			GUObjectArray.end()
		);
		NumPendingToKill = 0;
	}

	void ForceFlush() {
		GUObjectArray.clear();  // shutdown only
		NumPendingToKill = 0;
	}

	//UObject* FindByUUID(uint32 UUID)
	//{
	//	for (auto* Obj : GUObjectArray)
	//		if (Obj && Obj->UUID == UUID)
	//			return Obj;
	//	return nullptr;
	//}

	//UObject* FindByIndex(uint32 Index)
	//{
	//	if (Index >= GUObjectArray.size()) return nullptr;
	//	return GUObjectArray[Index];   // may be null if destroyed
	//}


private:
	UObjectManager() = default;
	~UObjectManager() { CollectGarbage(); }

	uint32 NumPendingToKill = 0;
	const uint32 GC_Threshold = 512;
};
