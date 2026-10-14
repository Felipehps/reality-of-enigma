#pragma once
#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "Dom/JsonObject.h"
#include "EnigmaCampaignMode.generated.h"
class UTexture2D;class UEnigmaGameInstance;class AEnigmaWorldPresentation;
UCLASS()
class REALITYOFENIGMAUE_API AEnigmaCampaignMode:public AGameModeBase {
 GENERATED_BODY()
public:
 AEnigmaCampaignMode();virtual void BeginPlay() override;virtual void Tick(float Dt) override;
 void Action(const FString& Name,const FString& Id=TEXT(""),int32 Number=0);
 void Click(FVector2D Position);void Wheel(int32 Direction);void Key(int32 Index);
 FString Page=TEXT("home"),Selected=TEXT("kael"),Target,TrainingBoss=TEXT("varkhan"),Message;
 int32 Region=0,ListPage=0,TeamSlot=0,OpeningShot=0,PendingStage=1;bool Auto=false,Repeat=false,ReducedMotion=false;
 float Volume=.7f,AnimationAge=-1,Clock=0;bool MoveMenu=false;
 bool LogVisible=false,OpeningAuto=true,ReplayOpening=false;
 float OpeningAge=0;
 TSharedPtr<FJsonObject> DisplayState();
 UEnigmaGameInstance* Game();
 struct FButton {FVector2D Min,Size;FString Name,Id;int32 Number=0;bool Enabled=true;};
 TArray<FButton> Buttons;
private:
 UPROPERTY() TObjectPtr<AEnigmaWorldPresentation> WorldArt;
 TSharedPtr<FJsonObject> BeforeAction;
 float AutoAge=0,AnimationDuration=2.2f;
 TSharedPtr<FJsonObject> AnimationDisplay;
 void UpdateDisplayedHealth();void PresentLastAction();
 FString CombatReplayFolder;TArray<FString> CombatReplayNames;int32 CombatReplayIndex=0,CombatReplayChecks=0,CombatReplayFailures=0;float CombatReplayNext=3;
 void TickCombatReplay();void CheckReplayHealth(TSharedPtr<FJsonObject> Sample,const FString& Name);
 FString UITourFolder;int32 UITourIndex=0;float UITourNext=3;int32 UIWarnings=0;
 void TickUITour();void CaptureUI(const FString& Name);
 void Start(const FString& Kind,int32 Stage=0);
 void Act(const FString& Skill=TEXT(""),int32 Cell=-1);void SavePreferences();void RunChecks();
};
UCLASS()
class REALITYOFENIGMAUE_API AEnigmaCampaignHUD:public AHUD {
 GENERATED_BODY()
public:virtual void DrawHUD() override;
private:
 UPROPERTY() TMap<FString,TObjectPtr<UTexture2D>> Art;
 AEnigmaCampaignMode* Mode=nullptr;float Scale=1;
 UTexture2D* Texture(const FString& Name);
 void Text(const FString& Value,float X,float Y,FLinearColor Color=FLinearColor::White,float Size=1);
 void Box(float X,float Y,float W,float H,FLinearColor Color);
 void Button(const FString& Label,float X,float Y,float W,const FString& Name,const FString& Id=TEXT(""),int32 Number=0,bool Enabled=true,float H=38);
 void Wrap(const FString& Value,float X,float Y,int32 Width,int32 MaxLines=8);
};
UCLASS()
class REALITYOFENIGMAUE_API AEnigmaCampaignController:public APlayerController {
 GENERATED_BODY()
public:void NoOp(){} virtual void BeginPlay() override;virtual void SetupInputComponent() override;
};
