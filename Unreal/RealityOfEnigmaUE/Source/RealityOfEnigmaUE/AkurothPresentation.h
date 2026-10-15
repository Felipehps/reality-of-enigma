#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AkurothPresentation.generated.h"
class AAkurothArenaMode;class ACameraActor;class USkeletalMeshComponent;
class UStaticMesh;class UMaterialInterface;class UMaterialInstanceDynamic;
class UInstancedStaticMeshComponent;class UProceduralMeshComponent;class UAnimSequence;
class UPointLightComponent;

UCLASS()
class REALITYOFENIGMAUE_API AAkurothPresentation:public AActor {
    GENERATED_BODY()
public:
    AAkurothPresentation();
    void Initialize(AAkurothArenaMode* Combat,AActor* Hero,AActor* Boss,ACameraActor* Camera);
    virtual void Tick(float Dt) override;
    void PlaySkill(int32 Index);void PlayHeroes();
    bool AssetsReady=false;int32 LastSkill=-1;
private:
    UPROPERTY() TObjectPtr<AAkurothArenaMode> Mode;
    UPROPERTY() TObjectPtr<AActor> HeroRoot;
    UPROPERTY() TObjectPtr<AActor> BossRoot;
    UPROPERTY() TObjectPtr<ACameraActor> View;
    UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Heroes;
    UPROPERTY() TObjectPtr<USkeletalMeshComponent> BossBody;
    UPROPERTY() TObjectPtr<UAnimSequence> Idle;
    UPROPERTY() TObjectPtr<UAnimSequence> Attack;
    UPROPERTY() TObjectPtr<UStaticMesh> Ball;
    UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
    UPROPERTY() TObjectPtr<UStaticMesh> Block;
    UPROPERTY() TObjectPtr<UMaterialInterface> BaseMaterial;
    UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Red;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Purple;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Shadow;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Hair;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Sparks;
    UPROPERTY() TObjectPtr<UProceduralMeshComponent> Cape;
    UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> HeroCapes;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Blood;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Void;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Rubble;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> BackdropMaterial;
    UPROPERTY() TObjectPtr<UPointLightComponent> KeyLight;
    TArray<FVector> HeroHomes;TArray<FVector> CapeVertices;
    float Clock=0,SkillAge=-1,HeroAttackAge=-1,HitAge=-1,PreviousHP=10000;
    int32 SkillIndex=-1;bool WasPhaseTwo=false;FVector CameraHome;
    UMaterialInstanceDynamic* Material(FLinearColor Color,float Glow=0);
    UInstancedStaticMeshComponent* Batch(UStaticMesh* Mesh,UMaterialInterface* Mat);
    void Instance(UInstancedStaticMeshComponent* Batch,FVector Position,FVector Scale);
    void Beam(UInstancedStaticMeshComponent* Batch,FVector Start,FVector End,float Width);
    void Ring(UInstancedStaticMeshComponent* Batch,FVector Center,float Radius,float Tilt=0,int32 Segments=48);
    void StaticShape(UStaticMesh* Mesh,FVector Position,FVector Scale,UMaterialInterface* Mat,FRotator Rotation=FRotator::ZeroRotator);
    void Sound(int32 Index);
};
