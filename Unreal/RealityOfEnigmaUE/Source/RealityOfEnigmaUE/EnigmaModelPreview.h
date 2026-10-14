#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnigmaModelPreview.generated.h"
class UStaticMeshComponent;class USceneCaptureComponent2D;class UTextureRenderTarget2D;
UCLASS()
class REALITYOFENIGMAUE_API AEnigmaModelPreview:public AActor {
 GENERATED_BODY()
public:
 AEnigmaModelPreview();virtual void BeginPlay() override;
 bool Ready() const;void SetActive(bool Active);void Rotate(float Degrees);
 UTextureRenderTarget2D* Texture() const{return Target;}
private:
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Model;
 UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
 UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
 float Angle=180;bool Active=false;
};
