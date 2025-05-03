#pragma once
#include "PlayerCameraManager.h"
#include "FBaseRenderPass.h"
#include "Container/Array.h"

class FLetterBoxRenderPass : public FBaseRenderPass
{
public:
    explicit FLetterBoxRenderPass(const FName& InShaderName);

    virtual ~FLetterBoxRenderPass() {}
    
    void AddRenderObjectsToRenderPass() override;
    void Prepare(FRenderer* Renderer, std::shared_ptr<FViewportClient> InViewportClient, const FString& InShaderName = FString("")) override;
    void Execute(std::shared_ptr<FViewportClient> InViewportClient) override;
    void ClearRenderObjects() override;
    bool bRender;

    class ID3D11Buffer* LetterBoxConstantBuffer = nullptr;

    void UpdateLetterConstant();

    TArray<APlayerCameraManager*> PlayerCameraManagers;
};
