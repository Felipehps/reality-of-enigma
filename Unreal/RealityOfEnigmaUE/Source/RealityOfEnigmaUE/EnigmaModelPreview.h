#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnigmaModelPreview.generated.h"
class USkeletalMeshComponent;class UAnimSequence;class UStaticMeshComponent;class USceneCaptureComponent2D;class UTextureRenderTarget2D;
UCLASS()
class REALITYOFENIGMAUE_API AEnigmaModelPreview:public AActor {
 GENERATED_BODY()
public:
 AEnigmaModelPreview();virtual void BeginPlay() override;
 bool Ready() const;void SetActive(bool Active);void Rotate(float Degrees);
 bool MotionReady() const;bool PlayMotion(int32 Index);void ShowStatic();bool ShowingMotion() const{return Motion;}
 FVector HandLocation() const;
 UTextureRenderTarget2D* Texture() const{return Target;}
private:
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Model;
 UPROPERTY() TObjectPtr<USkeletalMeshComponent> AnimatedModel;
 UPROPERTY() TArray<TObjectPtr<UAnimSequence>> MotionClips;
 UPROPERTY() TObjectPtr<USceneCaptureComponent2D> Capture;
 UPROPERTY() TObjectPtr<UTextureRenderTarget2D> Target;
 float Angle=180;bool Active=false,Motion=false;
};
