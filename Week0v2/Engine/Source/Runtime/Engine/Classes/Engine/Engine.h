#pragma once
#include "EngineBaseTypes.h"
#include "EngineTypes.h"
#include "World.h"
#include "Container/Array.h"
#include "UObject/ObjectMacros.h"


class UAssetManager;
struct FWorldContext
{
public:
    FWorldContext() : WorldType(EWorldType::None), World(nullptr){}

    EWorldType::Type WorldType;
    // TArray<FWorldContext*> ExternalReferences;
    ELevelTick LevelType;

    UWorld* GetWorld() { return World; }
    void SetWorld(UWorld* InWorld) { World = InWorld; }
    
private:
    UWorld* World;
};

class UEngine : public UObject
{
    DECLARE_CLASS(UEngine, UObject)

public:
    UEngine() = default;
    virtual ~UEngine() = default;

public:
    virtual void Init();
    virtual void Tick(float DeltaTime);
    virtual void Release() {}

    UAssetManager* AssetManager = nullptr;

    static inline UINT GFrameCount = 0;

protected:
    TMap<UWorld*, std::shared_ptr<FWorldContext>> WorldContexts;
};
