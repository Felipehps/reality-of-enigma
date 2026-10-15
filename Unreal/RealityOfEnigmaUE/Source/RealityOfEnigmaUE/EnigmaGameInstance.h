#pragma once
#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "Dom/JsonObject.h"
#include "EnigmaGameInstance.generated.h"

UCLASS()
class REALITYOFENIGMAUE_API UEnigmaGameInstance:public UGameInstance {
 GENERATED_BODY()
public:
 virtual void Init() override;
 virtual void Shutdown() override;
 bool Execute(const TSharedPtr<FJsonObject>& Request);
 bool Command(const FString& Name,const FString& Id=TEXT(""),int32 Number=0);
 TSharedPtr<FJsonObject> State;
 FString Error;
private:
 void* Library=nullptr;void* Session=nullptr;
 using FOpen=void*(__cdecl*)();using FClose=void(__cdecl*)(void*);
 using FExecute=char*(__cdecl*)(void*,const char*);using FFree=void(__cdecl*)(char*);
 FClose Close=nullptr;FExecute Run=nullptr;FFree Free=nullptr;
};
