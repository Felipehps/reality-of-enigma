#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EnigmaStoryEnvironment.generated.h"
class UProceduralMeshComponent;class UInstancedStaticMeshComponent;class UMaterialInstanceDynamic;class UPointLightComponent;class UExponentialHeightFogComponent;class USkyLightComponent;
struct FEnigmaEnvironment {
 FString Id,Title,Story,Theme;int32 Region=-1;bool HasPhase=false;
 FLinearColor Stone,Accent,Vegetation,Sky;
};
UCLASS()
class REALITYOFENIGMAUE_API AEnigmaStoryEnvironment:public AActor {
 GENERATED_BODY()
public:
 AEnigmaStoryEnvironment();virtual void Tick(float Delta) override;
 static const TArray<FEnigmaEnvironment>& Definitions();
 static FString ForBattle(int32 Region,const TArray<TSharedPtr<class FJsonValue>>& Enemies);
 bool Configure(const FString& Id,bool PhaseTwo);
 void SetReducedMotion(bool Value){ReducedMotion=Value;}
 void Impact(float Strength){Pulse=FMath::Max(Pulse,Strength);}
 FString CurrentId;bool Phase=false;int32 TriangleCount=0;float AmbientTime=0;
 FLinearColor KeyColor() const;FLinearColor RimColor() const;
private:
 UPROPERTY() TObjectPtr<UProceduralMeshComponent> Architecture;
 UPROPERTY() TObjectPtr<UProceduralMeshComponent> Horizon;
 UPROPERTY() TObjectPtr<USkyLightComponent> AmbientLight;
 UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Motes;
 UPROPERTY() TObjectPtr<UExponentialHeightFogComponent> Atmosphere;
 UPROPERTY() TArray<TObjectPtr<UMaterialInstanceDynamic>> Materials;
 UPROPERTY() TArray<TObjectPtr<UPointLightComponent>> Lamps;
 TArray<FVector> MoteSeeds;FEnigmaEnvironment Definition;
 bool ReducedMotion=false;float Pulse=0;
};
