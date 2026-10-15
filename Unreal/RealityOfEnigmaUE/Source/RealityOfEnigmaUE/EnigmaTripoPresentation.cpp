#include "EnigmaWorldPresentation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Materials/MaterialInterface.h"

bool AEnigmaWorldPresentation::IsTripoBody(int32 I) const {
 return Bodies.IsValidIndex(I)&&Bodies[I]->GetSkeletalMeshAsset()&&Bodies[I]->GetSkeletalMeshAsset()->GetName().StartsWith(TEXT("SK_AkurothPhase"));
}
UAnimSequence* AEnigmaWorldPresentation::BodyClip(int32 I,const FString& Kind){
 if(!IsTripoBody(I))return Idle;
 const int32 Phase=Bodies[I]->GetSkeletalMeshAsset()->GetName().EndsWith(TEXT("2"))?2:1;
 const FString Name=FString::Printf(TEXT("AK_%d_"),Phase)+Kind;
 if(auto Existing=TripoClips.Find(Name))return *Existing;
 auto Clip=LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/CharacterModels/Akuroth/")+Name+TEXT(".")+Name));
 TripoClips.Add(Name,Clip);return Clip;
}
void AEnigmaWorldPresentation::SyncTripoArt(){
 if(!Snapshot)return;
 auto Units=Snapshot->GetArrayField(TEXT("allies"));Units.Append(Snapshot->GetArrayField(TEXT("enemies")));int32 Phase=0;
 for(int32 I=0;I<Units.Num()&&I<Bodies.Num();I++){
  const auto U=Units[I]->AsObject();FString Id;U->TryGetStringField(TEXT("id"),Id);if(Id!=TEXT("akuroth"))continue;
  bool Form=false;U->TryGetBoolField(TEXT("ascended"),Form);Phase=Form?2:1;const FString Name=FString::Printf(TEXT("SK_AkurothPhase%d"),Phase);
  if(!Bodies[I]->GetSkeletalMeshAsset()||Bodies[I]->GetSkeletalMeshAsset()->GetName()!=Name){
   auto Mesh=LoadObject<USkeletalMesh>(nullptr,*(TEXT("/Game/CharacterModels/Akuroth/")+Name+TEXT(".")+Name));
   if(Mesh){TripoTransformPending=Form;Bodies[I]->EmptyOverrideMaterials();Bodies[I]->SetSkeletalMesh(Mesh);const FString MatName=FString::Printf(TEXT("M_Akuroth%d"),Phase);auto PBR=LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/CharacterModels/Akuroth/")+MatName+TEXT(".")+MatName));Bodies[I]->SetMaterial(0,PBR);UE_LOG(LogTemp,Display,TEXT("AKU_TRIPO_MATERIAL phase=%d material=%s"),Phase,PBR?*PBR->GetName():TEXT("MISSING"));Bodies[I]->SetWorldScale3D(FVector(2.5f));Bodies[I]->PlayAnimation(BodyClip(I,Form?TEXT("Transform"):TEXT("Idle")),!Form);UE_LOG(LogTemp,Display,TEXT("AKU_TRIPO_BODY phase=%d mesh=%s height_cm=%.2f"),Phase,*Name,Mesh->GetBounds().BoxExtent.Z*2);}
  }
 }
 TripoArenaEnabled=Phase>0;
 if(!TripoArena){TripoArena=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(TripoArena);TripoArena->SetupAttachment(RootComponent);TripoArena->SetCollisionEnabled(ECollisionEnabled::NoCollision);TripoArena->RegisterComponent();}
 if(Phase&&TripoArenaPhase!=Phase){const FString Name=FString::Printf(TEXT("SM_Arena%d"),Phase);auto Mesh=LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/CharacterModels/Akuroth/")+Name+TEXT(".")+Name));TripoArena->SetStaticMesh(Mesh);TripoArenaPhase=Phase;UE_LOG(LogTemp,Display,TEXT("AKU_TRIPO_ARENA phase=%d loaded=%d"),Phase,Mesh!=nullptr);}
 TripoArena->SetVisibility(TripoArenaEnabled);for(auto C:Scenery)if(C!=Backdrop)C->SetVisibility(!TripoArenaEnabled);
}
