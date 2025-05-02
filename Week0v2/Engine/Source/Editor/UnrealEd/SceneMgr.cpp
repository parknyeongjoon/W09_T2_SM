#include "UnrealEd/SceneMgr.h"
#include "JSON/json.hpp"
#include "UObject/Object.h"
#include "BaseGizmos/GizmoArrowComponent.h"
#include "UObject/ObjectFactory.h"
#include <fstream>

#include "EditorViewportClient.h"
#include "Camera/CameraComponent.h"
#include "Components/LightComponents/LightComponentBase.h"
#include "Components/PrimitiveComponents/UBillboardComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/CubeComp.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/SkySphereComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/SphereComp.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Engine/FLoaderOBJ.h"
#include "LevelEditor/SLevelEditor.h"

using json = nlohmann::json;

void FSceneMgr::ParseSceneData(const FString& jsonStr)
{
    SceneData sceneData;


        json j = json::parse(*jsonStr);

        // 버전과 NextUUID 읽기
        // sceneData.Version = j["Version"].get<int>();
        sceneData.NextUUID = j["NextUUID"].get<int>();

        auto perspectiveCamera = j["PerspectiveCamera"];
        std::shared_ptr<FEditorViewportClient> activeViewportClient = GEngine->GetLevelEditor()->GetActiveViewportClient();

        if (perspectiveCamera.contains("Location")) {
            auto location = perspectiveCamera["Location"].get<std::vector<float>>();
            activeViewportClient->ViewTransformPerspective.SetLocation(FVector(location[0], location[1], location[2]));
        }
        if (perspectiveCamera.contains("Rotation")) {
            auto rotation = perspectiveCamera["Rotation"].get<std::vector<float>>();
            activeViewportClient->ViewTransformPerspective.SetRotation(FVector(rotation[0], rotation[1], rotation[2]));
        }
        if (perspectiveCamera.contains("FOV")) {
            auto fovArray = perspectiveCamera["FOV"].get<std::vector<float>>();
            activeViewportClient->SetViewFOV(fovArray[0]);
        }
        if (perspectiveCamera.contains("NearClip")) {
            auto nearClipArray = perspectiveCamera["NearClip"].get<std::vector<float>>();
            activeViewportClient->SetNearClip(nearClipArray[0]);
        }
        if (perspectiveCamera.contains("FarClip")) {
            auto farClipArray = perspectiveCamera["FarClip"].get<std::vector<float>>();
            activeViewportClient->SetFarClip(farClipArray[0]);
        }
        // Primitives 처리 (C++14 스타일)
        auto primitives = j["Primitives"];
        for (auto it = primitives.begin(); it != primitives.end(); ++it)
        {
            int id = std::stoi(it.key());  // Key는 문자열, 숫자로 변환
            const json& value = it.value();
            UObject* obj = nullptr;
            if (value.contains("Type"))
            {
                const FString TypeName = value["Type"].get<std::string>();
                if (TypeName == USphereComp::StaticClass()->GetName())
                {
                    obj = FObjectFactory::ConstructObject<USphereComp>();
                }
                else if (TypeName == UCubeComp::StaticClass()->GetName())
                {
                    obj = FObjectFactory::ConstructObject<UCubeComp>();
                }
                else if (TypeName == UGizmoArrowComponent::StaticClass()->GetName())
                {
                    obj = FObjectFactory::ConstructObject<UGizmoArrowComponent>();
                }
                else if (TypeName == UBillboardComponent::StaticClass()->GetName())
                {
                    obj = FObjectFactory::ConstructObject<UBillboardComponent>();
                }
                else if (TypeName == ULightComponentBase::StaticClass()->GetName())
                {
                    obj = FObjectFactory::ConstructObject<ULightComponentBase>();
                }
                else if (TypeName == USkySphereComponent::StaticClass()->GetName())
                {
                    obj = FObjectFactory::ConstructObject<USkySphereComponent>();
                    USkySphereComponent* skySphere = static_cast<USkySphereComponent*>(obj);
                }
                else if (TypeName == "StaticMeshComp")
                {
                    UWorld* World = GEngine->GetWorld();
                    AActor* SpawnedActor = nullptr;
                    std::string Path = value["ObjStaticMeshAsset"].get<std::string>();
                    FString FileName = Path.substr(Path.find_last_of("/\\") + 1);
                    FWString WFileName = FileName.ToWideString();

                    AStaticMeshActor* TempActor = World->SpawnActor<AStaticMeshActor>();
                    TempActor->SetActorLocation(FVector(value["Location"].get<std::vector<float>>()[0],
                          value["Location"].get<std::vector<float>>()[1],
                          value["Location"].get<std::vector<float>>()[2]));
                    TempActor->SetActorRotation(FVector(value["Rotation"].get<std::vector<float>>()[0],
                        value["Rotation"].get<std::vector<float>>()[1],
                        value["Rotation"].get<std::vector<float>>()[2]));
                    TempActor->SetActorScale(FVector(value["Scale"].get<std::vector<float>>()[0],
                        value["Scale"].get<std::vector<float>>()[1],
                        value["Scale"].get<std::vector<float>>()[2]
                        ));
                    TempActor->SetActorLabel(TEXT(TempActor->GetName()));
                    UStaticMeshComponent* MeshComp = TempActor->GetStaticMeshComponent();
                    FManagerOBJ::CreateStaticMesh("Assets/" + FileName);
                    MeshComp->SetStaticMesh(FManagerOBJ::GetStaticMesh(WFileName));
                }
            }
        }
}

FString FSceneMgr::LoadSceneFromFile(const FString& filename)
{
    std::string NewFileName = *filename;
    std::ifstream inFile(NewFileName);
    if (!inFile) {
        UE_LOG(LogLevel::Error, "Failed to open file for reading: %s", *filename);
        return FString();
    }

    json j;
    try {
        inFile >> j; // JSON 파일 읽기
    }
    catch (const std::exception& e) {
        UE_LOG(LogLevel::Error, "Error parsing JSON: %s", e.what());
        return FString();
    }

    inFile.close();

    return j.dump(4);
}

std::string FSceneMgr::SerializeSceneData(const SceneData& sceneData)
{
    json j;

    // Version과 NextUUID 저장
    j["Version"] = sceneData.Version;
    j["NextUUID"] = sceneData.NextUUID;

    // Primitives 처리 (C++17 스타일)
    for (const auto& [Id, Obj] : sceneData.Primitives)
    {
        USceneComponent* primitive = static_cast<USceneComponent*>(Obj);
        std::vector<float> Location = { primitive->GetWorldLocation().X,primitive->GetWorldLocation().Y,primitive->GetWorldLocation().Z };
        std::vector<float> Rotation = { primitive->GetWorldRotation().Pitch,primitive->GetWorldRotation().Yaw,primitive->GetWorldRotation().Roll };
        std::vector<float> Scale = { primitive->GetWorldScale().X,primitive->GetWorldScale().Y,primitive->GetWorldScale().Z };

        std::string primitiveName = *primitive->GetName();
        size_t pos = primitiveName.rfind('_');
        if (pos != INDEX_NONE) {
            primitiveName = primitiveName.substr(0, pos);
        }
        j["Primitives"][std::to_string(Id)] = {
            {"Location", Location},
            {"Rotation", Rotation},
            {"Scale", Scale},
            {"Type",primitiveName}
        };
    }

    for (const auto& [id, camera] : sceneData.Cameras)
    {
        UCameraComponent* cameraComponent = static_cast<UCameraComponent*>(camera);
        TArray<float> Location = { cameraComponent->GetWorldLocation().X, cameraComponent->GetWorldLocation().Y, cameraComponent->GetWorldLocation().Z };
        TArray<float> Rotation = { 0.0f, cameraComponent->GetWorldRotation().Yaw, cameraComponent->GetWorldRotation().Roll };
        float FOV = cameraComponent->GetFOV();
        float nearClip = cameraComponent->GetNearClip();
        float farClip = cameraComponent->GetFarClip();
    
        //
        j["PerspectiveCamera"][std::to_string(id)] = {
            {"Location", Location},
            {"Rotation", Rotation},
            {"FOV", FOV},
            {"NearClip", nearClip},
            {"FarClip", farClip}
        };
    }


    return j.dump(4); // 4는 들여쓰기 수준
}

bool FSceneMgr::SaveSceneToFile(const FString& filename, const SceneData& sceneData)
{
    std::ofstream outFile(*filename);
    if (!outFile) {
        FString errorMessage = "Failed to open file for writing: ";
        MessageBoxA(NULL, *errorMessage, "Error", MB_OK | MB_ICONERROR);
        return false;
    }

    std::string jsonData = SerializeSceneData(sceneData);
    outFile << jsonData;
    outFile.close();

    return true;
}

