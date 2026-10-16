#include "EnigmaCampaignMode.h"
#include "EnigmaGameInstance.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "HAL/PlatformMisc.h"

namespace Story {
const FLinearColor Gold(.83f,.62f,.32f),Muted(.55f,.66f,.68f);
FString S(const TSharedPtr<FJsonObject>& O,const TCHAR* Key){FString V;if(O)O->TryGetStringField(Key,V);return V;}
double N(const TSharedPtr<FJsonObject>& O,const TCHAR* Key){double V=0;if(O)O->TryGetNumberField(Key,V);return V;}
TSharedPtr<FJsonObject> O(const TSharedPtr<FJsonObject>& P,const TCHAR* Key){const TSharedPtr<FJsonObject>* V=nullptr;return P&&P->TryGetObjectField(Key,V)?*V:MakeShared<FJsonObject>();}
const TArray<TSharedPtr<FJsonValue>>& A(const TSharedPtr<FJsonObject>& P,const TCHAR* Key){static TArray<TSharedPtr<FJsonValue>> Empty;const TArray<TSharedPtr<FJsonValue>>* V=nullptr;return P&&P->TryGetArrayField(Key,V)?*V:Empty;}
bool Won(const TSharedPtr<FJsonObject>& Save,const FString& Id){for(const auto& V:A(Save,TEXT("ChronicleWins")))if(V->AsString()==Id)return true;return false;}
bool Ready(const TSharedPtr<FJsonObject>& Save,const TSharedPtr<FJsonObject>& Entry){const FString Previous=S(Entry,TEXT("Previous"));return N(Save,TEXT("CompletedStage"))>=N(Entry,TEXT("RequiredStage"))&&(Previous.IsEmpty()||Won(Save,Previous));}
}

int32 AEnigmaCampaignMode::BattleRegion(){
 using namespace Story;const auto State=Game()->State;
 if(S(State,TEXT("mode"))==TEXT("campaign"))return FMath::Clamp((int32(N(State,TEXT("stage")))-1)/10,0,6);
 if(S(State,TEXT("mode"))==TEXT("chronicle")){
  const auto& Routes=A(State,TEXT("chronicles"));int32 Index=int32(N(State,TEXT("stage")))-1;
  if(Routes.IsValidIndex(Index))return FMath::Clamp(int32(N(Routes[Index]->AsObject(),TEXT("BattleRegion"))),0,6);
 }
 return S(State,TEXT("mode"))==TEXT("floral")?0:6;
}

void AEnigmaCampaignHUD::DrawStoryMap(){
 using namespace Story;const auto State=Mode->Game()->State;const auto Save=O(State,TEXT("save"));const auto& Routes=A(State,TEXT("chronicles"));
 if(Routes.Num()!=4){Wrap(TEXT("As Rotas da Ruptura não estão disponíveis nesta instalação."),45,180,100,3);return;}
 const bool Map=Mode->Page==TEXT("routes"),Outcome=Mode->Page==TEXT("routeoutcome");
 auto Picture=[&](UTexture2D* Image,float X,float Y,float W,float H,float U=0,float V=0,float UW=1,float VH=1){if(Image)DrawTexture(Image,ScreenX+X*Scale,ScreenY+Y*Scale,W*Scale,H*Scale,U,V,UW,VH,FLinearColor(.78f,.86f,.86f));};
 if(Map){
  Text(TEXT("Rotas da Ruptura"),45,110,Gold,1.8f);
  Wrap(TEXT("ATO II · Além do Sol Partido. As feridas entre os mundos continuam abertas. Siga seus vestígios e descubra quem está usando a ruptura."),45,158,150,2);
  for(int32 I=0;I<4;I++){
   const auto Entry=Routes[I]->AsObject();const float X=45+I*300;
   const bool Complete=Won(Save,S(Entry,TEXT("Id"))),Open=Ready(Save,Entry);
   Box(X,222,280,280,FLinearColor(.013f,.031f,.04f,.97f));
   Picture(Texture(S(Entry,TEXT("MapArt"))),X+1,223,278,145);
   Box(X,223,280,145,FLinearColor(.005f,.015f,.02f,Open?.13f:.55f));
   Box(X,222,280,1,Open?Gold:FLinearColor(.27f,.37f,.4f));
   Text(FString::Printf(TEXT("%02d  /  %s"),I+1,Complete?TEXT("CONCLUÍDA"):Open?TEXT("DISPONÍVEL"):TEXT("SELADA")),X+12,238,Open?Gold:Muted,.78f);
   Text(S(Entry,TEXT("Title")),X+12,387,Gold,1.05f);
   Text(S(Entry,TEXT("Subtitle")),X+12,420,Muted,.78f);
   const FString Label=Complete?TEXT("Revisitar história"):Open?TEXT("Iniciar travessia"):N(Save,TEXT("CompletedStage"))<70?TEXT("Conclua o Ato I"):TEXT("Conclua a rota anterior");
   Button(Label,X+12,453,256,TEXT("chronicle"),TEXT(""),I,Open);
   if(I<3)Box(X+280,352,20,1,FLinearColor(.83f,.62f,.32f,.45f));
  }
  const int32 Count=A(Save,TEXT("ChronicleWins")).Num();
  Text(FString::Printf(TEXT("%d / 4 HISTÓRIAS CONCLUÍDAS"),Count),45,530,Gold,1.f);
  Wrap(N(Save,TEXT("CompletedStage"))<70?TEXT("Conclua a campanha no Palácio do Sol Partido para abrir a primeira rota. O progresso e as recompensas das rotas são salvos separadamente das 70 etapas do primeiro ato."):TEXT("A vitória abre a próxima passagem. Cada rota concede sua recompensa apenas uma vez; depois, você pode revisitar o confronto e suas crônicas."),45,568,128,3);
  Button(TEXT("Retornar ao atlas"),970,588,250,TEXT("page"),TEXT("campaign"));
  return;
 }
 const int32 Index=FMath::Clamp(Mode->SelectedChronicle,0,Routes.Num()-1);const auto Entry=Routes[Index]->AsObject();
 Text(S(Entry,TEXT("Title")),45,110,Gold,1.7f);Text(S(Entry,TEXT("Subtitle")),45,153,Muted,1.f);
 Box(45,194,758,360,FLinearColor(.013f,.03f,.04f,.93f));Box(45,194,758,1,FLinearColor(.83f,.62f,.32f,.35f));
 Text(Outcome?TEXT("AS MARCAS DA TRAVESSIA"):TEXT("ANTES DO CONFRONTO"),65,211,Gold,.8f);
 Wrap(S(Entry,Outcome?TEXT("Outro"):TEXT("Intro")),65,248,105,11);
 const FString Id=S(Entry,TEXT("Id"));auto Hero=Texture(TEXT("Hero_")+Id+(Id==TEXT("mazakin")?TEXT(""):TEXT("_phase1")));
 if(Hero){
  if(Id==TEXT("mazakin")){
   const float PortraitWidth=360.f*(Hero->GetSizeX()*.16f)/(Hero->GetSizeY()*.40f);
   Picture(Hero,850+(345-PortraitWidth)*.5f,194,PortraitWidth,360,.30f,.04f,.16f,.40f);
  }
  else{const float Fit=FMath::Min(345.f/Hero->GetSizeX(),360.f/Hero->GetSizeY());const float PW=Hero->GetSizeX()*Fit,PH=Hero->GetSizeY()*Fit;Picture(Hero,850+(345-PW)*.5f,194+(360-PH)*.5f,PW,PH);}
 }
 else Picture(Texture(S(Entry,TEXT("MapArt"))),850,194,345,360);
 Box(850,510,345,44,FLinearColor(.005f,.015f,.022f,.92f));Text(TEXT("CAPÍTULO ")+FString::FromInt(Index+1)+TEXT("  /  ROTAS DA RUPTURA"),865,525,Gold,.73f);
 if(!Outcome){
  FString BossName;S(Entry,TEXT("Subtitle")).Split(TEXT(" · "),&BossName,nullptr);
  Button(TEXT("Enfrentar ")+BossName,65,584,310,TEXT("routeFight"),TEXT(""),0,Ready(Save,Entry));
  Text(S(Entry,TEXT("Objective")),65,557,Muted,.75f);
 }else{
  Text(TEXT("O caminho adiante está aberto."),65,557,Gold,.85f);
  Button(TEXT("Continuar a travessia"),65,584,310,TEXT("page"),TEXT("routes"));
 }
 Button(TEXT("Retornar às rotas"),405,584,270,TEXT("page"),TEXT("routes"));
}

void AEnigmaCampaignMode::TickChronicleTour(){
 using namespace Story;if(Clock<ChronicleNext)return;ChronicleNext=Clock+2;
 auto Check=[&](bool OK,const TCHAR* Label){ChronicleChecks++;if(!OK){ChronicleFailures++;UE_LOG(LogTemp,Error,TEXT("CHRONICLE_QA_FAIL %s"),Label);}};
 FString Shot;
 switch(ChronicleTourIndex++){
  case 0:Check(A(Game()->State,TEXT("chronicles")).Num()==4,TEXT("four_routes"));for(const TCHAR* Name:{TEXT("Hero_akuroth_phase1"),TEXT("Hero_hajun_phase1"),TEXT("Hero_zhujin_phase1"),TEXT("Hero_mazakin")}){const FString Art(Name);Check(LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/GameArt/")+Art+TEXT(".")+Art))!=nullptr,TEXT("story_portrait_cooked"));}Check(N(O(Game()->State,TEXT("save")),TEXT("CompletedStage"))==70,TEXT("isolated_preview_save"));Shot=TEXT("01-rotas");break;
  case 1:{auto Choice=Buttons.FindByPredicate([](const FButton& B){return B.Name==TEXT("chronicle")&&B.Number==0&&B.Enabled;});Check(Choice!=nullptr,TEXT("first_route_available"));if(Choice)Click(Choice->Min+Choice->Size*.5f);Check(Page==TEXT("routestory"),TEXT("story_route"));Shot=TEXT("02-zhu-jin");break;}
  case 2:Action(TEXT("routeFight"));Check(Page==TEXT("battle"),TEXT("native_encounter"));Check(S(A(O(Game()->State,TEXT("battle")),TEXT("enemies"))[0]->AsObject(),TEXT("id"))==TEXT("zhujin"),TEXT("story_boss_is_zhujin"));Shot=TEXT("03-confronto");break;
  case 3:Action(TEXT("chronicle"),TEXT(""),1);Shot=TEXT("04-hajun");break;
  case 4:Check(!Buttons.ContainsByPredicate([](const FButton& B){return B.Name==TEXT("routeFight")&&B.Enabled;}),TEXT("next_route_gated"));Action(TEXT("chronicle"),TEXT(""),2);Shot=TEXT("05-mazakin");break;
  case 5:Action(TEXT("chronicle"),TEXT(""),3);Shot=TEXT("06-akuroth");break;
  case 6:SelectedChronicle=0;Action(TEXT("page"),TEXT("routeoutcome"));Shot=TEXT("07-consequencia-preview");break;
  case 7:Action(TEXT("page"),TEXT("routes"));O(Game()->State,TEXT("save"))->SetNumberField(TEXT("CompletedStage"),0);Shot=TEXT("08-mapa-selado");break;
  case 8:Check(!Buttons.ContainsByPredicate([](const FButton& B){return B.Name==TEXT("chronicle")&&B.Enabled;}),TEXT("first_act_required"));Game()->Command(TEXT("state"));Action(TEXT("page"),TEXT("home"));Shot=TEXT("09-entrada-ato-dois");break;
  case 9:HomeFocus=0;MenuAccept();Check(Page==TEXT("routes"),TEXT("completed_act_home_route"));return;
  default:UE_LOG(LogTemp,Display,TEXT("ENIGMA_CHRONICLE_QA_RESULT checks=%d failures=%d"),ChronicleChecks,ChronicleFailures);FPlatformMisc::RequestExitWithStatus(false,ChronicleFailures?1:0);return;
 }
 FTimerHandle Handle;GetWorldTimerManager().SetTimer(Handle,[this,Shot](){
  using namespace Story;int32 Width=0,Height=0;GetWorld()->GetFirstPlayerController()->GetViewportSize(Width,Height);
  for(const auto& B:Buttons){ChronicleChecks++;if(B.Min.X<0||B.Min.Y<0||B.Min.X+B.Size.X>Width+.5f||B.Min.Y+B.Size.Y>Height+.5f){ChronicleFailures++;UE_LOG(LogTemp,Error,TEXT("CHRONICLE_QA_BOUNDS %s"),*Shot);}}
  FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(ChronicleTourFolder/(Shot+TEXT(".png"))),true,false);
 },.4f,false);
}
