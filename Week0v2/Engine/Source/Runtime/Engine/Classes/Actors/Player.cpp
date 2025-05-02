#include "Player.h"

#include "UnrealClient.h"
#include "Engine/World.h"
#include "BaseGizmos/GizmoArrowComponent.h"
#include "BaseGizmos/GizmoCircleComponent.h"
#include "BaseGizmos/TransformGizmo.h"
#include "Camera/CameraComponent.h"
#include "LevelEditor/SLevelEditor.h"
#include "Math/JungleMath.h"
#include "PropertyEditor/ShowFlags.h"
#include "UnrealEd/EditorViewportClient.h"
#include "UObject/UObjectIterator.h"

#include "EditorEngine.h"
#include "Components/LightComponents/LightComponentBase.h"
#include "PropertyEditor/PrimitiveDrawEditor.h"
#include "UnrealEd/UnrealEd.h"


class PrimitiveDrawEditor;
using namespace DirectX;

AEditorPlayer::AEditorPlayer()
{
}

void AEditorPlayer::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    Input();
}
void AEditorPlayer::MultiSelectingStart()
{
    GetCursorPos(&multiSelectingStartPos);
    bMultiSeleting = true;
}

void AEditorPlayer::MultiSelectingEnd()
{
    POINT multiSelectingEndPos;
    GetCursorPos(&multiSelectingEndPos);
    
    long leftTopX = std::min(multiSelectingStartPos.x, multiSelectingEndPos.x);
    long leftTopY = std::min(multiSelectingStartPos.y, multiSelectingEndPos.y);
    long rightBottomX = std::max(multiSelectingStartPos.x, multiSelectingEndPos.x);
    long rightBottomY =  std::max(multiSelectingStartPos.y, multiSelectingEndPos.y);

    GEngine->GetWorld()->ClearSelectedActors();
    
    // // TODO : 현재 UUID 가 높은 애들은 선택이 안됩니다.
    // for (long i = leftTopX; i <= rightBottomX; i +=10)
    // {
    //     for (long j = leftTopY ; j <= rightBottomY; j +=10)
    //     {
    //         uint32 UUID = GetEngine()->graphicDevice.GetPixelUUID(POINT(i,j));
    //          for ( USceneComponent* obj : TObjectRange<USceneComponent>())
    //          {
    //              if (obj->GetUUID() != UUID) continue;
    //              UE_LOG(LogLevel::Display, *obj->GetOwner()->GetName());
    //              GEngine->GetWorld()->AddSelectedActor(obj->GetOwner());
    //          }
    //     }
    // }

    bMultiSeleting = false;
}

void AEditorPlayer::MakeMulitRect()
{
    UE_LOG(LogLevel::Error, "MakeRecting");
    POINT multiSelectingEndPos;
    GetCursorPos(&multiSelectingEndPos);
    ImVec2 topLeft(std::min(multiSelectingStartPos.x, multiSelectingEndPos.x), std::min(multiSelectingStartPos.y, multiSelectingEndPos.y));
    ImVec2 bottomRight(std::max(multiSelectingStartPos.x, multiSelectingEndPos.x),std::max(multiSelectingStartPos.y, multiSelectingEndPos.y));
    ImU32 rectColor = ImGui::GetColorU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    float thickness = 2.0f;
    // ImGui::GetForegroundDrawList()->AddRect(topLeft, bottomRight, rectColor, 0.0f, 0, thickness);
    static_cast<PrimitiveDrawEditor*>(GEngine->GetUnrealEditor()->GetEditorPanel("PrimitiveDrawEditor").get())->DrawRectInfo.Add(
        FDrawRectInfo(topLeft,bottomRight,rectColor,0.0f,0,thickness));
}

void AEditorPlayer::Input()
{
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;
    if (GetAsyncKeyState(VK_LSHIFT) & 0x8000)
    {
        if (!bLShiftDown)
            bLShiftDown = true;
    }
    else
    {
        bLShiftDown = false;
    }
    if (GetAsyncKeyState(VK_LBUTTON) & 0x8000)
    {
        if (!bLeftMouseDown)
        {
            bLeftMouseDown = true;

            POINT mousePos;
            GetCursorPos(&mousePos);
            GetCursorPos(&lastMousePos);
            if ( bLAltDown && bLCtrlDown )
            {
                MultiSelectingStart();
            }
            // uint32 UUID = GetEngine()->graphicDevice.GetPixelUUID(mousePos);
            //  for ( const auto obj : TObjectRange<USceneComponent>())
            //  {
            //      if (obj->GetUUID() != UUID) continue;
            //
            //      UE_LOG(LogLevel::Display, *obj->GetName());
            //  }
            ScreenToClient(GetEngine()->hWnd, &mousePos);

            FVector pickPosition;

            const auto& ActiveViewport = GetEngine()->GetLevelEditor()->GetActiveViewportClient();
            ScreenToViewSpace(mousePos.x, mousePos.y, ActiveViewport->GetViewMatrix(), ActiveViewport->GetProjectionMatrix(), pickPosition);
            bool res = PickGizmo(pickPosition);
            if (!res) PickActor(pickPosition);
        }
        else
        {
            if (bMultiSeleting)
            {
                MakeMulitRect();
            }
            PickedObjControl();
        }
    }
    else
    {
        if (bLeftMouseDown)
        {
            bLeftMouseDown = false;
            bAlreadyDup = false;
            if (bMultiSeleting)
            {
                MultiSelectingEnd();
            }
            else
                GetWorld()->SetPickingGizmo(nullptr);
        }
    }
    if (GetAsyncKeyState(VK_SPACE) & 0x8000)
    {
        if (!bSpaceDown)
        {
            AddControlMode();
            bSpaceDown = true;
        }
    }
    else
    {
        if (bSpaceDown)
        {
            bSpaceDown = false;
        }
    }
    if (GetAsyncKeyState(VK_RBUTTON) & 0x8000)
    {
        if (!bRightMouseDown)
        {
            bRightMouseDown = true;
        }
    }
    else
    {
        bRightMouseDown = false;
        if (GetAsyncKeyState('Q') & 0x8000)
        {
            //GetWorld()->SetPickingObj(nullptr);
        }
        if (GetAsyncKeyState('W') & 0x8000)
        {
            cMode = CM_TRANSLATION;
        }
        if (GetAsyncKeyState('E') & 0x8000)
        {
            cMode = CM_ROTATION;
        }
        if (GetAsyncKeyState('R') & 0x8000)
        {
            cMode = CM_SCALE;
        }
    }
    if ((GetAsyncKeyState(VK_LSHIFT) & 0x8000) && (GetAsyncKeyState(VK_DELETE) & 0x8000))
    {
        for (AActor* actor : GEngine->GetWorld()->GetSelectedActors())
        {
            actor->Destroy();
        }
        GEngine->GetWorld()->ClearSelectedActors();
    }
    if (GetAsyncKeyState(VK_LCONTROL) & 0x8000)
    {
        if (!bLCtrlDown)
        {
            bLCtrlDown =true;
        }
    }
    else
    {
        bLCtrlDown= false;
    }
    if (GetAsyncKeyState('D') & 0x8000)
    {
        if (!bDkeyDown)
        {
            bDkeyDown = true;
            if (bLCtrlDown)
            {
                GetEngine()->GetWorld()->DuplicateSeletedActors();
            }
        }
    }
    else
    {
        bDkeyDown = false;
    }
    if (GetAsyncKeyState(VK_LMENU) & 0x8000)
    {
        if (!bLAltDown)
        {
            bLAltDown = true;
        }
    }
    else
    {
        bLAltDown =false;
    }
    
}

bool AEditorPlayer::PickGizmo(FVector& pickPosition)
{
    bool isPickedGizmo = false;
    if (!GetWorld()->GetSelectedActors().IsEmpty())
    {
        if (cMode == CM_TRANSLATION)
        {
            for (auto iter : GetWorld()->LocalGizmo->GetArrowArr())
            {
                int maxIntersect = 0;
                float minDistance = FLT_MAX;
                float Distance = 0.0f;
                int currentIntersectCount = 0;
                if (!iter) continue;
                if (RayIntersectsObject(pickPosition, iter, Distance, currentIntersectCount))
                {
                    if (Distance < minDistance)
                    {
                        minDistance = Distance;
                        maxIntersect = currentIntersectCount;
                        GetWorld()->SetPickingGizmo(iter);
                        isPickedGizmo = true;
                    }
                    else if (abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount > maxIntersect)
                    {
                        maxIntersect = currentIntersectCount;
                        GetWorld()->SetPickingGizmo(iter);
                        isPickedGizmo = true;
                    }
                }
            }
        }
        else if (cMode == CM_ROTATION)
        {
            for (auto iter : GetWorld()->LocalGizmo->GetDiscArr())
            {
                int maxIntersect = 0;
                float minDistance = FLT_MAX;
                float Distance = 0.0f;
                int currentIntersectCount = 0;
                //UPrimitiveComponent* localGizmo = dynamic_cast<UPrimitiveComponent*>(GetWorld()->LocalGizmo[i]);
                if (!iter) continue;
                if (RayIntersectsObject(pickPosition, iter, Distance, currentIntersectCount))
                {
                    if (Distance < minDistance)
                    {
                        minDistance = Distance;
                        maxIntersect = currentIntersectCount;
                        GetWorld()->SetPickingGizmo(iter);
                        isPickedGizmo = true;
                    }
                    else if (abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount > maxIntersect)
                    {
                        maxIntersect = currentIntersectCount;
                        GetWorld()->SetPickingGizmo(iter);
                        isPickedGizmo = true;
                    }
                }
            }
        }
        else if (cMode == CM_SCALE)
        {
            for (auto iter : GetWorld()->LocalGizmo->GetScaleArr())
            {
                int maxIntersect = 0;
                float minDistance = FLT_MAX;
                float Distance = 0.0f;
                int currentIntersectCount = 0;
                if (!iter) continue;
                if (RayIntersectsObject(pickPosition, iter, Distance, currentIntersectCount))
                {
                    if (Distance < minDistance)
                    {
                        minDistance = Distance;
                        maxIntersect = currentIntersectCount;
                        GetWorld()->SetPickingGizmo(iter);
                        isPickedGizmo = true;
                    }
                    else if (abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount > maxIntersect)
                    {
                        maxIntersect = currentIntersectCount;
                        GetWorld()->SetPickingGizmo(iter);
                        isPickedGizmo = true;
                    }
                }
            }
        }
    }
    return isPickedGizmo;
}

void AEditorPlayer::PickActor(const FVector& pickPosition)
{
    if (!(ShowFlags::GetInstance().currentFlags & EEngineShowFlags::SF_Primitives)) return;
    if (!bLShiftDown)
        GetWorld()->ClearSelectedActors();
    const UActorComponent* Possible = nullptr;
    int maxIntersect = 0;
    float minDistance = FLT_MAX;
    for (const auto iter : TObjectRange<UPrimitiveComponent>())
    {
        UPrimitiveComponent* pObj;
        if (iter->IsA<UPrimitiveComponent>() || iter->IsA<ULightComponentBase>())
        {
            pObj = static_cast<UPrimitiveComponent*>(iter);
        }
        else
        {
            continue;
        }

        if (pObj && !pObj->IsA<UGizmoBaseComponent>())
        {
            float Distance = 0.0f;
            int currentIntersectCount = 0;
            if (RayIntersectsObject(pickPosition, pObj, Distance, currentIntersectCount))
            {
                if (Distance < minDistance)
                {
                    minDistance = Distance;
                    maxIntersect = currentIntersectCount;
                    Possible = pObj;
                }
                else if (abs(Distance - minDistance) < FLT_EPSILON && currentIntersectCount > maxIntersect)
                {
                    maxIntersect = currentIntersectCount;
                    Possible = pObj;
                }
            }
        }
    }
    if (Possible)
    {
        GetWorld()->AddSelectedActor(Possible->GetOwner());
    }
}

void AEditorPlayer::AddControlMode()
{
    cMode = static_cast<ControlMode>((cMode + 1) % CM_END);
}

void AEditorPlayer::AddCoordiMode()
{
    cdMode = static_cast<CoordiMode>((cdMode + 1) % CDM_END);
}

void AEditorPlayer::ScreenToViewSpace(int screenX, int screenY, const FMatrix& viewMatrix, const FMatrix& projectionMatrix, FVector& pickPosition)
{
    D3D11_VIEWPORT viewport = GetEngine()->GetLevelEditor()->GetActiveViewportClient()->GetD3DViewport();
    
    float viewportX = screenX - viewport.TopLeftX;
    float viewportY = screenY - viewport.TopLeftY;

    pickPosition.X = ((2.0f * viewportX / viewport.Width) - 1) / projectionMatrix[0][0];
    pickPosition.Y = -((2.0f * viewportY / viewport.Height) - 1) / projectionMatrix[1][1];
    if (GetEngine()->GetLevelEditor()->GetActiveViewportClient()->IsOrtho())
    {
        pickPosition.Z = 0.0f;  // 오쏘 모드에서는 unproject 시 near plane 위치를 기준
    }
    else
    {
        pickPosition.Z = 1.0f;  // 퍼스펙티브 모드: near plane
    }
}

int AEditorPlayer::RayIntersectsObject(const FVector& pickPosition, USceneComponent* obj, float& hitDistance, int& intersectCount)
{
	FMatrix scaleMatrix = FMatrix::CreateScaleMatrix(
		obj->GetWorldScale().X,
		obj->GetWorldScale().Y,
		obj->GetWorldScale().Z
	);
	FMatrix rotationMatrix = FMatrix::CreateRotationMatrix(
		obj->GetWorldRotation().Roll,
		obj->GetWorldRotation().Pitch,
		obj->GetWorldRotation().Yaw
	);

	FMatrix translationMatrix = FMatrix::CreateTranslationMatrix(obj->GetWorldLocation());

	// ���� ��ȯ ���
	FMatrix worldMatrix = scaleMatrix * rotationMatrix * translationMatrix;
	FMatrix viewMatrix = GEngine->GetLevelEditor()->GetActiveViewportClient()->GetViewMatrix();
    
    bool bIsOrtho = GetEngine()->GetLevelEditor()->GetActiveViewportClient()->IsOrtho();
    

    if (bIsOrtho)
    {
        // 오쏘 모드: ScreenToViewSpace()에서 계산된 pickPosition이 클립/뷰 좌표라고 가정
        FMatrix inverseView = FMatrix::Inverse(viewMatrix);
        // pickPosition을 월드 좌표로 변환
        FVector worldPickPos = inverseView.TransformPosition(pickPosition);  
        // 오쏘에서는 픽킹 원점은 unproject된 픽셀의 위치
        FVector rayOrigin = worldPickPos;
        // 레이 방향은 카메라의 정면 방향 (평행)
        FVector orthoRayDir = GEngine->GetLevelEditor()->GetActiveViewportClient()->ViewTransformOrthographic.GetForwardVector().Normalize();

        // 객체의 로컬 좌표계로 변환
        FMatrix localMatrix = FMatrix::Inverse(worldMatrix);
        FVector localRayOrigin = localMatrix.TransformPosition(rayOrigin);
        FVector localRayDir = (localMatrix.TransformPosition(rayOrigin + orthoRayDir) - localRayOrigin).Normalize();
        
        intersectCount = obj->CheckRayIntersection(localRayOrigin, localRayDir, hitDistance);
        return intersectCount;
    }
    else
    {
        FMatrix inverseMatrix = FMatrix::Inverse(worldMatrix * viewMatrix);
        FVector cameraOrigin = { 0,0,0 };
        FVector pickRayOrigin = inverseMatrix.TransformPosition(cameraOrigin);
        // 퍼스펙티브 모드의 기존 로직 사용
        FVector transformedPick = inverseMatrix.TransformPosition(pickPosition);
        FVector rayDirection = (transformedPick - pickRayOrigin).Normalize();
        
        intersectCount = obj->CheckRayIntersection(pickRayOrigin, rayDirection, hitDistance);
        return intersectCount;
    }
}

void AEditorPlayer::PickedObjControl()
{
    
    if (GetWorld()->GetPickingGizmo())
    {
        POINT currentMousePos;
        GetCursorPos(&currentMousePos);
        int32 deltaX = currentMousePos.x - lastMousePos.x;
        int32 deltaY = currentMousePos.y - lastMousePos.y;

        // USceneComponent* pObj = GetWorld()->GetPickingObj();
        //AActor* PickedActor = GetWorld()->GetSelectedActors();
        for (auto pickedActor : GetWorld()->GetSelectedActors())
        {
            if (pickedActor== nullptr)
                continue;
            UGizmoBaseComponent* Gizmo = static_cast<UGizmoBaseComponent*>(GetWorld()->GetPickingGizmo());
            switch (cMode)
            {
            case CM_TRANSLATION:
                ControlTranslation(pickedActor->GetRootComponent(), Gizmo, deltaX, deltaY);
                break;
            case CM_SCALE:
                ControlScale(pickedActor->GetRootComponent(), Gizmo, deltaX, deltaY);
                break;
            case CM_ROTATION:
                ControlRotation(pickedActor->GetRootComponent(), Gizmo, deltaX, deltaY);
                break;
            default:
                break;
            }
        }
        lastMousePos = currentMousePos;
    }
        
}

void AEditorPlayer::ControlRotation(USceneComponent* pObj, UGizmoBaseComponent* Gizmo, int32 deltaX, int32 deltaY)
{
    FVector cameraForward = GetEngine()->GetLevelEditor()->GetActiveViewportClient()->ViewTransformPerspective.GetForwardVector();
    FVector cameraRight = GetEngine()->GetLevelEditor()->GetActiveViewportClient()->ViewTransformPerspective.GetRightVector();
    FVector cameraUp = GetEngine()->GetLevelEditor()->GetActiveViewportClient()->ViewTransformPerspective.GetUpVector();

    FQuat currentRotation = pObj->GetWorldRotation().ToQuaternion();

    FQuat rotationDelta;

    if (bLAltDown && !bAlreadyDup)
    {
        GetWorld()->DuplicateSeletedActorsOnLocation();
        bAlreadyDup = true;
    }
    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleX)
    {
        float rotationAmount = (cameraUp.Z >= 0 ? -1.0f : 1.0f) * deltaY * 0.01f;
        rotationAmount = rotationAmount + (cameraRight.X >= 0 ? 1.0f : -1.0f) * deltaX * 0.01f;

        rotationDelta = FQuat(FVector(1.0f, 0.0f, 0.0f), rotationAmount); // ���� X �� ���� ȸ��
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleY)
    {
        float rotationAmount = (cameraRight.X >= 0 ? 1.0f : -1.0f) * deltaX * 0.01f;
        rotationAmount = rotationAmount + (cameraUp.Z >= 0 ? 1.0f : -1.0f) * deltaY * 0.01f;

        rotationDelta = FQuat(FVector(0.0f, 1.0f, 0.0f), rotationAmount); // ���� Y �� ���� ȸ��
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::CircleZ)
    {
        float rotationAmount = (cameraForward.X <= 0 ? -1.0f : 1.0f) * deltaX * 0.01f;
        rotationDelta = FQuat(FVector(0.0f, 0.0f, 1.0f), rotationAmount); // ���� Z �� ���� ȸ��
    }
    if (cdMode == CDM_LOCAL)
    {
        pObj->SetRelativeRotation(currentRotation * rotationDelta);
    }
    else if (cdMode == CDM_WORLD)
    {
        pObj->SetRelativeRotation(rotationDelta * currentRotation);
    }
}

void AEditorPlayer::ControlTranslation(USceneComponent* pObj, UGizmoBaseComponent* Gizmo, int32 deltaX, int32 deltaY)
{
    float DeltaX = static_cast<float>(deltaX);
    float DeltaY = static_cast<float>(deltaY);
    auto ActiveViewport = GetEngine()->GetLevelEditor()->GetActiveViewportClient();

    FVector CamearRight = ActiveViewport->GetViewportType() == LVT_Perspective ?
        ActiveViewport->ViewTransformPerspective.GetRightVector() : ActiveViewport->ViewTransformOrthographic.GetRightVector();
    FVector CameraUp = ActiveViewport->GetViewportType() == LVT_Perspective ?
        ActiveViewport->ViewTransformPerspective.GetUpVector() : ActiveViewport->ViewTransformOrthographic.GetUpVector();
    
    FVector WorldMoveDirection = (CamearRight * DeltaX + CameraUp * -DeltaY) * 0.1f;

    if (bLAltDown && !bAlreadyDup)
    {
        GetWorld()->DuplicateSeletedActorsOnLocation();
        bAlreadyDup = true;
    }
    if (cdMode == CDM_LOCAL)
    {
        if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowX)
        {
            float moveAmount = WorldMoveDirection.Dot(pObj->GetForwardVector());
            pObj->AddLocation(pObj->GetForwardVector() * moveAmount);
        }
        else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowY)
        {
            float moveAmount = WorldMoveDirection.Dot(pObj->GetRightVector());
            pObj->AddLocation(pObj->GetRightVector() * moveAmount);
        }
        else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowZ)
        {
            float moveAmount = WorldMoveDirection.Dot(pObj->GetUpVector());
            pObj->AddLocation(pObj->GetUpVector() * moveAmount);
        }
    }
    else if (cdMode == CDM_WORLD)
    {
        float distance = (GEngine->GetLevelEditor()->GetActiveViewportClient()->ViewTransformPerspective.GetLocation() - (*GEngine->GetWorld()->GetSelectedActors().begin())->GetActorLocation()).Magnitude();
        distance/=100.0f;
        // 월드 좌표계에서 카메라 방향을 고려한 이동
        if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowX)
        {
            // 카메라의 오른쪽 방향을 X축 이동에 사용
            FVector moveDir = CamearRight * DeltaX * 0.05f;
            pObj->AddLocation(FVector(moveDir.X, 0.0f, 0.0f) * distance);
        }
        else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowY)
        {
            // 카메라의 오른쪽 방향을 Y축 이동에 사용
            FVector moveDir = CamearRight * DeltaX * 0.05f;
            pObj->AddLocation(FVector(0.0f, moveDir.Y, 0.0f) * distance);
        }
        else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ArrowZ)
        {
            // 카메라의 위쪽 방향을 Z축 이동에 사용
            FVector moveDir = CameraUp * -DeltaY * 0.05f;
            pObj->AddLocation(FVector(0.0f, 0.0f, moveDir.Z) * distance);
        }
    }
}

void AEditorPlayer::ControlScale(USceneComponent* pObj, UGizmoBaseComponent* Gizmo, int32 deltaX, int32 deltaY)
{
    float DeltaX = static_cast<float>(deltaX);
    float DeltaY = static_cast<float>(deltaY);
    auto ActiveViewport = GetEngine()->GetLevelEditor()->GetActiveViewportClient();

    FVector CamearRight = ActiveViewport->GetViewportType() == LVT_Perspective ?
        ActiveViewport->ViewTransformPerspective.GetRightVector() : ActiveViewport->ViewTransformOrthographic.GetRightVector();
    FVector CameraUp = ActiveViewport->GetViewportType() == LVT_Perspective ?
        ActiveViewport->ViewTransformPerspective.GetUpVector() : ActiveViewport->ViewTransformOrthographic.GetUpVector();
    
    // 월드 좌표계에서 카메라 방향을 고려한 이동
    if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleX)
    {
        // 카메라의 오른쪽 방향을 X축 이동에 사용
        FVector moveDir = CamearRight * DeltaX * 0.05f;
        pObj->AddScale(FVector(moveDir.X, 0.0f, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleY)
    {
        // 카메라의 오른쪽 방향을 Y축 이동에 사용
        FVector moveDir = CamearRight * DeltaX * 0.05f;
        pObj->AddScale(FVector(0.0f, moveDir.Y, 0.0f));
    }
    else if (Gizmo->GetGizmoType() == UGizmoBaseComponent::ScaleZ)
    {
        // 카메라의 위쪽 방향을 Z축 이동에 사용
        FVector moveDir = CameraUp * -DeltaY * 0.05f;
        pObj->AddScale(FVector(0.0f, 0.0f, moveDir.Z));
    }
}
