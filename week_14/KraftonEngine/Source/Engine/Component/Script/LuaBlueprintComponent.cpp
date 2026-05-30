#include "LuaBlueprintComponent.h"

#include "Component/PrimitiveComponent.h"
#include "Core/Logging/Log.h"
#include "GameFramework/AActor.h"
#include "Lua/LuaScriptManager.h"
#include "LuaBlueprint/LuaBlueprintAsset.h"
#include "LuaBlueprint/LuaBlueprintManager.h"
#include "Object/GarbageCollection.h"

#include <cstring>

ULuaBlueprintComponent::ULuaBlueprintComponent()  = default;
ULuaBlueprintComponent::~ULuaBlueprintComponent() = default;

void ULuaBlueprintComponent::SetBlueprintPath(const FString& InPath)
{
    if (BlueprintPath == InPath)
    {
        return;
    }

    BlueprintPath                  = InPath;
    BlueprintAsset                 = nullptr;
    LoadedBlueprintVersion         = 0;
    LoadedBlueprintRuntimeVersion  = 0;

    if (Env.valid() || LuaBeginPlay || LuaTick || LuaEndPlay)
    {
        ReloadBlueprint();
    }
}

bool ULuaBlueprintComponent::ReloadBlueprint()
{
    ClearCollisionBindings();
    const bool bInitialized = InitializeLua();
    if (bInitialized)
    {
        BindOwnerCollisionEvents();
    }
    return bInitialized;
}

bool ULuaBlueprintComponent::CallFunction(const FString& FunctionName)
{
    if (!Env.valid())
    {
        return false;
    }

    sol::object Target = Env[FunctionName.c_str()];
    if (!Target.valid() || Target.get_type() != sol::type::function)
    {
        return false;
    }

    sol::protected_function Fn = Target;
    bool                    bOk;
    {
        FLuaCallScope                  Scope(this);
        sol::protected_function_result Result = Fn();
        bOk                                   = Result.valid();
        if (!bOk)
        {
            sol::error Err = Result;
            UE_LOG("LuaBlueprint %s error in %s: %s", FunctionName.c_str(), GetRuntimeName().c_str(), Err.what());
        }
    }
    return bOk;
}

void ULuaBlueprintComponent::BeginPlay()
{
    bEndPlayRouted = false;
    UActorComponent::BeginPlay();
    if (!ReloadBlueprint())
    {
        return;
    }

    if (bWantsBeginPlay && LuaBeginPlay)
    {
        FLuaCallScope                  Scope(this);
        sol::protected_function_result Result = LuaBeginPlay();
        if (!Result.valid())
        {
            sol::error Err = Result;
            UE_LOG("LuaBlueprint BeginPlay error in %s: %s", GetRuntimeName().c_str(), Err.what());
        }
    }
}

void ULuaBlueprintComponent::EndPlay()
{
    if (bEndPlayRouted)
    {
        return;
    }
    bEndPlayRouted = true;

    UActorComponent::EndPlay();
    ClearCollisionBindings();
    if (LuaCallDepth > 0)
    {
        bPendingLuaEndPlay = true;
        bPendingLuaCleanup = true;
        return;
    }
    InvokeLuaEndPlay();
    ClearLuaRuntime();
}

void ULuaBlueprintComponent::RouteComponentDestroyed()
{
    ClearCollisionBindings();
    UActorComponent::RouteComponentDestroyed();
}

void ULuaBlueprintComponent::BeginDestroy()
{
    // UActorComponent::BeginDestroy() routes virtual EndPlay() first. Do not clear
    // Lua runtime before that, or Lua EndPlay can be skipped on direct component
    // destruction / GC sweep paths.
    UActorComponent::BeginDestroy();
}

void ULuaBlueprintComponent::AddReferencedObjects(FReferenceCollector& Collector)
{
    UActorComponent::AddReferencedObjects(Collector);
    Collector.AddReferencedObject(BlueprintAsset, "ULuaBlueprintComponent::BlueprintAsset");
    for (FLuaBlueprintRuntimeObjectVariable& Variable : RuntimeObjectVariables)
    {
        if (Variable.bStrong)
        {
            Collector.AddReferencedObject(Variable.StrongValue, "ULuaBlueprintComponent::RuntimeObjectVariable");
        }
    }
}

void ULuaBlueprintComponent::PreGetEditableProperties()
{
    UActorComponent::PreGetEditableProperties();
    LoadBlueprintAsset();
}

void ULuaBlueprintComponent::PostEditProperty(const char* PropertyName)
{
    UActorComponent::PostEditProperty(PropertyName);
    if (PropertyName && std::strcmp(PropertyName, "BlueprintPath") == 0)
    {
        BlueprintAsset                = nullptr;
        LoadedBlueprintVersion        = 0;
        LoadedBlueprintRuntimeVersion = 0;
        ReloadBlueprint();
    }
}

void ULuaBlueprintComponent::TickComponent(
    float                        DeltaTime,
    ELevelTick                   TickType,
    FActorComponentTickFunction& ThisTickFunction
    )
{
    UActorComponent::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (ULuaBlueprintAsset* ValidAsset = GetValidBlueprintAsset())
    {
        if (LoadedBlueprintRuntimeVersion != ValidAsset->GetRuntimeVersion())
        {
            ReloadBlueprint();
        }
    }

    if (bWantsTick && LuaTick)
    {
        FLuaCallScope                  Scope(this);
        sol::protected_function_result Result = LuaTick(DeltaTime);
        if (!Result.valid())
        {
            sol::error Err = Result;
            UE_LOG("LuaBlueprint Tick error in %s: %s", GetRuntimeName().c_str(), Err.what());
        }
    }
}

ULuaBlueprintAsset* ULuaBlueprintComponent::GetValidBlueprintAsset() const
{
    // BlueprintAsset 는 raw 포인터다. 보통 FLuaBlueprintManager 가 루트로 잡아 살아있지만,
    // ClearCache() 이후 GC sweep 이 에셋을 회수하면 dangling 이 될 수 있다. 접근 직전마다
    // 검증하여 stale 포인터를 정리한 뒤 valid 포인터(또는 nullptr)만 노출한다.
    const_cast<ULuaBlueprintComponent*>(this)->ClearInvalidBlueprintAsset();
    return BlueprintAsset;
}

void ULuaBlueprintComponent::ClearInvalidBlueprintAsset()
{
    // IsValid 는 GetAliveObjectFromAddress 기반이라 dangling/freed 포인터가 들어와도
    // deref 없이 안전하게 살아있는 UObject 인지 검사한다.
    if (BlueprintAsset && !IsValid(BlueprintAsset))
    {
        BlueprintAsset                = nullptr;
        LoadedBlueprintVersion        = 0;
        LoadedBlueprintRuntimeVersion = 0;
    }
}

bool ULuaBlueprintComponent::LoadBlueprintAsset()
{
    ClearInvalidBlueprintAsset();

    if (BlueprintAsset && LoadedBlueprintRuntimeVersion == BlueprintAsset->GetRuntimeVersion())
    {
        return BlueprintAsset->HasRunnableLuaSource();
    }

    if (!BlueprintPath.empty() && BlueprintPath != "None")
    {
        BlueprintAsset = FLuaBlueprintManager::Get().Load(BlueprintPath);
    }
    else
    {
        BlueprintAsset                = nullptr;
        LoadedBlueprintVersion        = 0;
        LoadedBlueprintRuntimeVersion = 0;
    }

    if (!BlueprintAsset)
    {
        if (!BlueprintPath.empty() && BlueprintPath != "None")
        {
            UE_LOG("LuaBlueprint asset could not be loaded: %s", BlueprintPath.c_str());
        }
        return false;
    }

    if (BlueprintAsset->IsCompileDirty())
    {
        BlueprintAsset->Compile();
    }

    if (BlueprintAsset->HasCompileErrors())
    {
        UE_LOG(
            "LuaBlueprint compile errors exist: %s. Runtime will use last good source if available.",
            BlueprintPath.c_str()
        );
    }

    if (!BlueprintAsset->HasRunnableLuaSource())
    {
        UE_LOG("LuaBlueprint has no runnable Lua source: %s", BlueprintPath.c_str());
        return false;
    }

    LoadedBlueprintVersion        = BlueprintAsset->GetVersion();
    LoadedBlueprintRuntimeVersion = BlueprintAsset->GetRuntimeVersion();
    return true;
}

bool ULuaBlueprintComponent::InitializeLua()
{
    ClearLuaRuntime();
    const uint32 ThisRuntimeGeneration = ++LuaRuntimeGeneration;

    if (!LoadBlueprintAsset())
    {
        return false;
    }

    const FString& Source = BlueprintAsset->GetRuntimeLuaSource();
    if (Source.empty())
    {
        UE_LOG("LuaBlueprint has empty generated source: %s", BlueprintPath.c_str());
        return false;
    }

    sol::state& Lua  = FLuaScriptManager::GetState();
    Env              = sol::environment(Lua, sol::create, Lua.globals());
    Env["obj"]       = GetOwner();
    Env["this"]      = this;
    Env["component"] = this;
    Env.set_function(
        "BP_InitVar",
        [this](const FString& Name, bool bStrong)
        {
            InitRuntimeObjectVariable(Name, bStrong);
        }
    );
    Env.set_function(
        "BP_SetVar",
        [this](const FString& Name, sol::object Value)
        {
            SetRuntimeObjectVariable(Name, Value);
        }
    );
    Env.set_function(
        "BP_GetVar",
        [this](const FString& Name) -> UObject*
        {
            return GetRuntimeObjectVariable(Name);
        }
    );
    Env.set_function(
        "BP_Delay",
        [this, ThisRuntimeGeneration](float Seconds, sol::protected_function Callback)
        {
            ScheduleLuaDelay(Seconds, Callback, ThisRuntimeGeneration);
        }
    );

    InitializeRuntimeObjectVariables();

    const FString                  ChunkName = BlueprintPath.empty() ? GetRuntimeName() : BlueprintPath;
    sol::protected_function_result Result    = Lua.safe_script(Source, Env, sol::script_pass_on_error, ChunkName);
    if (!Result.valid())
    {
        sol::error Err = Result;
        UE_LOG("Failed to load LuaBlueprint %s: %s", ChunkName.c_str(), Err.what());
        ClearLuaRuntime();
        return false;
    }

    bWantsBeginPlay      = ReadEventFlag("BeginPlay");
    bWantsTick           = ReadEventFlag("Tick");
    bWantsEndPlay        = ReadEventFlag("EndPlay");
    bWantsOverlap        = ReadEventFlag("OnOverlap");
    bWantsEndOverlap     = ReadEventFlag("OnEndOverlap");
    bWantsHit            = ReadEventFlag("OnHit");
    bWantsEndHit         = ReadEventFlag("OnEndHit");
    bHasCalledLuaEndPlay = false;
    bPendingLuaEndPlay   = false;

    LuaBeginPlay    = sol::nil;
    LuaTick         = sol::nil;
    LuaEndPlay      = sol::nil;
    LuaOnOverlap    = sol::nil;
    LuaOnEndOverlap = sol::nil;
    LuaOnHit        = sol::nil;
    LuaOnEndHit     = sol::nil;
    if (bWantsBeginPlay) LuaBeginPlay = Env["BeginPlay"];
    if (bWantsTick) LuaTick = Env["Tick"];
    if (bWantsEndPlay) LuaEndPlay = Env["EndPlay"];
    if (bWantsOverlap) LuaOnOverlap = Env["OnOverlap"];
    if (bWantsEndOverlap) LuaOnEndOverlap = Env["OnEndOverlap"];
    if (bWantsHit) LuaOnHit = Env["OnHit"];
    if (bWantsEndHit) LuaOnEndHit = Env["OnEndHit"];
    LoadedBlueprintVersion        = BlueprintAsset->GetVersion();
    LoadedBlueprintRuntimeVersion = BlueprintAsset->GetRuntimeVersion();
    return true;
}

void ULuaBlueprintComponent::ReleaseLuaRuntimeForShutdown()
{
    // lua_State 가 아직 valid 한 동안(FLuaScriptManager::Shutdown 의 Lua.reset() 이전) 호출되어,
    // 보유 중인 sol 핸들을 지금 deref 해 둔다. 이후 최종 GC sweep 이 이 컴포넌트를 destroy 하며
    // ClearLuaRuntime 을 다시 타더라도, 핸들이 비어 있어 닫힌 lua_State 를 건드리지 않는다.
    ClearLuaRuntime();
}

void ULuaBlueprintComponent::ClearLuaRuntime()
{
    ++LuaRuntimeGeneration;
    LuaBeginPlay    = sol::nil;
    LuaTick         = sol::nil;
    LuaEndPlay      = sol::nil;
    LuaOnOverlap    = sol::nil;
    LuaOnEndOverlap = sol::nil;
    LuaOnHit        = sol::nil;
    LuaOnEndHit     = sol::nil;
    Env             = sol::environment();
    RuntimeObjectVariables.clear();
    bWantsBeginPlay      = false;
    bWantsTick           = false;
    bWantsEndPlay        = false;
    bWantsOverlap        = false;
    bWantsEndOverlap     = false;
    bWantsHit            = false;
    bWantsEndHit         = false;
    bHasCalledLuaEndPlay = false;
    bPendingLuaEndPlay   = false;
    bPendingLuaCleanup   = false;
}

void ULuaBlueprintComponent::BindOwnerCollisionEvents()
{
    ClearCollisionBindings();

    if ((!bWantsOverlap || !LuaOnOverlap) && (!bWantsEndOverlap || !LuaOnEndOverlap) && (!bWantsHit || !LuaOnHit) && (!
        bWantsEndHit || !LuaOnEndHit))
    {
        return;
    }

    AActor* OwnerActor = GetOwner();
    if (!OwnerActor)
    {
        return;
    }

    for (UPrimitiveComponent* PrimitiveComponent : OwnerActor->GetPrimitiveComponents())
    {
        if (!PrimitiveComponent)
        {
            continue;
        }

        if (((bWantsOverlap && LuaOnOverlap) || (bWantsEndOverlap && LuaOnEndOverlap)) && PrimitiveComponent->
            GetGenerateOverlapEvents())
        {
            BoundOverlapComponents.push_back(PrimitiveComponent);
            BeginOverlapHandles.push_back(
                LuaOnOverlap
                ? PrimitiveComponent->OnComponentBeginOverlap.AddWeakUObject(this, &ULuaBlueprintComponent::HandleBeginOverlap)
                : FDelegateHandle()
            );
            EndOverlapHandles.push_back(
                LuaOnEndOverlap
                ? PrimitiveComponent->OnComponentEndOverlap.AddWeakUObject(this, &ULuaBlueprintComponent::HandleEndOverlap)
                : FDelegateHandle()
            );
        }

        if ((bWantsHit && LuaOnHit) || (bWantsEndHit && LuaOnEndHit))
        {
            BoundHitComponents.push_back(PrimitiveComponent);
            HitHandles.push_back(
                LuaOnHit ? PrimitiveComponent->OnComponentHit.AddWeakUObject(this, &ULuaBlueprintComponent::HandleHit)
                : FDelegateHandle()
            );
            EndHitHandles.push_back(
                LuaOnEndHit ? PrimitiveComponent->OnComponentEndHit.AddWeakUObject(this, &ULuaBlueprintComponent::HandleEndHit)
                : FDelegateHandle()
            );
        }
    }
}

void ULuaBlueprintComponent::ClearCollisionBindings()
{
    for (int32 Index = 0; Index < static_cast<int32>(BoundOverlapComponents.size()); ++Index)
    {
        UPrimitiveComponent* PrimitiveComponent = BoundOverlapComponents[Index];
        if (!PrimitiveComponent)
        {
            continue;
        }

        if (Index < static_cast<int32>(BeginOverlapHandles.size()) && BeginOverlapHandles[Index].IsValid())
        {
            PrimitiveComponent->OnComponentBeginOverlap.Remove(BeginOverlapHandles[Index]);
        }
        if (Index < static_cast<int32>(EndOverlapHandles.size()) && EndOverlapHandles[Index].IsValid())
        {
            PrimitiveComponent->OnComponentEndOverlap.Remove(EndOverlapHandles[Index]);
        }
    }
    BoundOverlapComponents.clear();
    BeginOverlapHandles.clear();
    EndOverlapHandles.clear();

    for (int32 Index = 0; Index < static_cast<int32>(BoundHitComponents.size()); ++Index)
    {
        UPrimitiveComponent* PrimitiveComponent = BoundHitComponents[Index];
        if (!PrimitiveComponent)
        {
            continue;
        }

        if (Index < static_cast<int32>(HitHandles.size()) && HitHandles[Index].IsValid())
        {
            PrimitiveComponent->OnComponentHit.Remove(HitHandles[Index]);
        }
        if (Index < static_cast<int32>(EndHitHandles.size()) && EndHitHandles[Index].IsValid())
        {
            PrimitiveComponent->OnComponentEndHit.Remove(EndHitHandles[Index]);
        }
    }
    BoundHitComponents.clear();
    HitHandles.clear();
    EndHitHandles.clear();
}

FString ULuaBlueprintComponent::GetRuntimeName() const
{
    if (!BlueprintPath.empty())
    {
        return BlueprintPath;
    }
    ULuaBlueprintAsset* ValidAsset = GetValidBlueprintAsset();
    if (ValidAsset && !ValidAsset->GetSourcePath().empty())
    {
        return ValidAsset->GetSourcePath();
    }
    return "TransientLuaBlueprint";
}

void ULuaBlueprintComponent::InitializeRuntimeObjectVariables()
{
    RuntimeObjectVariables.clear();
    ULuaBlueprintAsset* ValidAsset = GetValidBlueprintAsset();
    if (!ValidAsset)
    {
        return;
    }

    for (const FLuaBlueprintVariable& Variable : ValidAsset->GetVariables())
    {
        if (Variable.Type == ELuaBlueprintPinType::Object)
        {
            InitRuntimeObjectVariable(Variable.Name.ToString(), Variable.bStrongObject);
        }
    }
}

void ULuaBlueprintComponent::InitRuntimeObjectVariable(const FString& Name, bool bStrong)
{
    if (Name.empty())
    {
        return;
    }

    for (FLuaBlueprintRuntimeObjectVariable& Variable : RuntimeObjectVariables)
    {
        if (Variable.Name == Name)
        {
            Variable.bStrong = bStrong;
            return;
        }
    }

    FLuaBlueprintRuntimeObjectVariable Variable;
    Variable.Name    = Name;
    Variable.bStrong = bStrong;
    RuntimeObjectVariables.push_back(std::move(Variable));
}

void ULuaBlueprintComponent::SetRuntimeObjectVariable(const FString& Name, sol::object Value)
{
    if (Name.empty())
    {
        return;
    }

    UObject* ObjectValue = nullptr;
    if (Value.valid() && Value.get_type() != sol::type::nil)
    {
        if (Value.is<UObject*>())
        {
            ObjectValue = Value.as<UObject*>();
        }
        else if (Value.is<AActor*>())
        {
            ObjectValue = Value.as<AActor*>();
        }
        else if (Value.is<UPrimitiveComponent*>())
        {
            ObjectValue = Value.as<UPrimitiveComponent*>();
        }
    }

    bool bFoundVariable = false;
    for (const FLuaBlueprintRuntimeObjectVariable& Existing : RuntimeObjectVariables)
    {
        if (Existing.Name == Name)
        {
            bFoundVariable = true;
            break;
        }
    }
    if (!bFoundVariable)
    {
        InitRuntimeObjectVariable(Name, false);
    }

    for (FLuaBlueprintRuntimeObjectVariable& Variable : RuntimeObjectVariables)
    {
        if (Variable.Name == Name)
        {
            if (Variable.bStrong)
            {
                Variable.StrongValue = ObjectValue;
                Variable.WeakValue.Reset();
            }
            else
            {
                Variable.WeakValue   = ObjectValue;
                Variable.StrongValue = nullptr;
            }
            return;
        }
    }
}

UObject* ULuaBlueprintComponent::GetRuntimeObjectVariable(const FString& Name) const
{
    for (const FLuaBlueprintRuntimeObjectVariable& Variable : RuntimeObjectVariables)
    {
        if (Variable.Name == Name)
        {
            return Variable.bStrong ? Variable.StrongValue : Variable.WeakValue.Get();
        }
    }
    return nullptr;
}

bool ULuaBlueprintComponent::IsLuaRuntimeGenerationValid(uint32 Generation) const
{
    return Env.valid() && LuaRuntimeGeneration == Generation && !bPendingLuaCleanup;
}

void ULuaBlueprintComponent::ScheduleLuaDelay(float Seconds, sol::protected_function Callback, uint32 Generation)
{
    if (!Callback.valid() || !IsLuaRuntimeGenerationValid(Generation))
    {
        return;
    }

    sol::state& Lua = FLuaScriptManager::GetState();
    sol::object CoroutineObject = Lua["CoroutineManager"];
    if (!CoroutineObject.valid() || CoroutineObject.get_type() != sol::type::table)
    {
        UE_LOG("LuaBlueprint Delay skipped in %s: CoroutineManager is not available", GetRuntimeName().c_str());
        return;
    }

    sol::table CoroutineManager = CoroutineObject.as<sol::table>();
    sol::object DelayObject = CoroutineManager["Delay"];
    if (!DelayObject.valid() || DelayObject.get_type() != sol::type::function)
    {
        UE_LOG("LuaBlueprint Delay skipped in %s: CoroutineManager.Delay is not available", GetRuntimeName().c_str());
        return;
    }

    TWeakObjectPtr<ULuaBlueprintComponent> WeakThis(this);
    sol::protected_function SafeCallback = Callback;
    sol::protected_function DelayFunction = DelayObject;
    sol::protected_function_result DelayResult = DelayFunction(
        Seconds,
        [WeakThis, Generation, SafeCallback]() mutable
        {
            ULuaBlueprintComponent* Owner = WeakThis.Get();
            if (!Owner || !Owner->IsLuaRuntimeGenerationValid(Generation))
            {
                return;
            }

            FLuaCallScope Scope(Owner);
            sol::protected_function_result CallbackResult = SafeCallback();
            if (!CallbackResult.valid())
            {
                sol::error Err = CallbackResult;
                UE_LOG("LuaBlueprint Delay callback error in %s: %s", Owner->GetRuntimeName().c_str(), Err.what());
            }
        }
    );

    if (!DelayResult.valid())
    {
        sol::error Err = DelayResult;
        UE_LOG("LuaBlueprint Delay scheduling error in %s: %s", GetRuntimeName().c_str(), Err.what());
    }
}

bool ULuaBlueprintComponent::ReadEventFlag(const char* EventName) const
{
    if (!EventName || !Env.valid())
    {
        return false;
    }

    sol::object EventsObject = Env["__events"];
    if (!EventsObject.valid() || EventsObject.get_type() != sol::type::table)
    {
        sol::object FunctionObject = Env[EventName];
        return FunctionObject.valid() && FunctionObject.get_type() == sol::type::function;
    }

    sol::table  Events = EventsObject.as<sol::table>();
    sol::object Flag   = Events[EventName];
    return Flag.valid() && Flag.get_type() == sol::type::boolean && Flag.as<bool>();
}

void ULuaBlueprintComponent::InvokeLuaEndPlay()
{
    if (bHasCalledLuaEndPlay || !bWantsEndPlay || !LuaEndPlay)
    {
        return;
    }

    bHasCalledLuaEndPlay = true;
    FLuaCallScope                  Scope(this);
    sol::protected_function_result Result = LuaEndPlay();
    if (!Result.valid())
    {
        sol::error Err = Result;
        UE_LOG("LuaBlueprint EndPlay error in %s: %s", GetRuntimeName().c_str(), Err.what());
    }
}

void ULuaBlueprintComponent::HandleDeferredLuaCleanup()
{
    if (LuaCallDepth != 0 || !bPendingLuaCleanup)
    {
        return;
    }

    const bool bShouldRunEndPlay = bPendingLuaEndPlay;
    bPendingLuaCleanup           = false;
    bPendingLuaEndPlay           = false;
    if (bShouldRunEndPlay)
    {
        InvokeLuaEndPlay();
    }
    ClearLuaRuntime();
}

void ULuaBlueprintComponent::HandleBeginOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor*              OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 /*OtherBodyIndex*/,
    bool /*bFromSweep*/,
    const FHitResult& /*SweepResult*/
    )
{
    if (!IsValid(this) || bPendingLuaCleanup || !Env.valid() || !LuaOnOverlap)
    {
        return;
    }

    AActor* SafeOtherActor = IsValid(OtherActor) ? OtherActor : nullptr;
    UPrimitiveComponent* SafeOverlappedComponent = IsValid(OverlappedComponent) ? OverlappedComponent : nullptr;
    UPrimitiveComponent* SafeOtherComp = IsValid(OtherComp) ? OtherComp : nullptr;
    {
        FLuaCallScope                  Scope(this);
        sol::protected_function_result Result = LuaOnOverlap(SafeOtherActor, SafeOverlappedComponent, SafeOtherComp);
        if (!Result.valid())
        {
            sol::error Err = Result;
            UE_LOG("LuaBlueprint OnOverlap error in %s: %s", GetRuntimeName().c_str(), Err.what());
        }
    }
}

void ULuaBlueprintComponent::HandleEndOverlap(
    UPrimitiveComponent* OverlappedComponent,
    AActor*              OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 /*OtherBodyIndex*/
    )
{
    if (!IsValid(this) || bPendingLuaCleanup || !Env.valid() || !LuaOnEndOverlap)
    {
        return;
    }

    AActor* SafeOtherActor = IsValid(OtherActor) ? OtherActor : nullptr;
    UPrimitiveComponent* SafeOverlappedComponent = IsValid(OverlappedComponent) ? OverlappedComponent : nullptr;
    UPrimitiveComponent* SafeOtherComp = IsValid(OtherComp) ? OtherComp : nullptr;
    {
        FLuaCallScope                  Scope(this);
        sol::protected_function_result Result = LuaOnEndOverlap(SafeOtherActor, SafeOverlappedComponent, SafeOtherComp);
        if (!Result.valid())
        {
            sol::error Err = Result;
            UE_LOG("LuaBlueprint OnEndOverlap error in %s: %s", GetRuntimeName().c_str(), Err.what());
        }
    }
}

void ULuaBlueprintComponent::HandleHit(
    UPrimitiveComponent* HitComponent,
    AActor*              OtherActor,
    UPrimitiveComponent* OtherComp,
    FVector              NormalImpulse,
    const FHitResult&    HitResult
    )
{
    if (!IsValid(this) || bPendingLuaCleanup || !Env.valid() || !LuaOnHit)
    {
        return;
    }

    AActor* SafeOtherActor = IsValid(OtherActor) ? OtherActor : nullptr;
    UPrimitiveComponent* SafeHitComponent = IsValid(HitComponent) ? HitComponent : nullptr;
    UPrimitiveComponent* SafeOtherComp = IsValid(OtherComp) ? OtherComp : nullptr;
    {
        FLuaCallScope                  Scope(this);
        sol::protected_function_result Result = LuaOnHit(SafeOtherActor, SafeHitComponent, SafeOtherComp, NormalImpulse, HitResult);
        if (!Result.valid())
        {
            sol::error Err = Result;
            UE_LOG("LuaBlueprint OnHit error in %s: %s", GetRuntimeName().c_str(), Err.what());
        }
    }
}

void ULuaBlueprintComponent::HandleEndHit(
    UPrimitiveComponent* HitComponent,
    AActor*              OtherActor,
    UPrimitiveComponent* OtherComp
    )
{
    if (!IsValid(this) || bPendingLuaCleanup || !Env.valid() || !LuaOnEndHit)
    {
        return;
    }

    AActor* SafeOtherActor = IsValid(OtherActor) ? OtherActor : nullptr;
    UPrimitiveComponent* SafeHitComponent = IsValid(HitComponent) ? HitComponent : nullptr;
    UPrimitiveComponent* SafeOtherComp = IsValid(OtherComp) ? OtherComp : nullptr;
    {
        FLuaCallScope                  Scope(this);
        sol::protected_function_result Result = LuaOnEndHit(SafeOtherActor, SafeHitComponent, SafeOtherComp);
        if (!Result.valid())
        {
            sol::error Err = Result;
            UE_LOG("LuaBlueprint OnEndHit error in %s: %s", GetRuntimeName().c_str(), Err.what());
        }
    }
}
