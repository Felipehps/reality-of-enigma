#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"
namespace EnigmaCombatTiming {
 inline bool IsAnnihilationDischarge(const TSharedPtr<FJsonObject>& Battle){
  FString Skill,Actor;Battle->TryGetStringField(TEXT("lastSkill"),Skill);if(Skill!=TEXT("akuroth_annihilation"))return false;Battle->TryGetStringField(TEXT("lastActor"),Actor);
  for(const TCHAR* Side:{TEXT("allies"),TEXT("enemies")}){const TArray<TSharedPtr<FJsonValue>>* Units=nullptr;if(Battle->TryGetArrayField(Side,Units))for(auto Unit:*Units){auto U=Unit->AsObject();FString Key;bool Charging=false;U->TryGetStringField(TEXT("key"),Key);U->TryGetBoolField(TEXT("charging"),Charging);if(Key==Actor&&Charging)return false;}}
  const TArray<TSharedPtr<FJsonValue>>* Hits=nullptr;if(Battle->TryGetArrayField(TEXT("impacts"),Hits))for(auto Value:*Hits){FString Kind;Value->AsObject()->TryGetStringField(TEXT("kind"),Kind);if(Kind==TEXT("physical")||Kind==TEXT("essence"))return true;}return false;
 }

 inline float ImpactAt(const FString& Skill,const TSharedPtr<FJsonObject>& Hit){
  double Index=0;FString Kind;Hit->TryGetNumberField(TEXT("hit"),Index);Hit->TryGetStringField(TEXT("kind"),Kind);
  if(Kind==TEXT("poison")||Kind==TEXT("bleed")||Kind==TEXT("hazard"))return .25f;
  if(Skill==TEXT("akuroth_annihilation"))return int32(Index)%2?3.f:1.1f;
  return (Skill.Contains(TEXT("meteor"))||Skill.Contains(TEXT("rain"))?1.3f:.65f)+FMath::Clamp(int32(Index),0,10)*.08f;
 }
 inline double HealthAt(double Initial,double Final,double Maximum,const FString& Key,const FString& Skill,const TArray<TSharedPtr<FJsonValue>>& Hits,float Age,float Duration){
  double HP=Initial;bool HadHit=false;
  for(auto Value:Hits){auto Hit=Value->AsObject();FString Target,Kind;double Amount=0;Hit->TryGetStringField(TEXT("target"),Target);if(Target!=Key)continue;HadHit=true;Hit->TryGetStringField(TEXT("kind"),Kind);Hit->TryGetNumberField(TEXT("amount"),Amount);if(Age>=ImpactAt(Skill,Hit))HP+=Amount*(Kind==TEXT("heal")?1:-1);}
  if((!HadHit&&Age>=.65f)||Age>=Duration-.1f)HP=Final;return FMath::Clamp(HP,0.0,Maximum);
 }
}
