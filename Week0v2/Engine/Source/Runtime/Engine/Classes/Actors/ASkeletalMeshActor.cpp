#include "ASkeletalMeshActor.h"

#include "Engine/Loader/FBX/FLoaderFBX.h"

ASkeletalMeshActor::ASkeletalMeshActor()
{
    SkeletalMeshComponent = AddComponent<USkeletalMeshComponent>(EComponentOrigin::Constructor);
    RootComponent = SkeletalMeshComponent;
    
    FSkeletalMeshData data;
    if (!FLoaderFBX::LoadSkeletalMesh("Contents/FBX/Wizard.fbx", data))
    {
        UE_LOG(LogLevel::Error, "Failed to load skeletal mesh");
    }
    
    USkeletalMesh* SkeletalMesh = FObjectFactory::ConstructObject<USkeletalMesh>();
    SkeletalMesh->SetData(data);
    
    SkeletalMeshComponent->SetSkeletalMesh(SkeletalMesh);
}