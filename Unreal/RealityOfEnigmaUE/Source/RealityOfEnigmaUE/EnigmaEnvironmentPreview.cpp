#include "EnigmaCampaignMode.h"
#include "EnigmaGameInstance.h"
#include "EnigmaWorldPresentation.h"
#include "EnigmaStoryEnvironment.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "TimerManager.h"

void AEnigmaCampaignHUD::DrawEnvironmentPreview(){
 const auto& Rows=AEnigmaStoryEnvironment::Definitions();if(!Rows.IsValidIndex(Mode->SceneryIndex))return;
 const auto& D=Rows[Mode->SceneryIndex];
 Box(0,0,1280,126,FLinearColor(.005f,.012f,.018f,.84f));
 Text(TEXT("ATLAS DAS REALIDADES"),42,22,FLinearColor(.83f,.62f,.32f),.8f);
 Text(D.Title,40,47,FLinearColor(.93f,.78f,.51f),1.85f);
 Text(FString::Printf(TEXT("%02d / %02d  ·  %s"),Mode->SceneryIndex+1,Rows.Num(),Mode->SceneryPhase?TEXT("FORMA TRANSFORMADA"):D.HasPhase?TEXT("ANTES DA TRANSFORMAÇÃO"):TEXT("A JORNADA")),42,94,FLinearColor(.72f,.8f,.8f),.76f);
 Box(0,574,1280,146,FLinearColor(.005f,.012f,.018f,.88f));Wrap(D.Story,42,586,155,3);
 Button(TEXT("Anterior"),42,664,145,TEXT("sceneryNext"),TEXT(""),-1);
 Button(TEXT("Próximo"),205,664,145,TEXT("sceneryNext"),TEXT(""),1);
 if(D.HasPhase)Button(Mode->SceneryPhase?TEXT("Primeira fase"):TEXT("Transformação"),374,664,220,TEXT("sceneryPhase"));
 Button(TEXT("Voltar aos extras"),1020,664,220,TEXT("page"),TEXT("extras"));
}

void AEnigmaCampaignMode::TickSceneryTour(){
 if(Clock<SceneryNext)return;SceneryNext=Clock+1.9f;
 auto Check=[&](bool OK,const TCHAR* Label){SceneryChecks++;if(!OK){SceneryFailures++;UE_LOG(LogTemp,Error,TEXT("SCENERY_QA_FAIL %s index=%d"),Label,SceneryTourIndex);}};
 const auto& Rows=AEnigmaStoryEnvironment::Definitions();
 // Twelve locations, plus each boss's transformed environment, in narrative order.
 TArray<TPair<int32,bool>> Shots;for(int I=0;I<Rows.Num();I++){Shots.Add({I,false});if(Rows[I].HasPhase)Shots.Add({I,true});}
 if(SceneryTourIndex==0){Check(Rows.Num()==12,TEXT("all_story_locations"));TSet<FString> IDs;for(auto& D:Rows){Check(!IDs.Contains(D.Id),TEXT("unique_location"));IDs.Add(D.Id);}for(int I=0;I<7;I++)Check(AEnigmaStoryEnvironment::ForBattle(I,{})==Rows[I+1].Id,TEXT("region_selection"));}
 if(SceneryTourIndex<Shots.Num()){
  const auto Shot=Shots[SceneryTourIndex];SceneryIndex=Shot.Key;SceneryPhase=Shot.Value;
  Action(TEXT("page"),TEXT("scenery"));WorldArt->PreviewEnvironment(Rows[SceneryIndex].Id,SceneryPhase);
  auto Env=WorldArt->Environment();Check(Env&&Env->CurrentId==Rows[SceneryIndex].Id,TEXT("live_environment"));
  Check(Env&&Env->Phase==SceneryPhase,TEXT("phase_selection"));Check(Env&&Env->TriangleCount>1000&&Env->TriangleCount<180000,TEXT("bounded_real_geometry"));
  const FString File=Rows[SceneryIndex].Id+(SceneryPhase?TEXT("-phase2"):TEXT("-phase1"));
  FTimerHandle Handle;GetWorldTimerManager().SetTimer(Handle,[this,File](){FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(SceneryTourFolder/(File+TEXT(".png"))),true,false);},.5f,false);
 }else if(SceneryTourIndex==Shots.Num()){
  SceneryIndex=Rows.Num()-1;Action(TEXT("sceneryNext"),TEXT(""),1);Check(SceneryIndex==0&&WorldArt->Environment()->CurrentId==Rows[0].Id,TEXT("gallery_next_wrap"));
  Action(TEXT("sceneryNext"),TEXT(""),-1);Check(SceneryIndex==Rows.Num()-1,TEXT("gallery_previous_wrap"));
  Action(TEXT("sceneryPhase"));Check(WorldArt->Environment()->Phase,TEXT("gallery_phase_action"));
  // Exercise production battle integration, including enemy-only overrides and phase resets.
  Start(TEXT("demo"));const TSharedPtr<FJsonObject>* BattlePtr=nullptr;
  Check(Game()->State->TryGetObjectField(TEXT("battle"),BattlePtr),TEXT("native_battle_available"));
  if(BattlePtr){auto Battle=*BattlePtr;auto Enemies=Battle->GetArrayField(TEXT("enemies"));auto Unit=Enemies[0]->AsObject();
   for(int I=0;I<7;I++){Unit->SetStringField(TEXT("id"),TEXT("enemy_fenda"));WorldArt->ShowBattle(Battle,I);Check(WorldArt->Environment()->CurrentId==Rows[I+1].Id,TEXT("live_region_routing"));}
   for(int I=8;I<Rows.Num();I++){
    Unit->SetStringField(TEXT("id"),Rows[I].Id);
    for(bool Phase:{false,true,false}){Unit->SetBoolField(TEXT("ascended"),Phase&&Rows[I].Id!=TEXT("zhujin"));Unit->SetBoolField(TEXT("crimson"),Phase&&Rows[I].Id==TEXT("zhujin"));WorldArt->ShowBattle(Battle,0);Check(WorldArt->Environment()->CurrentId==Rows[I].Id&&WorldArt->Environment()->Phase==Phase,TEXT("live_boss_phase_routing"));}
   }
  }
  Action(TEXT("page"),TEXT("home"));Check(WorldArt->Environment()->IsHidden(),TEXT("hide_after_battle"));
 }else if(SceneryTourIndex<=Shots.Num()+5){
  const int32 Sample=SceneryTourIndex-Shots.Num()-1;
  const TCHAR* Modes[]={TEXT("story"),TEXT("zhujin"),TEXT("hajun"),TEXT("mazakin"),TEXT("akuroth")};
  const TCHAR* Expected[]={TEXT("forest"),TEXT("zhujin"),TEXT("hajun"),TEXT("mazakin"),TEXT("akuroth")};
  Start(Modes[Sample]);Check(Page==TEXT("battle")&&WorldArt->Environment()->CurrentId==Expected[Sample]&&!WorldArt->Environment()->Phase,TEXT("unmodified_native_encounter"));
  const FString File=TEXT("battle-")+FString(Expected[Sample]);FTimerHandle Handle;
  GetWorldTimerManager().SetTimer(Handle,[this,File](){FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(SceneryTourFolder/(File+TEXT(".png"))),true,false);},.5f,false);
 }else{
  UE_LOG(LogTemp,Display,TEXT("ENIGMA_SCENERY_QA_RESULT checks=%d failures=%d"),SceneryChecks,SceneryFailures);FPlatformMisc::RequestExitWithStatus(false,SceneryFailures?1:0);
 }
 SceneryTourIndex++;
}
