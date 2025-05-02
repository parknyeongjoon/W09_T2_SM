#include "PropertyEditorPanel.h"

#include "Engine/World.h"
#include "Engine/FLoaderOBJ.h"
#include "Math/MathUtility.h"
#include "UnrealEd/ImGuiWidget.h"
#include "UObject/Casts.h"

#include "Components/GameFramework/ProjectileMovementComponent.h"
#include "Components/GameFramework/RotatingMovementComponent.h"
#include <Math/JungleMath.h>
#include <UObject/UObjectIterator.h>

#include "Components/LuaComponent.h"
#include "Components/LightComponents/DirectionalLightComponent.h"
#include "Components/LightComponents/PointLightComponent.h"
#include "Components/LightComponents/SpotLightComponent.h"
#include "Components/Mesh/StaticMesh.h"
#include "Components/PrimitiveComponents/HeightFogComponent.h"
#include "Components/PrimitiveComponents/UParticleSubUVComp.h"
#include "Components/PrimitiveComponents/UTextComponent.h"
#include "Components/PrimitiveComponents/MeshComponents/StaticMeshComponents/CubeComp.h"
#include "Components/PrimitiveComponents/Physics/UShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/UBoxShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/USphereShapeComponent.h"
#include "Components/PrimitiveComponents/Physics/UCapsuleShapeComponent.h"

#include "LevelEditor/SLevelEditor.h"
#include "tinyfiledialogs/tinyfiledialogs.h"
#include "UnrealEd/EditorViewportClient.h"
#include <windows.h> // 기본적인 Windows API 포함
#include <shellapi.h> // ShellExecute 관련 함수 정의 포함
#include <filesystem> // C++17 filesystem 사용
#include "UObject/FunctionRegistry.h"


void PropertyEditorPanel::Render()
{
    // TODO PickedComponent 패널에서 뺴기 우선 임시용으로 배치
    if ((GetAsyncKeyState(VK_DELETE) & 0x8000))
    {
        if (PickedComponent != nullptr)
        {
            AActor* PickedActor = nullptr;
            PickedActor = *GEngine->GetWorld()->GetSelectedActors().begin();

            //루트 컴포넌트면 삭제 불가
            if (PickedComponent != PickedActor->GetRootComponent())
            {
                PickedComponent->DestroyComponent();
                PickedComponent = nullptr;
            }
        }
    }
    /* Pre Setup */
    float PanelWidth = (Width) * 0.2f - 6.0f;
    float PanelHeight = (Height) * 0.65f;

    float PanelPosX = (Width) * 0.8f + 5.0f;
    float PanelPosY = (Height) * 0.3f + 15.0f;

    ImVec2 MinSize(140, 370);
    ImVec2 MaxSize(FLT_MAX, 900);

    /* Min, Max Size */
    ImGui::SetNextWindowSizeConstraints(MinSize, MaxSize);

    /* Panel Position */
    ImGui::SetNextWindowPos(ImVec2(PanelPosX, PanelPosY), ImGuiCond_Always);

    /* Panel Size */
    ImGui::SetNextWindowSize(ImVec2(PanelWidth, PanelHeight), ImGuiCond_Always);

    /* Panel Flags */
    ImGuiWindowFlags PanelFlags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

    /* Render Start */
    ImGui::Begin("Detail", nullptr, PanelFlags);

    AActor* PickedActor = nullptr;
    AEditorPlayer* player = GEngine->GetWorld()->GetEditorPlayer();
    if (!GEngine->GetWorld()->GetSelectedActors().IsEmpty())
            PickedActor = *GEngine->GetWorld()->GetSelectedActors().begin();

    ImVec2 imageSize = ImVec2(256, 256); // 이미지 출력 크기

    // TODO: 추후에 RTTI를 이용해서 프로퍼티 출력하기
    if (PickedActor)
    {
        if (ImGui::TreeNodeEx("Components", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_OpenOnArrow))
        {
            const TArray<UActorComponent*>& AllComponents = PickedActor->GetComponents();
            
            for (UActorComponent* Component : AllComponents)
            {
                if (USceneComponent* SceneComp = Cast<USceneComponent>(Component))
                {
                    if (SceneComp->GetAttachParent() == nullptr)
                    {
                        DrawSceneComponentTree(SceneComp, PickedComponent);
                    }
                }
                else
                {
                    DrawActorComponent(Component, PickedComponent);
                }
            }

            if (ImGui::Button("+", ImVec2(ImGui::GetWindowContentRegionMax().x * 0.9f, 32)))
            {
                ImGui::OpenPopup("AddComponentPopup");
            }

            // 팝업 메뉴
            if (ImGui::BeginPopup("AddComponentPopup"))
            {
                if (ImGui::Selectable("TextComponent"))
                {
                    UTextComponent* TextComponent = PickedActor->AddComponent<UTextComponent>(EComponentOrigin::Editor);
                    if (USceneComponent* ParentComponent = Cast<USceneComponent>(PickedComponent))
                    {
                        TextComponent->DetachFromParent();
                        TextComponent->SetupAttachment(ParentComponent);
                    }
                    PickedComponent = TextComponent;
                    TextComponent->SetTexture(L"Assets/Texture/font.png");
                    TextComponent->SetRowColumnCount(106, 106);
                    TextComponent->SetText(L"안녕하세요 Jungle");
                }
                if (ImGui::Selectable("BillboardComponent"))
                {
                    UBillboardComponent* BillboardComponent = PickedActor->AddComponent<UBillboardComponent>(EComponentOrigin::Editor);
                    if (USceneComponent* ParentComponent = Cast<USceneComponent>(PickedComponent))
                    {
                        BillboardComponent->DetachFromParent();
                        BillboardComponent->SetupAttachment(ParentComponent);
                    }
                    PickedComponent = BillboardComponent;
                    BillboardComponent->SetTexture(L"Assets/Texture/Pawn_64x.png");
                    BillboardComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
                }
                //if (ImGui::Selectable("LightComponent"))
                //{
                //    ULightComponentBase* LightComponent = PickedActor->AddComponent<ULightComponentBase>();
                //    PickedComponent = LightComponent;
                //}
                if (ImGui::Selectable("DirectionalLightComponent"))
                {
                    UDirectionalLightComponent* DirectionalLightComponent = PickedActor->AddComponent<UDirectionalLightComponent>(EComponentOrigin::Editor);
                    if (USceneComponent* ParentComponent = Cast<USceneComponent>(PickedComponent))
                    {
                        DirectionalLightComponent->DetachFromParent();
                        DirectionalLightComponent->SetupAttachment(ParentComponent);
                    }
                    PickedComponent = DirectionalLightComponent;
                }
                if (ImGui::Selectable("PointLightComponent"))
                {
                    UPointLightComponent* PointLightComponent = PickedActor->AddComponent<UPointLightComponent>(EComponentOrigin::Editor);
                    if (USceneComponent* ParentComponent = Cast<USceneComponent>(PickedComponent))
                    {
                        PointLightComponent->DetachFromParent();
                        PointLightComponent->SetupAttachment(ParentComponent);
                    }
                    PickedComponent = PointLightComponent;
                }
                if (ImGui::Selectable("SpotLightComponent"))
                {
                    USpotLightComponent* SpotLightComponent = PickedActor->AddComponent<USpotLightComponent>(EComponentOrigin::Editor);
                    PickedComponent = SpotLightComponent;
                }
                if (ImGui::Selectable("ParticleComponent"))
                {
                    UParticleSubUVComp* ParticleComponent = PickedActor->AddComponent<UParticleSubUVComp>(EComponentOrigin::Editor);
                    if (USceneComponent* ParentComponent = Cast<USceneComponent>(PickedComponent))
                    {
                        ParticleComponent->DetachFromParent();
                        ParticleComponent->SetupAttachment(ParentComponent);
                    }
                    PickedComponent = ParticleComponent;
                    ParticleComponent->SetTexture(L"Assets/Texture/T_Explosion_SubUV.png");
                    ParticleComponent->SetRowColumnCount(6, 6);
                    ParticleComponent->SetRelativeScale(FVector(10.0f, 10.0f, 1.0f));
                    ParticleComponent->Activate();
                }
                if (ImGui::Selectable("StaticMeshComponent"))
                {
                    UStaticMeshComponent* StaticMeshComponent = PickedActor->AddComponent<UStaticMeshComponent>(EComponentOrigin::Editor);
                    if (USceneComponent* ParentComponent = Cast<USceneComponent>(PickedComponent))
                    {
                        StaticMeshComponent->DetachFromParent();
                        StaticMeshComponent->SetupAttachment(ParentComponent);
                    }
                    PickedComponent = StaticMeshComponent;
                    FManagerOBJ::CreateStaticMesh("Assets/Cube.obj");
                    StaticMeshComponent->SetStaticMesh(FManagerOBJ::GetStaticMesh(L"Cube.obj"));
                }
                if (ImGui::Selectable("CubeComponent"))
                {
                    UCubeComp* CubeComponent = PickedActor->AddComponent<UCubeComp>(EComponentOrigin::Editor);
                    if (USceneComponent* ParentComponent = Cast<USceneComponent>(PickedComponent))
                    {
                        CubeComponent->DetachFromParent();
                        CubeComponent->SetupAttachment(ParentComponent);
                    }
                    PickedComponent = CubeComponent;
                }

                if (ImGui::Selectable("ProjectileMovementComponent"))
                {
                    UProjectileMovementComponent* ProjectileComp = PickedActor->AddComponent<UProjectileMovementComponent>(EComponentOrigin::Editor);
                    PickedComponent = ProjectileComp;
                }

                if (ImGui::Selectable("RotatingMovementComponent"))
                {
                    URotatingMovementComponent* RotatingComponent = PickedActor->AddComponent<URotatingMovementComponent>(EComponentOrigin::Editor);
                    PickedComponent = RotatingComponent;
                }
                
                if (ImGui::Selectable("LuaComponent"))
                {
                    ULuaComponent* LuaComponent = PickedActor->AddComponent<ULuaComponent>(EComponentOrigin::Editor);
                    PickedComponent = LuaComponent;
                }
                
                if (ImGui::Selectable("HeartComponent"))
                {
                    UBillboardComponent* BillboardComponent = PickedActor->AddComponent<UBillboardComponent>(EComponentOrigin::Editor);
                    if (USceneComponent* ParentComponent = Cast<USceneComponent>(PickedComponent))
                    {
                        BillboardComponent->DetachFromParent();
                        BillboardComponent->SetupAttachment(ParentComponent);
                    }
                    PickedComponent = BillboardComponent;
                    BillboardComponent->SetTexture(L"Assets/Texture/heartpixelart.png");
                    BillboardComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 3.0f));
                    BillboardComponent->bOnlyForEditor = false;
                }

                ImGui::EndPopup();
            }
            ImGui::TreePop();
        }
    }

    if (PickedActor) // Delegate Test
    {
        RenderDelegate(GEngine->GetWorld()->GetLevel());
    }

    if (PickedActor)
    {
        if (PickedComponent && PickedComponent->GetOwner() && PickedComponent->GetOwner() != PickedActor)
        {
            // 다른 액터를 픽한 것 -> PickedComponent를 PickedActor의 RootComponent로 바꿔준다
            PickedComponent = PickedActor->GetRootComponent();
        }
    }
    
    if (PickedActor && PickedComponent && PickedComponent->IsA<ULuaComponent>())
    {
        ULuaComponent* LuaComp = Cast<ULuaComponent>(PickedComponent);
        RenderForLua(LuaComp);
    }

    // TODO: 추후에 RTTI를 이용해서 프로퍼티 출력하기
    if (PickedActor && PickedComponent && PickedComponent->IsA<USceneComponent>())
    {
        USceneComponent* SceneComp = Cast<USceneComponent>(PickedComponent);
        ImGui::SetItemDefaultFocus();
        // TreeNode 배경색을 변경 (기본 상태)
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
        {
            Location = SceneComp->GetRelativeLocation();
            Rotation = SceneComp->GetRelativeRotation().ToVector();
            Scale = SceneComp->GetRelativeScale();
            if (PickedComponent != LastComponent)
            {
                LastComponent = PickedComponent;
                bFirstFrame = true;
            }

            bool bChanged = false;

            bChanged |= FImGuiWidget::DrawVec3Control("Location", Location, 0, 85);
            ImGui::Spacing();

            bChanged |= FImGuiWidget::DrawVec3Control("Rotation", Rotation, 0, 85);
            ImGui::Spacing();

            bChanged |= FImGuiWidget::DrawVec3Control("Scale", Scale, 0, 85);
            ImGui::Spacing();

            if (bChanged && !bFirstFrame)
            {
                SceneComp->SetRelativeLocation(Location);
                SceneComp->SetRelativeRotation(Rotation);
                SceneComp->SetRelativeScale(Scale);
            }
            
            //always local
            //std::string coordiButtonLabel;
            //if (player->GetCoordiMode() == CoordiMode::CDM_WORLD)
            //    coordiButtonLabel = "World";
            //else if (player->GetCoordiMode() == CoordiMode::CDM_LOCAL)
            //    coordiButtonLabel = "Local";

            //if (ImGui::Button(coordiButtonLabel.c_str(), ImVec2(ImGui::GetWindowContentRegionMax().x * 0.9f, 32)))
            //{
            //    player->AddCoordiMode();
            //}
            ImGui::TreePop(); // 트리 닫기
        }
        ImGui::PopStyleColor();
        bFirstFrame = false;
    }

    if (PickedActor && PickedComponent && PickedComponent->IsA<ULightComponentBase>())
    {
        ULightComponentBase* lightObj = Cast<ULightComponentBase>(PickedComponent);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        if (ImGui::TreeNodeEx("Light Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
        {
            FVector4 currColor = lightObj->GetLightColor();

            float r = currColor.x;
            float g = currColor.y;
            float b = currColor.z;
            float a = currColor.w;
            float h, s, v;
            float lightColor[4] = { r, g, b, a };

            // SpotLight Color
            if (ImGui::ColorPicker4("##SpotLight Color", lightColor,
                ImGuiColorEditFlags_DisplayRGB |
                ImGuiColorEditFlags_NoSidePreview |
                ImGuiColorEditFlags_NoInputs |
                ImGuiColorEditFlags_Float))

            {

                r = lightColor[0];
                g = lightColor[1];
                b = lightColor[2];
                a = lightColor[3];
                lightObj->SetColor(FVector4(r, g, b, a));
            }
            RGBToHSV(r, g, b, h, s, v);
            // RGB/HSV
            bool changedRGB = false;
            bool changedHSV = false;

            // RGB
            ImGui::PushItemWidth(50.0f);
            if (ImGui::DragFloat("R##R", &r, 0.001f, 0.f, 1.f)) changedRGB = true;
            ImGui::SameLine();
            if (ImGui::DragFloat("G##G", &g, 0.001f, 0.f, 1.f)) changedRGB = true;
            ImGui::SameLine();
            if (ImGui::DragFloat("B##B", &b, 0.001f, 0.f, 1.f)) changedRGB = true;
            ImGui::Spacing();

            // HSV
            if (ImGui::DragFloat("H##H", &h, 0.1f, 0.f, 360)) changedHSV = true;
            ImGui::SameLine();
            if (ImGui::DragFloat("S##S", &s, 0.001f, 0.f, 1)) changedHSV = true;
            ImGui::SameLine();
            if (ImGui::DragFloat("V##V", &v, 0.001f, 0.f, 1)) changedHSV = true;
            ImGui::PopItemWidth();
            ImGui::Spacing();

            if (changedRGB && !changedHSV)
            {
                // RGB -> HSV
                RGBToHSV(r, g, b, h, s, v);
                lightObj->SetColor(FVector4(r, g, b, a));
            }
            else if (changedHSV && !changedRGB)
            {
                // HSV -> RGB
                HSVToRGB(h, s, v, r, g, b);
                lightObj->SetColor(FVector4(r, g, b, a));
            }

            // Light Radius
            //float radiusVal = lightObj->GetRadius();
            //if (ImGui::SliderFloat("Radius", &radiusVal, 1.0f, 100.0f))
            //{
            //    lightObj->SetRadius(radiusVal);
            //}
            ImGui::TreePop();
        }
        ImGui::PopStyleColor();

        //shadow on/off
        bool bCastShadow = lightObj->CanCastShadows();
        if (ImGui::Checkbox("Cast Shadow", &bCastShadow))
        {
            lightObj->SetCastShadows(bCastShadow);
        }

        // show intensity
        float intensityVal = lightObj->GetIntensity();
        if (ImGui::SliderFloat("Intensity", &intensityVal, 0.01f, 10.0f))
        {
            lightObj->SetIntensity(intensityVal);
        }
        ImGui::Spacing();

        if (PickedComponent->IsA<UDirectionalLightComponent>())
        {
            // direction
            UDirectionalLightComponent* DirectionalLight = Cast<UDirectionalLightComponent>(PickedComponent);
            bool override = Cast<UDirectionalLightComponent>(GEngine->GetLevelEditor()->GetActiveViewportClient()->GetOverrideComponent());
            if (ImGui::Checkbox("Override Camera", &override))
            {
                if (override)
                {
                    GEngine->GetLevelEditor()->GetActiveViewportClient()->SetOverrideComponent(DirectionalLight);
                }
                else
                {
                    GEngine->GetLevelEditor()->GetActiveViewportClient()->SetOverrideComponent(nullptr);
                }
            }
            ImTextureID LightDepth = reinterpret_cast<ImTextureID>(DirectionalLight->GetShadowResource()->GetSRV());
            ImGui::Text("Shadow Map");
            ImGui::Image(LightDepth, imageSize);
            ImTextureID LightDepth1 = reinterpret_cast<ImTextureID>(DirectionalLight->GetShadowResource()[1].GetSRV());
            ImGui::Text("Shadow Map");
            ImGui::Image(LightDepth1, imageSize);
            ImTextureID LightDepth2 = reinterpret_cast<ImTextureID>(DirectionalLight->GetShadowResource()[2].GetSRV());
            ImGui::Text("Shadow Map");
            ImGui::Image(LightDepth2, imageSize);
            ImTextureID LightDepth3 = reinterpret_cast<ImTextureID>(DirectionalLight->GetShadowResource()[3].GetSRV());
            ImGui::Text("Shadow Map");
            ImGui::Image(LightDepth3, imageSize);

            // ImTextureID LightTexture = reinterpret_cast<ImTextureID>(DirectionalLight->GetLightMap()->TextureSRV);
            //
            // ImGui::Text("Light Depth View");
            // ImGui::Image(LightTexture, imageSize);
            //TODO: 회전 각에 맞춰 direction 변동
            /*if (DirectionalLight)
            {
                FVector LightDirection = DirectionalLight->GetDirection();
                bool bChanged = FImGuiWidget::DrawVec3Control("Direction", LightDirection, 0, 85);
                ImGui::Spacing();

                if (bChanged)
                {
                    DirectionalLight->SetDirection(LightDirection);
                }
            }*/
        }

        if (PickedComponent->IsA<UPointLightComponent>())
        {
            // radius
            UPointLightComponent* PointLight = Cast<UPointLightComponent>(PickedComponent);
            if (PointLight)
            {
                float radiusVal = PointLight->GetRadius();
                if (ImGui::SliderFloat("Radius", &radiusVal, 1.0f, 100.0f))
                {
                    PointLight->SetRadius(radiusVal);
                }
                ImGui::Spacing();
                //float attenuationVal = PointLight->GetAttenuationFalloff();
                //if (ImGui::SliderFloat("Attenuation", &attenuationVal, 0.0001f, 0.001f))
                //{
                //    PointLight->SetAttenuationFallOff(attenuationVal);
                //}
            }

            static int selectedFace = 0;
            static const char* faceNames[] = {
                "+X (Right)", "-X (Left)",
                "+Y (Top)",   "-Y (Bottom)",
                "+Z (Front)", "-Z (Back)"
            };

            ImGui::Text("Cube Map Face");
            if (ImGui::BeginCombo("Face", faceNames[selectedFace])) {
                for (int i = 0; i < 6; i++) {
                    bool isSelected = (selectedFace == i);
                    if (ImGui::Selectable(faceNames[i], isSelected)) {
                        selectedFace = i;
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }

            // 선택된 면에 해당하는 SRV 생성 및 표시
            FShadowResource* ShadowResouce = PointLight->GetShadowResource();
            if (ShadowResouce->GetAtlasSlotIndex() != -1)
            {
                ID3D11ShaderResourceView* CubeFaceSRV = ShadowResouce->GetCubeAtlasSRVFace(GEngine->GetEngine()->graphicDevice.Device,
                    ShadowResouce->GetAtlasSlotIndex(), selectedFace);

                if (CubeFaceSRV)
                {

                    ImTextureID LightDepth = reinterpret_cast<ImTextureID>(CubeFaceSRV);
                    ImVec2 imageSize(128, 128); // 필요에 따라 크기 조정
                    ImGui::Text("Shadow Map");
                    ImGui::Image(LightDepth, imageSize);
                }
            }
        }

        if (PickedComponent->IsA<USpotLightComponent>())
        {
            USpotLightComponent* SpotLight = Cast<USpotLightComponent>(PickedComponent);
            if (SpotLight)
            {
                float OuterAngle = JungleMath::RadToDeg(SpotLight->GetOuterConeAngle());
                float InnerAngle = JungleMath::RadToDeg(SpotLight->GetInnerConeAngle());

                // 먼저 Outer 처리
                if (ImGui::SliderFloat("Outer Angle", &OuterAngle, 0.0f, 89.9f))
                {
                    SpotLight->SetOuterConeAngle(OuterAngle);

                    // Outer를 줄였으면 Inner도 맞춰줌
                    InnerAngle = FMath::Min(InnerAngle, OuterAngle);
                    SpotLight->SetInnerConeAngle(InnerAngle);
                }

                // Inner는 항상 Outer보다 작게 clamp
                if (ImGui::SliderFloat("Inner Angle", &InnerAngle, 0.0f, OuterAngle))
                {
                    InnerAngle = FMath::Clamp(InnerAngle, 0.0f, OuterAngle);
                    SpotLight->SetInnerConeAngle(InnerAngle);
                }
            }
            

            FShadowMapAtlas* ShadowMapAtlas = SpotLight->GetShadowResource()->GetParentAtlas();
            if (ShadowMapAtlas)
            {
                ImTextureID LightDepth = reinterpret_cast<ImTextureID>(ShadowMapAtlas->GetSRV2D());
                FVector4 UV = SpotLight->GetLightAtlasUV(); // x,y,width,height

                ImVec2 uv0 = ImVec2(UV.x, UV.y);
                ImVec2 uv1 = ImVec2(UV.x + UV.z, UV.y + UV.w);

                ImGui::Text("Shadow Map");
                ImGui::Image(LightDepth, imageSize, uv0, uv1);
            }
            
            bool override = Cast<USpotLightComponent>(GEngine->GetLevelEditor()->GetActiveViewportClient()->GetOverrideComponent());
            if (ImGui::Checkbox("Override Camera", &override))
            {
                if (override)
                {
                    GEngine->GetLevelEditor()->GetActiveViewportClient()->SetOverrideComponent(SpotLight);
                }
                else
                {
                    GEngine->GetLevelEditor()->GetActiveViewportClient()->SetOverrideComponent(nullptr);
                }
            }
        }
    }

    // TODO: 추후에 RTTI를 이용해서 프로퍼티 출력하기
    if (PickedActor && PickedComponent && PickedComponent->IsA<UTextComponent>())
    {
        UTextComponent* textOBj = Cast<UTextComponent>(PickedComponent);
        ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
        if (ImGui::TreeNodeEx("Text Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
        {
            if (textOBj) {
                textOBj->SetTexture(L"Assets/Texture/font.png");
                textOBj->SetRowColumnCount(106, 106);
                FWString wText = textOBj->GetText();
                int len = WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, nullptr, 0, nullptr, nullptr);
                std::string u8Text(len, '\0');
                WideCharToMultiByte(CP_UTF8, 0, wText.c_str(), -1, u8Text.data(), len, nullptr, nullptr);

                static char buf[256];
                strcpy_s(buf, u8Text.c_str());

                ImGui::Text("Text: ", buf);
                ImGui::SameLine();
                ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
                if (ImGui::InputText("##Text", buf, 256, ImGuiInputTextFlags_EnterReturnsTrue))
                {
                    textOBj->ClearText();
                    int wlen = MultiByteToWideChar(CP_UTF8, 0, buf, -1, nullptr, 0);
                    FWString newWText(wlen, L'\0');
                    MultiByteToWideChar(CP_UTF8, 0, buf, -1, newWText.data(), wlen);
                    textOBj->SetText(newWText);
                }
                ImGui::PopItemFlag();
            }
            ImGui::TreePop();
        }
        ImGui::PopStyleColor();
    }

    // TODO: 추후에 RTTI를 이용해서 프로퍼티 출력하기
    if (PickedActor)
        if (UStaticMeshComponent* StaticMeshComponent = PickedActor->GetComponentByClass<UStaticMeshComponent>())
        {
            RenderForStaticMesh(StaticMeshComponent);
            RenderForMaterial(StaticMeshComponent);
        }

    if (PickedActor && PickedComponent && PickedComponent->IsA<UBillboardComponent>())
    {
        static const char* CurrentBillboardName = "Pawn";
        if (ImGui::TreeNodeEx("BillBoard", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            if (ImGui::BeginCombo("##", CurrentBillboardName, ImGuiComboFlags_None))
            {
                if (ImGui::Selectable("Pawn", strcmp(CurrentBillboardName, "Pawn") == 0))
                {
                    CurrentBillboardName = "Pawn";
                    Cast<UBillboardComponent>(PickedComponent)->SetTexture(L"Assets/Texture/Pawn_64x.png");
                }
                if (ImGui::Selectable("PointLight", strcmp(CurrentBillboardName, "PointLight") == 0))
                {
                    CurrentBillboardName = "PointLight";
                    Cast<UBillboardComponent>(PickedComponent)->SetTexture(L"Assets/Texture/PointLight_64x.png");
                }
                if (ImGui::Selectable("SpotLight", strcmp(CurrentBillboardName, "SpotLight") == 0))
                {
                    CurrentBillboardName = "SpotLight";
                    Cast<UBillboardComponent>(PickedComponent)->SetTexture(L"Assets/Texture/SpotLight_64x.png");
                }

                ImGui::EndCombo();
            }
            ImGui::TreePop();
        }

    }
    if (PickedActor && PickedComponent && PickedComponent->IsA<UHeightFogComponent>())
    {
        UHeightFogComponent* HeightFogComp = Cast<UHeightFogComponent>(PickedComponent);

        // Height Fog 속성 편집
        if (ImGui::TreeNodeEx("Height Fog Properties", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            // 기본 속성
            ImGui::Text("Basic Properties");
            ImGui::Separator();

            float FogDensity = HeightFogComp->GetFogDensity();
            if (ImGui::SliderFloat("Fog Density", &FogDensity, 0.0f, 1.0f, "%.4f"))
            {
                HeightFogComp->SetFogDensity(FogDensity);
            }

            float FogMaxOpacity = HeightFogComp->GetMaxOpacity();
            if (ImGui::SliderFloat("Fog Max Opacity", &FogMaxOpacity, 0.0f, 1.0f, "%.4f"))
            {
                HeightFogComp->SetMaxOpacity(FogMaxOpacity);
            }

            float FogEnd = HeightFogComp->GetFogEnd();
            if (ImGui::SliderFloat("Fog End", &FogEnd, 0.0f, 1000.0f, "%.2f"))
            {
                HeightFogComp->SetFogEnd(FogEnd);

                // FogStart가 제한 범위 벗어났다면 자동 보정
                float maxFogStart = FogEnd / 3.0f;
                if (HeightFogComp->GetFogStart() > maxFogStart)
                {
                    HeightFogComp->SetFogStart(maxFogStart);
                }
            }

            // FogStart 제한: 최대값은 항상 FogEnd / 3
            float FogStart = HeightFogComp->GetFogStart();
            float maxFogStart = HeightFogComp->GetFogEnd() / 3.0f;
            if (ImGui::SliderFloat("Fog Start", &FogStart, 0.0f, maxFogStart, "%.2f"))
            {
                HeightFogComp->SetFogStart(FogStart);
            }

            float FogBaseHeight = HeightFogComp->GetFogBaseHeight();
            if (ImGui::SliderFloat("Fog Base Height", &FogBaseHeight, HeightFogComp->GetFogZPosition(), 100.0f, "%.2f"))
            {
                HeightFogComp->SetFogBaseHeight(FogBaseHeight);
            }

            float FogHeightFallOff = HeightFogComp->GetHeightFallOff();
            if (ImGui::SliderFloat("Fog Height Fall Off", &FogHeightFallOff, 0.0f, 0.1f, "%.3f"))
            {
                HeightFogComp->SetHeightFallOff(FogHeightFallOff);
            }

            ImGui::Spacing();
            ImGui::Text("Color Properties");
            ImGui::Separator();

            // 안개 색상 편집
            float FogColor[3] = {
                HeightFogComp->GetFogColor().X,
                HeightFogComp->GetFogColor().Y,
                HeightFogComp->GetFogColor().Z,
            };

            if (ImGui::ColorEdit3("Fog Color", FogColor))
            {
                HeightFogComp->SetFogColor(FVector(FogColor[0], FogColor[1], FogColor[2]));
            }
            ImGui::TreePop();
        }

    }

    if (PickedActor && PickedComponent && PickedComponent->IsA<UProjectileMovementComponent>())
    {
        UProjectileMovementComponent* ProjectileComp = Cast<UProjectileMovementComponent>(PickedComponent);

        if (ImGui::TreeNodeEx("Projectile", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {

            ImGui::DragFloat("Initial Speed", &ProjectileComp->InitialSpeed, 1.0f, 0.0f, 1000000.0f);
            ImGui::DragFloat("Max Speed", &ProjectileComp->MaxSpeed, 1.0f, 0.0f, 1000000.0f);
            ImGui::DragFloat("Gravity Scale", &ProjectileComp->ProjectileGravityScale, 0.01f, 0.0f, 2.0f);

            FVector Velocity = ProjectileComp->Velocity;

            if (FImGuiWidget::DrawVec3Control("Velocity", Velocity, 0, 85))
            {
                ProjectileComp->Velocity = Velocity;
            }

            ImGui::TreePop();
        }
    }

    if (PickedActor && PickedComponent && PickedComponent->IsA<URotatingMovementComponent>())
    {
        URotatingMovementComponent* RotatingComp = Cast<URotatingMovementComponent>(PickedComponent);

        if (ImGui::TreeNodeEx("RotatingMovement", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            FVector RotationRate = RotatingComp->RotationRate;

            if (FImGuiWidget::DrawVec3Control("RotationRate", RotationRate, 0, 85))
            {
                RotatingComp->RotationRate = RotationRate;
            }

            ImGui::TreePop();
        }
    }

    RenderShapeProperty(PickedActor);

    ImGui::End();


}

void PropertyEditorPanel::DrawSceneComponentTree(USceneComponent* Component, UActorComponent*& PickedComponent)
{
    if (!Component) return;

    FString Label = *Component->GetName();
    bool bSelected = (PickedComponent == Component);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (bSelected)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    // 노드를 클릭 가능한 셀렉션으로 표시
    bool bOpened = ImGui::TreeNodeEx(*Label, nodeFlags);

    // 클릭되었을 때 선택 갱신
    if (ImGui::IsItemClicked())
    {
        PickedComponent = Component;
    }

    // 자식 재귀 호출
    if (bOpened)
    {
        for (USceneComponent* Child : Component->GetAttachChildren())
        {
            DrawSceneComponentTree(Child, PickedComponent);
        }
        ImGui::TreePop();
    }
}

void PropertyEditorPanel::DrawActorComponent(UActorComponent* Component, UActorComponent*& PickedComponent)
{
    if (!Component) return;

    FString Label = *Component->GetName();
    bool bSelected = (PickedComponent == Component);

    ImGuiTreeNodeFlags nodeFlags = ImGuiTreeNodeFlags_OpenOnArrow;
    if (bSelected)
        nodeFlags |= ImGuiTreeNodeFlags_Selected;

    if (ImGui::Selectable(*Label, nodeFlags))
    {
        PickedComponent = Component;
    }
}

void PropertyEditorPanel::RGBToHSV(float r, float g, float b, float& h, float& s, float& v) const
{
    float mx = FMath::Max(r, FMath::Max(g, b));
    float mn = FMath::Min(r, FMath::Min(g, b));
    float delta = mx - mn;

    v = mx;

    if (mx == 0.0f) {
        s = 0.0f;
        h = 0.0f;
        return;
    }
    else {
        s = delta / mx;
    }

    if (delta < 1e-6) {
        h = 0.0f;
    }
    else {
        if (r >= mx) {
            h = (g - b) / delta;
        }
        else if (g >= mx) {
            h = 2.0f + (b - r) / delta;
        }
        else {
            h = 4.0f + (r - g) / delta;
        }
        h *= 60.0f;
        if (h < 0.0f) {
            h += 360.0f;
        }
    }
}

void PropertyEditorPanel::HSVToRGB(float h, float s, float v, float& r, float& g, float& b) const
{
    // h: 0~360, s:0~1, v:0~1
    float c = v * s;
    float hp = h / 60.0f;             // 0~6 구간
    float x = c * (1.0f - fabsf(fmodf(hp, 2.0f) - 1.0f));
    float m = v - c;

    if (hp < 1.0f) { r = c;  g = x;  b = 0.0f; }
    else if (hp < 2.0f) { r = x;  g = c;  b = 0.0f; }
    else if (hp < 3.0f) { r = 0.0f; g = c;  b = x; }
    else if (hp < 4.0f) { r = 0.0f; g = x;  b = c; }
    else if (hp < 5.0f) { r = x;  g = 0.0f; b = c; }
    else { r = c;  g = 0.0f; b = x; }

    r += m;  g += m;  b += m;
}

void PropertyEditorPanel::RenderForStaticMesh(UStaticMeshComponent* StaticMeshComp)
{
    if (StaticMeshComp->GetStaticMesh() == nullptr)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Static Mesh", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        ImGui::Text("StaticMesh");
        ImGui::SameLine();

        FString PreviewName = StaticMeshComp->GetStaticMesh()->GetRenderData()->DisplayName;
        const TMap<FWString, UStaticMesh*> Meshes = FManagerOBJ::GetStaticMeshes();
        if (ImGui::BeginCombo("##StaticMesh", GetData(PreviewName), ImGuiComboFlags_None))
        {
            for (const auto Mesh : Meshes)
            {
                if (ImGui::Selectable(GetData(Mesh.Value->GetRenderData()->DisplayName), false))
                {
                    StaticMeshComp->SetStaticMesh(Mesh.Value);
                }
            }

            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }
    ImGui::PopStyleColor();
}


void PropertyEditorPanel::RenderForMaterial(UStaticMeshComponent* StaticMeshComp)
{
    if (StaticMeshComp->GetStaticMesh() == nullptr)
    {
        return;
    }

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    if (ImGui::TreeNodeEx("Materials", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        for (uint32 i = 0; i < StaticMeshComp->GetNumMaterials(); ++i)
        {
            if (ImGui::Selectable(GetData(StaticMeshComp->GetMaterialSlotNames()[i].ToString()), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    std::cout << GetData(StaticMeshComp->GetMaterialSlotNames()[i].ToString()) << std::endl;
                    SelectedMaterialIndex = i;
                    SelectedStaticMeshComp = StaticMeshComp;
                }
            }
        }

        if (ImGui::Button("    +    ")) {
            IsCreateMaterial = true;
        }

        ImGui::TreePop();
    }

    if (ImGui::TreeNodeEx("SubMeshes", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen)) // 트리 노드 생성
    {
        auto subsets = StaticMeshComp->GetStaticMesh()->GetRenderData()->MaterialSubsets;
        for (uint32 i = 0; i < subsets.Num(); ++i)
        {
            std::string temp = "subset " + std::to_string(i);
            if (ImGui::Selectable(temp.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
            {
                if (ImGui::IsMouseDoubleClicked(0))
                {
                    StaticMeshComp->SetselectedSubMeshIndex(i);
                    SelectedStaticMeshComp = StaticMeshComp;
                }
            }
        }
        std::string temp = "clear subset";
        if (ImGui::Selectable(temp.c_str(), false, ImGuiSelectableFlags_AllowDoubleClick))
        {
            if (ImGui::IsMouseDoubleClicked(0))
                StaticMeshComp->SetselectedSubMeshIndex(-1);
        }

        ImGui::TreePop();
    }

    ImGui::PopStyleColor();

    if (SelectedMaterialIndex != -1)
    {
        RenderMaterialView(SelectedStaticMeshComp->GetMaterial(SelectedMaterialIndex));
    }
    if (IsCreateMaterial) {
        RenderCreateMaterialView();
    }
}

void PropertyEditorPanel::RenderMaterialView(UMaterial* Material)
{
    ImGui::SetNextWindowSize(ImVec2(380, 400), ImGuiCond_Once);
    ImGui::Begin("Material Viewer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

    static ImGuiSelectableFlags BaseFlag = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None | ImGuiColorEditFlags_NoAlpha;

    FVector MatDiffuseColor = Material->GetMaterialInfo().Diffuse;
    FVector MatSpecularColor = Material->GetMaterialInfo().Specular;
    FVector MatAmbientColor = Material->GetMaterialInfo().Ambient;
    FVector MatEmissiveColor = Material->GetMaterialInfo().Emissive;

    float dr = MatDiffuseColor.X;
    float dg = MatDiffuseColor.Y;
    float db = MatDiffuseColor.Z;
    float da = 1.0f;
    float DiffuseColorPick[4] = { dr, dg, db, da };

    ImGui::Text("Material Name |");
    ImGui::SameLine();
    ImGui::Text(*Material->GetMaterialInfo().MTLName);
    ImGui::Separator();

    ImGui::Text("  Diffuse Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Diffuse##Color", (float*)&DiffuseColorPick, BaseFlag))
    {
        FVector NewColor = { DiffuseColorPick[0], DiffuseColorPick[1], DiffuseColorPick[2] };
        Material->SetDiffuse(NewColor);
    }

    float sr = MatSpecularColor.X;
    float sg = MatSpecularColor.Y;
    float sb = MatSpecularColor.Z;
    float sa = 1.0f;
    float SpecularColorPick[4] = { sr, sg, sb, sa };

    ImGui::Text("Specular Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Specular##Color", (float*)&SpecularColorPick, BaseFlag))
    {
        FVector NewColor = { SpecularColorPick[0], SpecularColorPick[1], SpecularColorPick[2] };
        Material->SetSpecular(NewColor);
    }


    float ar = MatAmbientColor.X;
    float ag = MatAmbientColor.Y;
    float ab = MatAmbientColor.Z;
    float aa = 1.0f;
    float AmbientColorPick[4] = { ar, ag, ab, aa };

    ImGui::Text("Ambient Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Ambient##Color", (float*)&AmbientColorPick, BaseFlag))
    {
        FVector NewColor = { AmbientColorPick[0], AmbientColorPick[1], AmbientColorPick[2] };
        Material->SetAmbient(NewColor);
    }


    float er = MatEmissiveColor.X;
    float eg = MatEmissiveColor.Y;
    float eb = MatEmissiveColor.Z;
    float ea = 1.0f;
    float EmissiveColorPick[4] = { er, eg, eb, ea };

    ImGui::Text("Emissive Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Emissive##Color", (float*)&EmissiveColorPick, BaseFlag))
    {
        FVector NewColor = { EmissiveColorPick[0], EmissiveColorPick[1], EmissiveColorPick[2] };
        Material->SetEmissive(NewColor);
    }

    ImGui::Spacing();
    ImGui::Separator();

    ImGui::Text("Choose Material");
    ImGui::Spacing();

    ImGui::Text("Material Slot Name |");
    ImGui::SameLine();
    ImGui::Text(GetData(SelectedStaticMeshComp->GetMaterialSlotNames()[SelectedMaterialIndex].ToString()));

    ImGui::Text("Override Material |");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160);
    // 메테리얼 이름 목록을 const char* 배열로 변환
    std::vector<const char*> materialChars;
    for (const auto& material : FManagerOBJ::GetMaterials()) {
        materialChars.push_back(*material.Value->GetMaterialInfo().MTLName);
    }

    //// 드롭다운 표시 (currentMaterialIndex가 범위를 벗어나지 않도록 확인)
    //if (currentMaterialIndex >= FManagerOBJ::GetMaterialNum())
    //    currentMaterialIndex = 0;

    if (ImGui::Combo("##MaterialDropdown", &CurMaterialIndex, materialChars.data(), FManagerOBJ::GetMaterialNum())) {
        UMaterial* material = FManagerOBJ::GetMaterial(materialChars[CurMaterialIndex]);
        SelectedStaticMeshComp->SetMaterial(SelectedMaterialIndex, material);
    }

    if (ImGui::Button("Close"))
    {
        SelectedMaterialIndex = -1;
        SelectedStaticMeshComp = nullptr;
    }

    ImGui::End();
}

void PropertyEditorPanel::RenderCreateMaterialView()
{
    ImGui::SetNextWindowSize(ImVec2(300, 500), ImGuiCond_Once);
    ImGui::Begin("Create Material Viewer", nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoNav);

    static ImGuiSelectableFlags BaseFlag = ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel | ImGuiColorEditFlags_None | ImGuiColorEditFlags_NoAlpha;

    ImGui::Text("New Name");
    ImGui::SameLine();
    static char materialName[256] = "New Material";
    // 기본 텍스트 입력 필드
    ImGui::SetNextItemWidth(128);
    if (ImGui::InputText("##NewName", materialName, IM_ARRAYSIZE(materialName))) {
        tempMaterialInfo.MTLName = materialName;
    }

    FVector MatDiffuseColor = tempMaterialInfo.Diffuse;
    FVector MatSpecularColor = tempMaterialInfo.Specular;
    FVector MatAmbientColor = tempMaterialInfo.Ambient;
    FVector MatEmissiveColor = tempMaterialInfo.Emissive;

    float dr = MatDiffuseColor.X;
    float dg = MatDiffuseColor.Y;
    float db = MatDiffuseColor.Z;
    float da = 1.0f;
    float DiffuseColorPick[4] = { dr, dg, db, da };

    ImGui::Text("Set Property");
    ImGui::Indent();

    ImGui::Text("  Diffuse Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Diffuse##Color", (float*)&DiffuseColorPick, BaseFlag))
    {
        FVector NewColor = { DiffuseColorPick[0], DiffuseColorPick[1], DiffuseColorPick[2] };
        tempMaterialInfo.Diffuse = NewColor;
    }

    float sr = MatSpecularColor.X;
    float sg = MatSpecularColor.Y;
    float sb = MatSpecularColor.Z;
    float sa = 1.0f;
    float SpecularColorPick[4] = { sr, sg, sb, sa };

    ImGui::Text("Specular Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Specular##Color", (float*)&SpecularColorPick, BaseFlag))
    {
        FVector NewColor = { SpecularColorPick[0], SpecularColorPick[1], SpecularColorPick[2] };
        tempMaterialInfo.Specular = NewColor;
    }


    float ar = MatAmbientColor.X;
    float ag = MatAmbientColor.Y;
    float ab = MatAmbientColor.Z;
    float aa = 1.0f;
    float AmbientColorPick[4] = { ar, ag, ab, aa };

    ImGui::Text("Ambient Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Ambient##Color", (float*)&AmbientColorPick, BaseFlag))
    {
        FVector NewColor = { AmbientColorPick[0], AmbientColorPick[1], AmbientColorPick[2] };
        tempMaterialInfo.Ambient = NewColor;
    }


    float er = MatEmissiveColor.X;
    float eg = MatEmissiveColor.Y;
    float eb = MatEmissiveColor.Z;
    float ea = 1.0f;
    float EmissiveColorPick[4] = { er, eg, eb, ea };

    ImGui::Text("Emissive Color");
    ImGui::SameLine();
    if (ImGui::ColorEdit4("Emissive##Color", (float*)&EmissiveColorPick, BaseFlag))
    {
        FVector NewColor = { EmissiveColorPick[0], EmissiveColorPick[1], EmissiveColorPick[2] };
        tempMaterialInfo.Emissive = NewColor;
    }
    ImGui::Unindent();

    ImGui::NewLine();
    if (ImGui::Button("Create Material")) {
        FManagerOBJ::CreateMaterial(tempMaterialInfo);
    }

    ImGui::NewLine();
    if (ImGui::Button("Close"))
    {
        IsCreateMaterial = false;
    }

    ImGui::End();
}

void PropertyEditorPanel::RenderForLua(ULuaComponent* LuaComponent)
{
     ULuaComponent* LuaComp = Cast<ULuaComponent>(PickedComponent); // Lua 컴포넌트로 캐스팅

    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.1f, 0.15f, 0.1f, 1.0f)); // Lua 컴포넌트용 색상 (예시)
    if (ImGui::TreeNodeEx("Lua Component", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
    {
        // --- 스크립트 경로 표시 및 찾아보기 ---
        char scriptPathBuffer[1024]; // ImGui InputText용 버퍼
        // LuaComp->LuaScriptPath가 std::string 이라고 가정
        errno_t err = strncpy_s(
            scriptPathBuffer,                 // 1. 대상 버퍼 포인터
            sizeof(scriptPathBuffer),         // 2. 대상 버퍼의 *전체 크기* (null 문자 포함)
            *LuaComp->LuaScriptPath,   // 3. 원본 문자열 포인터
            _TRUNCATE                         // 4. 복사할 최대 문자 수 (또는 _TRUNCATE)
                                              //    _TRUNCATE는 버퍼 크기에 맞게 복사하고 null 종료, 넘치면 잘라냄
        );

        if (err != 0) {
            // 오류 처리 (예: 버퍼가 너무 작거나 다른 문제 발생 시)
            std::cerr << "strncpy_s failed with error code: " << err << std::endl;
            scriptPathBuffer[0] = '\0'; // 안전하게 빈 문자열로 만듦
        }
        ImGui::Text("Script Path:");
        ImGui::SameLine();
        // 경로를 InputText로 표시 (읽기 전용, 복사 가능)
        ImGui::PushItemWidth(-FLT_MIN); // 너비 최대로
        ImGui::InputText("##LuaScriptPath", scriptPathBuffer, sizeof(scriptPathBuffer), ImGuiInputTextFlags_ReadOnly);
        ImGui::PopItemWidth();

        if (ImGui::Button("Browse..."))
        {
            // tinyfd를 사용하여 Lua 스크립트 파일 열기 대화상자 표시
            char const* lFilterPatterns[1] = { "*.lua" };
            const char* selectedFilePath = tinyfd_openFileDialog(
                "Select Lua Script",                      // 대화상자 제목
                *FLuaManager::Get().GetScriptsBasePath(), // 기본 경로 (스크립트 폴더)
                1,                                       // 필터 개수
                lFilterPatterns,                         // 필터 패턴 (".lua")
                "Lua Script (*.lua)",                    // 필터 설명
                0                                        // 다중 선택 비활성화
            );
            
             if (selectedFilePath != nullptr) // 사용자가 파일을 선택했다면
            {
                // 선택된 전체 경로
                std::string selectedFullPath = selectedFilePath;

                // (선택 사항) 스크립트 기본 경로 기준 상대 경로로 변환
                std::string relativePathStr = selectedFullPath; // 기본값은 전체 경로
                try {
                    std::filesystem::path fullPath(selectedFullPath);
                    std::filesystem::path absoluteBasePath = std::filesystem::absolute(FLuaManager::Get().GetScriptsBasePath());

                    std::filesystem::path relativePath = std::filesystem::relative(fullPath, absoluteBasePath);
                    
                    if (!relativePath.empty() && relativePath.native().find(L"..") != 0) { // L".." 은 Windows wchar_t 기준
                        // 성공적으로 상대 경로를 얻었고, 상위 경로가 아님
                        // generic_string()을 사용하여 플랫폼 독립적인 '/' 구분자로 변환
                        relativePathStr = relativePath.generic_string();
                        std::cout << "Calculated Relative Path: " << relativePathStr << std::endl;
                    } else {
                        // 상대 경로가 비었거나 ".." 로 시작하는 경우 (즉, basePath 외부에 있음)
                        std::cerr << "Warning: Selected script is outside the project's script base path hierarchy. Using absolute path." << std::endl;
                        // relativePathStr는 이미 selectedFullPath (절대 경로)로 초기화되어 있음
                        // 필요하다면 여기서 fullPath.generic_string() 등을 사용해 경로 형식을 통일할 수 있습니다.
                        relativePathStr = fullPath.generic_string(); // 일관성을 위해 '/' 사용 절대 경로
                    }
                } catch (const std::exception& e) {
                     std::cerr << "Error calculating relative path: " << e.what() << ". Using full path." << std::endl;
                    relativePathStr = std::filesystem::path(selectedFullPath).generic_string();
                }


                LuaComp->LuaScriptPath = relativePathStr; // 선택된 경로(상대 또는 전체)로 업데이트

                // 중요: 스크립트 경로가 변경되었으므로 컴포넌트를 다시 초기화하거나
                // 스크립트를 다시 로드하는 로직 호출 필요
                // 예: LuaComp->ReloadScript(); 또는 LuaComp->InitializeComponent();
                std::cout << "LuaComponent: Script path changed. Need to reload script: " << LuaComp->LuaScriptPath << std::endl; // 임시 로그
            }
            // 사용자가 취소하면 selectedFilePath는 nullptr이므로 아무 작업도 하지 않음
        }

        ImGui::Separator(); // 구분선

        // --- 새 스크립트 생성 ---
        if (ImGui::Button("Create New Script..."))
        {
            ImGui::OpenPopup("Create Lua Script");
        }

        // "Create Lua Script" 팝업 정의
        if (ImGui::BeginPopupModal("Create Lua Script", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            static char newScriptNameBuffer[128] = "NewActorScript";
            ImGui::InputText("Script Name (.lua)", newScriptNameBuffer, sizeof(newScriptNameBuffer));
            ImGui::Separator();

            if (ImGui::Button("Create", ImVec2(120, 0)))
            {
                std::string filename = newScriptNameBuffer;
                if (!filename.empty())
                {
                    // '.lua' 확장자 추가 (이미 있다면 중복 방지)
                    if (filename.rfind(".lua") == std::string::npos) {
                        filename += ".lua";
                    }

                    // 스크립트 저장 경로 조합
                    FString scriptsBasePath = FLuaManager::Get().GetScriptsBasePath();
                    FString templatePath = FLuaManager::Get().GetTemplateLuaPath();
                    FString ScriptsReleativePath = scriptsBasePath + filename;

                    // 파일이 이미 존재하는지 확인 (선택 사항)
                    if (std::filesystem::exists(ScriptsReleativePath)) {
                         tinyfd_messageBox("Error", "A script with this name already exists!", "ok", "error", 1);
                    } else {
                        // 1. 템플릿 파일 복사
                        bool copied = false;
                        try {
                            std::ifstream src(*templatePath, std::ios::binary);
                            std::ofstream dst(*ScriptsReleativePath, std::ios::binary);
                            if (src && dst) {
                               dst << src.rdbuf();
                               copied = true;
                            } else {
                                if (!src) std::cerr << "Error: Template file not found or couldn't be opened: " << templatePath << std::endl;
                                if (!dst) std::cerr << "Error: Could not create destination file: " << ScriptsReleativePath << std::endl;
                            }
                        } catch (const std::exception& e) {
                             std::cerr << "Error copying template: " << e.what() << std::endl;
                        }
                        
                        if (copied)
                        {
                            std::cout << "Created new script: " << ScriptsReleativePath << std::endl;
                            // 2. 컴포넌트의 경로 업데이트 (상대 경로로 저장)
                            LuaComp->LuaScriptPath = filename;
                            std::filesystem::path basePath(FLuaManager::Get().GetScriptsBasePath());
                            std::filesystem::path absolutePath = std::filesystem::absolute(basePath / ScriptsReleativePath);
                            std::string fullPath = absolutePath.string();

                            // 3. 생성된 스크립트 편집기로 열기 (OpenExternalEditor 함수 사용)
                            ShellExecuteA(NULL, "open", fullPath.c_str(), NULL, NULL, SW_SHOWNORMAL);
                            
                            // 4. 중요: 컴포넌트 다시 초기화/스크립트 로드
                            // 예: LuaComp->ReloadScript(); 또는 LuaComp->InitializeComponent();
                            std::cout << "LuaComponent: New script created. Need to reload script: " << LuaComp->LuaScriptPath << std::endl; // 임시 로그

                            ImGui::CloseCurrentPopup();
                        }
                        else
                        {
                            tinyfd_messageBox("Error", "Failed to create the script file. Check permissions or template file path.", "ok", "error", 1);
                            std::cerr << "Failed to copy template Lua file from " << templatePath << " to " << ScriptsReleativePath << std::endl;
                        }
                    }
                } else {
                    tinyfd_messageBox("Warning", "Please enter a name for the script.", "ok", "warning", 1);
                }
            }
            ImGui::SetItemDefaultFocus();
            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(120, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }

        ImGui::SameLine(); // 생성 버튼 옆에 편집 버튼 배치

        // --- 스크립트 편집 ---
        // 스크립트 경로가 있을 때만 활성화
        bool hasScript = !LuaComp->LuaScriptPath.IsEmpty();
        ImGui::BeginDisabled(!hasScript); // 경로 없으면 비활성화 시작

        if (ImGui::Button("Edit Script"))
        {
            // 스크립트 전체 경로 계산 (상대 경로일 수 있으므로 기본 경로와 조합)
            FString fullPath;
            try {
                // LuaScriptPath가 절대 경로인지 간단히 확인 (더 견고한 방법 필요할 수 있음)
                std::filesystem::path scriptPathObj(LuaComp->LuaScriptPath);
                if (scriptPathObj.is_absolute()) {
                    fullPath = LuaComp->LuaScriptPath;
                } else {
                    // 상대 경로라면 기준 경로와 조합하여 절대 경로 생성
                    std::filesystem::path basePath(FLuaManager::Get().GetScriptsBasePath());
                    std::filesystem::path absolutePath = std::filesystem::absolute(basePath / scriptPathObj);
                    fullPath = absolutePath.string();
                }
            } catch(const std::exception& e) {
                 // 경로 오류 발생 시 fallback
                 std::cerr << "Path error for Edit Script: " << e.what() << std::endl;
                 fullPath = FLuaManager::Get().GetScriptsBasePath() + LuaComp->LuaScriptPath; // 기본 조합 시도
            }


            if (std::filesystem::exists(fullPath)) {
                ShellExecuteA(NULL, "open", *fullPath, NULL, NULL, SW_SHOWNORMAL);
            } else {
                 tinyfd_messageBox("Error", "Script file not found at the specified path.", "ok", "error", 1);
                 std::cerr << "Edit Script Error: File not found at " << fullPath << std::endl;
            }
        }

        ImGui::EndDisabled(); // 비활성화 종료

        // (선택 사항) 스크립트 로드 상태 표시
        // std::string status = LuaComp->GetScriptStatus(); // 컴포넌트가 상태 문자열 반환 가정
        // ImGui::Text("Status: %s", status.c_str());

        ImGui::TreePop(); // 트리 닫기
    }
    ImGui::PopStyleColor(); // 스타일 복원
}

void PropertyEditorPanel::RenderShapeProperty(AActor* PickedActor)
{
    if (PickedActor && PickedComponent && PickedComponent->IsA<UBoxShapeComponent>())
    {
        UBoxShapeComponent* ShapeComp = Cast<UBoxShapeComponent>(PickedComponent);

        if (ImGui::TreeNodeEx("BoxShapeComponent", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            FVector BoxExtent = ShapeComp->GetBoxExtent();

            if (FImGuiWidget::DrawVec3Control("BoxExtent", BoxExtent, 0, 10))
            {
                ShapeComp->SetBoxExtent(BoxExtent);
            }

            ImGui::TreePop();
        }
    }

    if (PickedActor && PickedComponent && PickedComponent->IsA<USphereShapeComponent>())
    {
        USphereShapeComponent* ShapeComp = Cast<USphereShapeComponent>(PickedComponent);

        if (ImGui::TreeNodeEx("SphereShapeComponent", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            float Radius = ShapeComp->GetRadius();

            if (ImGui::SliderFloat("Radius", &Radius, 0.0f, 100.0f))
            {
                ShapeComp->SetRadius(Radius);
            }

            ImGui::TreePop();
        }
    }

    if (PickedActor && PickedComponent && PickedComponent->IsA<UCapsuleShapeComponent>())
    {
        UCapsuleShapeComponent* ShapeComp = Cast<UCapsuleShapeComponent>(PickedComponent);

        if (ImGui::TreeNodeEx("CapsuleShapeComponent", ImGuiTreeNodeFlags_Framed | ImGuiTreeNodeFlags_DefaultOpen))
        {
            float CapsuleRaidus = ShapeComp->GetRadius();

            if (ImGui::SliderFloat("CapsuleRaidus", &CapsuleRaidus, 0.0f, 100.0f))
            {
                ShapeComp->SetRadius(CapsuleRaidus);
            }

            float CapsuleHalfHeight = ShapeComp->GetHalfHeight();

            if (ImGui::SliderFloat("CapsuleHalfHeight", &CapsuleHalfHeight, 0.0f, 100.0f))
            {
                ShapeComp->SetHalfHeight(CapsuleHalfHeight);
            }

            ImGui::TreePop();
        }
    }

}

void PropertyEditorPanel::RenderDelegate(ULevel* level)
{
    static AActor* SelectedActor = nullptr;
    FString SelectedActorName;
    SelectedActor ? SelectedActorName = SelectedActor->GetName() : SelectedActorName = "";
    
    if (ImGui::BeginCombo("Delegate Object", GetData(SelectedActorName), ImGuiComboFlags_None))
    {
        for (const auto& Actor : level->GetActors())
        {
            if (ImGui::Selectable(GetData(Actor->GetName()), false))
            {
                SelectedActor = Actor;
            }
        }
        ImGui::EndCombo();
    }
    
    static FString SelectedFunctionName = "";
    if (SelectedActor)
    {
        if (ImGui::BeginCombo("Delegate Function", GetData(SelectedFunctionName), ImGuiComboFlags_None))
        {
            for (const auto& function : SelectedActor->FunctionRegistry()->GetRegisteredFunctions())
            {
                if (ImGui::Selectable(GetData(function.Key.ToString()), false))
                {
                    SelectedFunctionName = function.Key.ToString();
                }
            }
            ImGui::EndCombo();
        }
    }
}


void PropertyEditorPanel::OnResize(HWND hWnd)
{
    RECT clientRect;
    GetClientRect(hWnd, &clientRect);
    Width = clientRect.right - clientRect.left;
    Height = clientRect.bottom - clientRect.top;
}
