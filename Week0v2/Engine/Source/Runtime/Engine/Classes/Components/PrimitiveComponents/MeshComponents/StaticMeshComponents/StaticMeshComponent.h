#pragma once
#include "Components/PrimitiveComponents/MeshComponents/MeshComponent.h"

class UStaticMesh;

struct FStaticMeshComponentInfo : public FPrimitiveComponentInfo
{
    DECLARE_ACTORCOMPONENT_INFO(FStaticMeshComponentInfo);
    FWString StaticMeshPath;

    FStaticMeshComponentInfo()
        : FPrimitiveComponentInfo()
        , StaticMeshPath(L"")
    {
        InfoType = TEXT("FStaticMeshComponentInfo");
        ComponentClass = TEXT("UStaticMeshComponent");
    }

    virtual void Serialize(FArchive& ar) const override
    {
        FPrimitiveComponentInfo::Serialize(ar);
        ar << StaticMeshPath;
    }

    virtual void Deserialize(FArchive& ar) override
    {
        FPrimitiveComponentInfo::Deserialize(ar);
        ar >> StaticMeshPath;
    }
};

class UStaticMeshComponent : public UMeshComponent
{
    DECLARE_CLASS(UStaticMeshComponent, UMeshComponent)

public:
    UStaticMeshComponent() = default;
    UStaticMeshComponent(const UStaticMeshComponent& Other);

    virtual UObject* Duplicate() const override;
    virtual void DuplicateSubObjects(const UObject* Source) override;
    virtual void PostDuplicate() override;
    virtual void TickComponent(float DeltaTime) override;

    UPROPERTY(int, selectedSubMeshIndex);



    virtual uint32 GetNumMaterials() const override;
    virtual UMaterial* GetMaterial(uint32 ElementIndex) const override;
    virtual uint32 GetMaterialIndex(FName MaterialSlotName) const override;
    virtual TArray<FName> GetMaterialSlotNames() const override;
    virtual void GetUsedMaterials(TArray<UMaterial*>& Out) const override;

    virtual int CheckRayIntersection(FVector& rayOrigin, FVector& rayDirection, float& pfNearHitDistance) override;
    
    UStaticMesh* GetStaticMesh() const { return staticMesh; }
    void SetStaticMesh(UStaticMesh* value);

    
    std::unique_ptr<FActorComponentInfo> GetComponentInfo() override;
    virtual void SaveComponentInfo(FActorComponentInfo& OutInfo) override;
    virtual void LoadAndConstruct(const FActorComponentInfo& Info) override;

protected:
    UStaticMesh* staticMesh = nullptr;
    int selectedSubMeshIndex = -1;
private:
    float Timer = 0.0f;
};