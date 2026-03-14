#pragma once
#include "UObject.h"

#pragma region Types

#include <cstdint>
#include <string>
#include <vector>

template <typename T>
using TArray = std::vector<T>;

typedef std::string FString;

typedef int int32;
typedef unsigned int uint32;
typedef int64_t int64;
typedef uint64_t uint64;

#pragma endregion

struct AppContext;
class UPrimitiveComponent;

class UScene :
    public UObject
{
public:
    UScene() : Ctx(nullptr), Name("default") {};

    UScene(AppContext* Ctx, FString Name)
        : Ctx(Ctx), Name(Name){
    };
    virtual ~UScene();

    //에디터 단계이기 때문에 해당 함수는 필요없음
    //virtual void Update();

    void Add(UPrimitiveComponent* comp);
    void Remove(uint32 uuid);
    UPrimitiveComponent* Find(uint32 uuid) const;
    const TArray<UObject*>& GetAllObjects()   const;

    void SetContext(AppContext* ctx) { Ctx = ctx; }
private:
    FString Name;
    AppContext* Ctx;
};

