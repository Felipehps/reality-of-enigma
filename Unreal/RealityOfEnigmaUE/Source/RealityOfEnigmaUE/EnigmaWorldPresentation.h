#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Dom/JsonObject.h"
#include "EnigmaWorldPresentation.generated.h"
class AEnigmaStoryEnvironment;class UProceduralMeshComponent;class UDirectionalLightComponent;class USoundWave;class UAudioComponent;class UStaticMeshComponent;class USkeletalMeshComponent;class UInstancedStaticMeshComponent;class UMaterialInterface;class UStaticMesh;class UAnimSequence;class ACameraActor;class UPointLightComponent;
UCLASS()
class REALITYOFENIGMAUE_API AEnigmaWorldPresentation:public AActor {
 GENERATED_BODY()
public:
 AEnigmaWorldPresentation();virtual void BeginPlay() override;virtual void Tick(float Dt) override;
 void ShowBattle(TSharedPtr<FJsonObject> Battle,int32 Region);
 void PlayAction(TSharedPtr<FJsonObject> Battle,float Volume);void HideBattle();
 void PreviewEnvironment(const FString& Id,bool PhaseTwo);
 AEnigmaStoryEnvironment* Environment() const{return StoryEnvironment;}
 bool ReducedMotion=false;bool HasImpactCueAt(float At) const;
 bool Ready=false;bool Transforming() const{return TransformAge>=0&&TransformAge<3;}
private:
 UPROPERTY() TObjectPtr<AEnigmaStoryEnvironment> StoryEnvironment;
 void SyncEnvironment(TSharedPtr<FJsonObject> Battle,int32 Region);
 UPROPERTY() TObjectPtr<UStaticMeshComponent> TripoArena;
 UPROPERTY() TMap<FString,TObjectPtr<UAnimSequence>> TripoClips;
 bool IsTripoBody(int32 Index) const;
 UAnimSequence* BodyClip(int32 Index,const FString& Kind);
 void SyncTripoArt();
 bool TripoTransformPending=false;bool TripoArenaEnabled=false;int32 TripoArenaPhase=0;
 UPROPERTY() TArray<TObjectPtr<USkeletalMeshComponent>> Bodies;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Scenery;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Effects;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Dark;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Crimson;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> ShadowLimbs;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> WhiteHair;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Gold;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Lightning;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Mount;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Petals;
 UPROPERTY() TMap<FString,TObjectPtr<UProceduralMeshComponent>> Garments;
 UPROPERTY() TObjectPtr<UAnimSequence> Attack2;
 UPROPERTY() TObjectPtr<UAnimSequence> Attack3;
 UPROPERTY() TObjectPtr<UAnimSequence> ChargedAttack;
 UPROPERTY() TObjectPtr<UAnimSequence> Walk;
 UPROPERTY() TObjectPtr<UAnimSequence> HitReact;
 UPROPERTY() TObjectPtr<UAnimSequence> Death;
 TArray<bool> RenderAlive;TArray<float> LastReact,ReactUntil;
 FString TransformBoss,AscendedBossKey;int32 ArenaRegion=6;
 void DrawNewCharacter(int32 Index,TSharedPtr<FJsonObject> Unit,FVector Position);
 void DrawNewSkill(FVector Source);
 void DrawAkurothCostume(int32 Index,TSharedPtr<FJsonObject> Unit);
 void UpdateGarment(int32 Index,const FString& Id,FVector Position);
 void FaceRing(UInstancedStaticMeshComponent* Batch,FVector P,float Radius);
 UPROPERTY() TObjectPtr<UStaticMeshComponent> Backdrop;
 UPROPERTY() TObjectPtr<UStaticMesh> Sphere;
 UPROPERTY() TObjectPtr<UStaticMesh> Cylinder;
 UPROPERTY() TObjectPtr<UStaticMesh> Cube;
 UPROPERTY() TObjectPtr<UAnimSequence> Idle;
 UPROPERTY() TObjectPtr<UAnimSequence> Attack;
 UPROPERTY() TObjectPtr<UMaterialInterface> Material;
 UPROPERTY() TObjectPtr<ACameraActor> Camera;
 UPROPERTY() TObjectPtr<UPointLightComponent> Light;
 UPROPERTY() TObjectPtr<UDirectionalLightComponent> KeyLight;
 UPROPERTY() TObjectPtr<UDirectionalLightComponent> RimLight;
 TMap<FString,double> BeforeHealth;
 TArray<FString> Keys;TArray<FVector> Homes;TArray<FLinearColor> Colors;
 TSharedPtr<FJsonObject> Snapshot;
 FString Skill,Operator;int32 Attacker=-1;TArray<int32> Victims;float Age=-1,Clock=0,TransformAge=-1,Duration=2.2f;bool Ascended=false;
 UPROPERTY() TMap<FString,TObjectPtr<USoundWave>> Sounds;
 UPROPERTY() TArray<TObjectPtr<UAudioComponent>> Voices;
 TSharedPtr<FJsonObject> SoundProfiles,SampleProfiles;
 struct FSoundEvent {float At;FString Cue;float Pitch,Gain;};TArray<FSoundEvent> SoundEvents;
 float PlayVolume=.7f,LastStep=-1;FString StepCue;float StepPitch=1;
 void PlayCue(const FString& Cue,float Pitch=1,float Gain=1);void StopSounds();
 void Beam(UInstancedStaticMeshComponent* Batch,FVector A,FVector B,float Width);
 void Ball(UInstancedStaticMeshComponent* Batch,FVector P,float Radius);
 void Ring(UInstancedStaticMeshComponent* Batch,FVector P,float Radius);
 void Shape(UStaticMesh* Mesh,FVector P,FVector S,FLinearColor Color,FRotator R=FRotator::ZeroRotator);
};
