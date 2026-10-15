#include "EnigmaCampaignMode.h"
#include "EnigmaGameInstance.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Texture2D.h"
#include "Fonts/SlateFontInfo.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "TimerManager.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"

namespace {
const FLinearColor Gold(.83f,.62f,.32f,1), Ivory(.93f,.9f,.83f,1), Muted(.48f,.57f,.6f,1);
double Number(const TSharedPtr<FJsonObject>& O,const TCHAR* Key){double N=0;if(O)O->TryGetNumberField(Key,N);return N;}
TSharedPtr<FJsonObject> Object(const TSharedPtr<FJsonObject>& O,const TCHAR* Key){const TSharedPtr<FJsonObject>* V=nullptr;return O&&O->TryGetObjectField(Key,V)?*V:MakeShared<FJsonObject>();}
bool Seen(const TSharedPtr<FJsonObject>& O){bool V=false;return O&&O->TryGetBoolField(TEXT("OpeningSeen"),V)&&V;}
}

namespace {
FSlateFontInfo MenuFont(float Size,bool Display){
 const FString Font=Display?FPaths::ProjectContentDir()/TEXT("UI/Fonts/Cinzel.ttf"):FPaths::EngineContentDir()/TEXT("Slate/Fonts/Roboto-Regular.ttf");
 static TMap<FString,TSharedPtr<const FCompositeFont>> Fonts;
 auto& Composite=Fonts.FindOrAdd(Font);
 if(!Composite.IsValid())Composite=MakeShared<FCompositeFont>(FName(TEXT("Regular")),Font,EFontHinting::Default,EFontLoadingPolicy::LazyLoad);
 return FSlateFontInfo(Composite,FMath::Max(8.f,Size),FName(TEXT("Regular")));
}
struct FMeasuredMenuText: FCanvasTextItem {
 using FCanvasTextItem::FCanvasTextItem;
 float Width()const{return GetTextSize(1.f).X;}
};
}
float AEnigmaCampaignHUD::MenuTextWidth(const FString& Value,float Size,bool Display){
 FMeasuredMenuText Item(FVector2D::ZeroVector,FText::FromString(Value),GEngine->GetMediumFont(),FLinearColor::White);
 Item.SlateFontInfo=MenuFont(Size*Scale,Display);
 return Item.Width()/Scale;
}
void AEnigmaCampaignHUD::MenuText(const FString& Value,float X,float Y,float Size,FLinearColor Color,bool Display){
 const FSlateFontInfo Info=MenuFont(Size*Scale,Display);
 FCanvasTextItem Item(FVector2D(HomeX+X*Scale,HomeY+Y*Scale),FText::FromString(Value),GEngine->GetMediumFont(),Color);
 Item.SlateFontInfo=Info;
 Item.EnableShadow(FLinearColor(0,0,0,.65f),FVector2D(0,1));
 Canvas->DrawItem(Item);
}

void AEnigmaCampaignHUD::MenuButton(const FString& Label,const FString& Detail,float X,float Y,float W,float H,const FString& Name,const FString& Id,int32 NumberValue,bool Primary){
 const int32 Index=Mode->Buttons.Num();
 const FVector2D Min(HomeX+X*Scale,HomeY+Y*Scale),Size(W*Scale,H*Scale);
 float MX=0,MY=0;auto PC=GetOwningPlayerController();
 const bool Mouse=PC&&PC->GetMousePosition(MX,MY);
 const bool Over=Mouse&&MX>=Min.X&&MX<Min.X+Size.X&&MY>=Min.Y&&MY<Min.Y+Size.Y;
 if(Over&&(!Mode->HomeQuitDialog||Name==TEXT("quit")||Name==TEXT("homeCancel"))&&FVector2D::DistSquared(LastHomeMouse,FVector2D(MX,MY))>1)Mode->HomeFocus=Index;
 const bool Focus=Index==Mode->HomeFocus;
 float& A=HomeHover.FindOrAdd(Name+TEXT("/")+Id+FString::FromInt(Index));
 A=Mode->ReducedMotion?(Focus?1.f:0.f):FMath::FInterpTo(A,Focus?1.f:0.f,FMath::Min(GetWorld()->GetDeltaSeconds(),.05f),12.f);
 auto Rect=[&](float PX,float PY,float PW,float PH,FLinearColor C){DrawRect(C,HomeX+PX*Scale,HomeY+PY*Scale,PW*Scale,PH*Scale);};
 auto Line=[&](FVector2D P,FVector2D Q,FLinearColor C,float Thickness=1.f){FCanvasLineItem L(FVector2D(HomeX,HomeY)+P*Scale,FVector2D(HomeX,HomeY)+Q*Scale);L.SetColor(C);L.LineThickness=Thickness*Scale;Canvas->DrawItem(L);};
 if(Primary){
  Rect(X,Y,W,H,FLinearColor(.015f,.025f,.028f,.35f+.25f*A));
  Rect(X,Y,W,1,FLinearColor(.83f,.62f,.32f,.45f+.5f*A));
  Rect(X,Y+H-1,W,1,FLinearColor(.83f,.62f,.32f,.25f+.35f*A));
  Rect(X,Y,2,H,FLinearColor(.83f,.62f,.32f,.65f+.3f*A));
 }else if(!Detail.IsEmpty()){
  Rect(X,Y,W,H,FLinearColor(.06f,.08f,.09f,.05f+.42f*A));
  Rect(X,Y+H-1,W*(.2f+.8f*A),.7f,FLinearColor(.83f,.62f,.32f,.1f+.4f*A));
 }
 const float Inset=Primary?25.f:Detail.IsEmpty()?0.f:22.f;
 MenuText(Label,X+Inset+4*A,Y+(Detail.IsEmpty()?9:Primary?9:4),Primary?19:Detail.IsEmpty()?11:17,Focus?Ivory:FLinearColor(.68f,.73f,.73f),Primary);
 if(!Detail.IsEmpty())MenuText(Detail,X+Inset+4*A,Y+(Primary?39:32),10,FLinearColor(.48f+.2f*A,.56f+.13f*A,.59f+.05f*A));
 if(Detail.IsEmpty())Rect(X,Y+H-3,W*A,1,FLinearColor(.83f,.62f,.32f,.75f*A));
 else{
  const float CY=Y+H*.5f;
  Line(FVector2D(X+W-28,CY-4),FVector2D(X+W-22,CY),FLinearColor(.83f,.62f,.32f,.35f+.65f*A));
  Line(FVector2D(X+W-22,CY),FVector2D(X+W-28,CY+4),FLinearColor(.83f,.62f,.32f,.35f+.65f*A));
  if(Focus){
   const float CX=X-10;
   Line(FVector2D(CX,CY-5),FVector2D(CX+4,CY),Gold);
   Line(FVector2D(CX+4,CY),FVector2D(CX,CY+5),Gold);
   Line(FVector2D(CX,CY+5),FVector2D(CX-4,CY),Gold);
   Line(FVector2D(CX-4,CY),FVector2D(CX,CY-5),Gold);
  }
 }
 Mode->Buttons.Add({Min,Size,Name,Id,NumberValue,true});
}

void AEnigmaCampaignHUD::DrawHome(){
 HomeX=(Canvas->ClipX-1280*Scale)*.5f;HomeY=(Canvas->ClipY-720*Scale)*.5f;
 const float Clock=Mode->ReducedMotion?0:Mode->Clock;
 DrawRect(FLinearColor(.006f,.012f,.019f),0,0,Canvas->ClipX,Canvas->ClipY);
 UTexture2D* Back=Texture(TEXT("TitleLimiar"));
 if(!Back)Back=Texture(TEXT("HomeBackground"));
 if(Back){
  const float Cover=FMath::Max(Canvas->ClipX/float(Back->GetSizeX()),Canvas->ClipY/float(Back->GetSizeY()));
  const float Zoom=Mode->ReducedMotion?1.f:1.018f+.004f*FMath::Sin(Clock*.13f);
  const float W=Back->GetSizeX()*Cover*Zoom,H=Back->GetSizeY()*Cover*Zoom;
  const float DX=Mode->ReducedMotion?0:3*FMath::Sin(Clock*.11f);
  DrawTexture(Back,(Canvas->ClipX-W)*.5f+DX,(Canvas->ClipY-H)*.5f,W,H,0,0,1,1,FLinearColor::White);
 }
 // Smooth scrims protect text while leaving the landscape bright and legible.
 for(int32 I=0;I<64;I++){
  const float T=I/63.f,Alpha=.68f*FMath::Pow(1-T,2.5f);
  DrawRect(FLinearColor(.008f,.017f,.026f,Alpha),I*Canvas->ClipX/64,0,Canvas->ClipX/64,Canvas->ClipY);
  DrawRect(FLinearColor(.003f,.009f,.015f,.55f*FMath::Pow(T,5.f)),0,I*Canvas->ClipY/64,Canvas->ClipX,Canvas->ClipY/64);
 }
 auto Rect=[&](float X,float Y,float W,float H,FLinearColor C){DrawRect(C,HomeX+X*Scale,HomeY+Y*Scale,W*Scale,H*Scale);};
 auto Line=[&](FVector2D P,FVector2D Q,FLinearColor C,float Thickness=1.f){FCanvasLineItem L(FVector2D(HomeX,HomeY)+P*Scale,FVector2D(HomeX,HomeY)+Q*Scale);L.SetColor(C);L.LineThickness=Thickness*Scale;Canvas->DrawItem(L);};
 // The broken diamond is an original navigation mark, shared by the title and focus.
 const FVector2D C(81,52);
 Line(C+FVector2D(-10,0),C+FVector2D(0,-15),Gold);Line(C+FVector2D(2,-12),C+FVector2D(12,0),Gold);
 Line(C+FVector2D(10,3),C+FVector2D(0,15),Gold);Line(C+FVector2D(-2,12),C+FVector2D(-12,0),Gold);
 MenuText(TEXT("O LIMIAR ENTRE REALIDADES"),108,43,10,FLinearColor(.71f,.73f,.69f));
 MenuText(TEXT("REALITY OF ENIGMA"),979,43,10,Gold,true);
 const bool Extras=Mode->Page==TEXT("extras");
 if(!Extras){
  MenuText(TEXT("REALITY"),72,139,59,Gold,true);
  MenuText(TEXT("OF ENIGMA"),75,207,44,Gold,true);
  Rect(76,281,36,1.5f,Gold);MenuText(TEXT("UM MUNDO PARTIDO. DESTINOS ENTRELAÇADOS."),127,275,9.5f,FLinearColor(.7f,.74f,.74f));
  auto Save=Object(Mode->Game()->State,TEXT("save"));const int32 Completed=Number(Save,TEXT("CompletedStage"));
  const bool Started=Seen(Save)||Completed>0;
  MenuButton(Completed>=70?TEXT("Jornada concluída"):Started?TEXT("Continuar jornada"):TEXT("Começar jornada"),Completed>=70?TEXT("Revisite o mundo e descubra novos desafios"):Started?FString::Printf(TEXT("Campanha  /  Etapa %02d de 70"),FMath::Min(70,Completed+1)):TEXT("Atravesse o limiar. Sua história começa aqui."),76,344,374,68,Completed>=70?TEXT("page"):TEXT("start"),TEXT("campaign"),FMath::Min(70,Completed+1),true);
  MenuButton(TEXT("Explorar o mundo"),TEXT("Descubra as sete regiões da campanha"),76,428,374,58,TEXT("page"),TEXT("campaign"));
  MenuButton(TEXT("Personagens"),TEXT("Reúna seus heróis. Escolha sua formação."),76,496,374,58,TEXT("page"),TEXT("team"));
  Rect(76,584,374,.7f,FLinearColor(.4f,.49f,.5f,.38f));
  MenuButton(TEXT("Configurações"),TEXT(""),76,605,116,35,TEXT("page"),TEXT("settings"));
  MenuButton(TEXT("Extras"),TEXT(""),220,605,62,35,TEXT("page"),TEXT("extras"));
  MenuButton(TEXT("Sair"),TEXT(""),319,605,46,35,TEXT("homeQuit"));
  MenuText(TEXT("A RUPTURA"),908,557,11,Gold,true);
  MenuText(TEXT("Nem todo fim é o fim."),908,584,16,Ivory,true);
  // Seven fragments reflect campaign progress without introducing a second map UI.
  for(int32 I=0;I<7;I++){
   const bool Unlocked=I<=Completed/10;const float X=912+I*31,Y=627+(I%2?3.f:0.f);
   const FLinearColor FC=Unlocked?Gold:FLinearColor(.44f,.51f,.54f,.5f);
   Line(FVector2D(X,Y-3),FVector2D(X+3,Y),FC);Line(FVector2D(X+3,Y),FVector2D(X,Y+3),FC);
   Line(FVector2D(X,Y+3),FVector2D(X-3,Y),FC);Line(FVector2D(X-3,Y),FVector2D(X,Y-3),FC);
   if(I<6)Line(FVector2D(X+7,Y),FVector2D(X+23,627+((I+1)%2?3.f:0.f)),FLinearColor(.5f,.55f,.52f,.3f));
  }
 }else{
  MenuText(TEXT("ALÉM DA"),75,127,38,Ivory,true);MenuText(TEXT("JORNADA"),72,175,53,Ivory,true);
  MenuText(TEXT("PREPARE-SE PARA ATRAVESSAR O DESCONHECIDO"),76,257,10,Muted);
  MenuButton(TEXT("Treinamento"),TEXT("Prepare sua equipe para os próximos desafios"),76,307,400,54,TEXT("page"),TEXT("training"));
  MenuButton(TEXT("Arenas"),TEXT("Enfrente os chefes e experimente formações"),76,371,400,54,TEXT("page"),TEXT("arenas"));
  MenuButton(TEXT("Invocação"),TEXT("Encontre novos aliados para a sua jornada"),76,435,400,54,TEXT("page"),TEXT("summon"));
  MenuButton(TEXT("Compêndio"),TEXT("Conheça os heróis e as ameaças deste mundo"),76,499,400,54,TEXT("page"),TEXT("cards"));
  MenuButton(TEXT("Rever prólogo"),TEXT(""),76,600,122,35,TEXT("page"),TEXT("opening"));
  MenuButton(TEXT("Voltar ao início"),TEXT(""),240,600,142,35,TEXT("page"),TEXT("home"));
 }
 if(!Mode->ReducedMotion){
  for(int32 I=0;I<26;I++){
   const float Seed=I*37.71f;
   const float X=520+FMath::Fmod(Seed*11+FMath::Sin(Clock*.12f+I)*13,700.f);
   const float Y=710-FMath::Fmod(I*29.f+Clock*(3.f+(I%3)),680.f);
   const float A=.13f+.12f*FMath::Sin(Clock*.5f+I);
   Rect(X,Y,I%5==0?2.f:1.f,I%5==0?2.f:1.f,FLinearColor(.91f,.64f,.32f,A));
  }
 }
 MenuText(TEXT("PROGRESSO SALVO AUTOMATICAMENTE"),76,680,8,FLinearColor(.4f,.5f,.53f));
 MenuText(TEXT("TAB / DIRECIONAIS  NAVEGAR    ENTER  SELECIONAR    ESC  VOLTAR"),795,680,8,FLinearColor(.56f,.62f,.61f));
 if(!Mode->Message.IsEmpty())MenuText(Mode->Message.Left(85),76,654,10,FLinearColor(1,.56f,.3f));
 if(Mode->HomeQuitDialog){
  Mode->Buttons.Reset();
  DrawRect(FLinearColor(.002f,.005f,.01f,.88f),0,0,Canvas->ClipX,Canvas->ClipY);
  Rect(360,250,560,213,FLinearColor(.022f,.034f,.043f));Rect(360,250,560,1,Gold);
  MenuText(TEXT("ENCERRAR A JORNADA?"),398,285,23,Ivory,true);
  MenuText(TEXT("Seu progresso será salvo antes de sair."),398,336,13,Muted);
  MenuButton(TEXT("Continuar aqui"),TEXT(""),399,390,170,38,TEXT("homeCancel"));
  MenuButton(TEXT("Salvar e sair"),TEXT(""),675,390,160,38,TEXT("quit"));
 }
 float MX=0,MY=0;if(GetOwningPlayerController()&&GetOwningPlayerController()->GetMousePosition(MX,MY))LastHomeMouse=FVector2D(MX,MY);
}

void AEnigmaCampaignMode::MenuNavigate(int32 Direction){
 if(Page==TEXT("battle"))return;
 for(int32 I=0;I<Buttons.Num();I++){HomeFocus=(HomeFocus+Direction+Buttons.Num())%Buttons.Num();if(Buttons[HomeFocus].Enabled)break;}
}
void AEnigmaCampaignMode::MenuAccept(){
 if(Page==TEXT("battle"))return;
 if(Buttons.IsValidIndex(HomeFocus)){const FButton Choice=Buttons[HomeFocus];if(Choice.Enabled)Action(Choice.Name,Choice.Id,Choice.Number);}
}
void AEnigmaCampaignMode::MenuBack(){
 if(HomeQuitDialog){HomeQuitDialog=false;HomeFocus=0;return;}
 if(Page==TEXT("home")){HomeQuitDialog=true;HomeFocus=0;return;}
 Action(TEXT("page"),TEXT("home"));
}

void AEnigmaCampaignMode::TickHomeTour(){
 if(Clock<HomeTourNext)return;HomeTourNext=Clock+1.6f;
 auto Check=[this](bool OK,const TCHAR* Label){HomeTourChecks++;if(!OK){HomeTourFailures++;UE_LOG(LogTemp,Error,TEXT("HOME_QA_FAIL %s"),Label);}};
 auto Capture=[this](const FString& Name){FTimerHandle H;GetWorldTimerManager().SetTimer(H,[this,Name](){
  int32 W=0,Ht=0;UGameplayStatics::GetPlayerController(this,0)->GetViewportSize(W,Ht);
  for(const auto& B:Buttons){HomeTourChecks++;if(B.Min.X<0||B.Min.Y<0||B.Min.X+B.Size.X>W+.5f||B.Min.Y+B.Size.Y>Ht+.5f){HomeTourFailures++;UE_LOG(LogTemp,Error,TEXT("HOME_QA_BOUNDS %s"),*Name);}}
  for(int32 I=0;I<Buttons.Num();I++)for(int32 J=I+1;J<Buttons.Num();J++){
   const auto& P=Buttons[I];const auto& Q=Buttons[J];
   const float OverlapWidth=FMath::Min(P.Min.X+P.Size.X,Q.Min.X+Q.Size.X)-FMath::Max(P.Min.X,Q.Min.X);
   const float OverlapHeight=FMath::Min(P.Min.Y+P.Size.Y,Q.Min.Y+Q.Size.Y)-FMath::Max(P.Min.Y,Q.Min.Y);
   HomeTourChecks++;if(OverlapWidth>1&&OverlapHeight>1){HomeTourFailures++;UE_LOG(LogTemp,Error,TEXT("HOME_QA_OVERLAP %s %s %s"),*Name,*P.Name,*Q.Name);}
  }
  FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(HomeTourFolder/(Name+TEXT(".png"))),true,false);
 },.35f,false);};
 switch(HomeTourIndex++){
  case 0:Check(Page==TEXT("home"),TEXT("startup"));Check(LoadObject<UTexture2D>(nullptr,TEXT("/Game/GameArt/TitleLimiar.TitleLimiar"))!=nullptr,TEXT("background_cooked"));Check(IFileManager::Get().FileExists(*(FPaths::ProjectContentDir()/TEXT("UI/Fonts/Cinzel.ttf"))),TEXT("font_staged"));Check(Buttons.Num()==6,TEXT("home_navigation_count"));Capture(TEXT("01-inicio"));break;
  case 1:HomeFocus=0;MenuNavigate(-1);Check(HomeFocus==5,TEXT("keyboard_wrap"));MenuNavigate(1);Check(HomeFocus==0,TEXT("keyboard_return"));MenuNavigate(1);MenuAccept();Check(Page==TEXT("campaign"),TEXT("world_map_route"));Capture(TEXT("02-mapa"));break;
  case 2:Action(TEXT("page"),TEXT("home"));HomeFocus=2;Capture(TEXT("03-personagens-foco"));break;
  case 3:MenuAccept();Check(Page==TEXT("team"),TEXT("characters_route"));Capture(TEXT("04-equipe"));break;
  case 4:Action(TEXT("page"),TEXT("extras"));Check(Page==TEXT("extras"),TEXT("extras_route"));Capture(TEXT("05-extras"));break;
  case 5:Check(Buttons.Num()==6,TEXT("extras_navigation_count"));HomeFocus=0;MenuAccept();Check(Page==TEXT("training"),TEXT("training_route"));Capture(TEXT("06-treinamento"));break;
  case 6:Action(TEXT("page"),TEXT("home"));HomeQuitDialog=true;HomeFocus=0;Capture(TEXT("07-sair-confirmacao"));break;
  case 7:MenuBack();Check(!HomeQuitDialog,TEXT("exit_cancel"));ReducedMotion=true;Capture(TEXT("08-movimento-reduzido"));break;
  case 8:HomeFocus=0;MenuAccept();Check(Page==TEXT("opening"),TEXT("new_campaign_opening"));MenuBack();Check(Page==TEXT("home"),TEXT("back_to_home"));ReducedMotion=false;Capture(TEXT("09-inicio-final"));break;
  case 9:Action(TEXT("page"),TEXT("extras"));HomeFocus=1;break;
  case 10:MenuAccept();Check(Page==TEXT("arenas"),TEXT("arenas_route"));Capture(TEXT("10-arenas"));break;
  case 11:Action(TEXT("page"),TEXT("extras"));HomeFocus=2;break;
  case 12:MenuAccept();Check(Page==TEXT("summon"),TEXT("summon_route"));Capture(TEXT("11-invocacao"));break;
  case 13:Action(TEXT("page"),TEXT("extras"));HomeFocus=3;break;
  case 14:MenuAccept();Check(Page==TEXT("cards"),TEXT("compendium_route"));Capture(TEXT("12-compendio"));break;
  case 15:Action(TEXT("page"),TEXT("extras"));HomeFocus=4;break;
  case 16:MenuAccept();Check(Page==TEXT("opening"),TEXT("prologue_replay_route"));MenuBack();Check(Page==TEXT("home"),TEXT("prologue_back"));break;
  case 17:HomeFocus=3;MenuAccept();Check(Page==TEXT("settings"),TEXT("settings_route"));Capture(TEXT("13-configuracoes"));break;
  case 18:MenuBack();HomeQuitDialog=true;HomeFocus=0;break;
  case 19:Check(Buttons.Num()==2,TEXT("modal_owns_input"));if(Buttons.Num()==2){const FButton Cancel=Buttons[0];Click(Cancel.Min+Cancel.Size*.5f);}Check(!HomeQuitDialog,TEXT("mouse_cancel_exit"));Capture(TEXT("14-inicio-validado"));break;
  case 20:Action(TEXT("page"),TEXT("formation"));Capture(TEXT("15-formacao"));break;
  case 21:Check(Buttons.Num()==16,TEXT("formation_actions"));HomeFocus=1;MenuNavigate(1);Check(Buttons.IsValidIndex(HomeFocus)&&Buttons[HomeFocus].Enabled,TEXT("inner_keyboard_navigation"));Selected=TEXT("hajun");Action(TEXT("page"),TEXT("reference"));Capture(TEXT("16-galeria"));break;
  case 22:Action(TEXT("page"),TEXT("opening"));OpeningAuto=false;Capture(TEXT("17-prologo"));break;
  case 23:Action(TEXT("page"),TEXT("ending"));Capture(TEXT("18-cronicas"));break;
  case 24:Action(TEXT("page"),TEXT("campaign"));HomeFocus=1;Capture(TEXT("19-atlas-teclado"));break;
  case 25:MenuNavigate(1);Check(Buttons.IsValidIndex(HomeFocus)&&Buttons[HomeFocus].Enabled&&Buttons[HomeFocus].Name==TEXT("region"),TEXT("locked_stages_skipped"));Start(TEXT("demo"));Capture(TEXT("20-combate"));break;
  case 26:Action(TEXT("page"),TEXT("settings"));HomeFocus=1;ReducedMotion=true;Capture(TEXT("21-configuracoes-acessiveis"));break;
  case 27:MenuNavigate(1);Check(Buttons.IsValidIndex(HomeFocus)&&Buttons[HomeFocus].Name==TEXT("volume")&&Buttons[HomeFocus].Number==1,TEXT("settings_keyboard_navigation"));Action(TEXT("page"),TEXT("home"));ReducedMotion=false;Capture(TEXT("22-inicio-dourado"));break;
  default:UE_LOG(LogTemp,Display,TEXT("ENIGMA_HOME_QA_RESULT checks=%d failures=%d"),HomeTourChecks,HomeTourFailures);FPlatformMisc::RequestExitWithStatus(false,HomeTourFailures?1:0);break;
 }
}
