#include "EnigmaCampaignMode.h"
#include "EnigmaModelPreview.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EnigmaCombatTiming.h"
#include "Misc/FileHelper.h"
#include "EnigmaGameInstance.h"
#include "EnigmaWorldPresentation.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"
#include "UnrealClient.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "GameFramework/GameUserSettings.h"
#include "Components/InputComponent.h"
#include "InputCoreTypes.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "HAL/PlatformMisc.h"
namespace EJ {
 TSharedPtr<FJsonObject> O(TSharedPtr<FJsonObject> P,const TCHAR* K){const TSharedPtr<FJsonObject>* V=nullptr;return P&&P->TryGetObjectField(K,V)?*V:MakeShared<FJsonObject>();}
 const TArray<TSharedPtr<FJsonValue>>& A(TSharedPtr<FJsonObject> P,const TCHAR* K){static TArray<TSharedPtr<FJsonValue>> Empty;const TArray<TSharedPtr<FJsonValue>>* V=nullptr;return P&&P->TryGetArrayField(K,V)?*V:Empty;}
 FString S(TSharedPtr<FJsonObject> P,const TCHAR* K){FString V;if(P)P->TryGetStringField(K,V);return V;}
 double N(TSharedPtr<FJsonObject> P,const TCHAR* K){double V=0;if(P)P->TryGetNumberField(K,V);return V;}
 bool B(TSharedPtr<FJsonObject> P,const TCHAR* K){bool V=false;if(P)P->TryGetBoolField(K,V);return V;}
 TSharedPtr<FJsonObject> Find(const TArray<TSharedPtr<FJsonValue>>& A,const FString& Id,const TCHAR* K=TEXT("Id")){for(auto V:A)if(S(V->AsObject(),K)==Id)return V->AsObject();return MakeShared<FJsonObject>();}
}
using namespace EJ;
AEnigmaCampaignMode::AEnigmaCampaignMode(){PrimaryActorTick.bCanEverTick=true;DefaultPawnClass=nullptr;PlayerControllerClass=AEnigmaCampaignController::StaticClass();HUDClass=AEnigmaCampaignHUD::StaticClass();}
UEnigmaGameInstance* AEnigmaCampaignMode::Game(){return Cast<UEnigmaGameInstance>(GetGameInstance());}
TSharedPtr<FJsonObject> AEnigmaCampaignMode::DisplayState(){return AnimationAge>=0&&AnimationDisplay?AnimationDisplay:Game()->State;}
void AEnigmaCampaignMode::BeginPlay(){
 Super::BeginPlay();FParse::Value(FCommandLine::Get(),TEXT("EnigmaHomeTour="),HomeTourFolder);FParse::Value(FCommandLine::Get(),TEXT("EnigmaCombatReplay="),CombatReplayFolder);FParse::Value(FCommandLine::Get(),TEXT("EnigmaUITour="),UITourFolder);WorldArt=GetWorld()->SpawnActor<AEnigmaWorldPresentation>();
 const FString Pref=FPaths::ProjectSavedDir()/TEXT("EnigmaPreferences.ini");GConfig->GetFloat(TEXT("Audio"),TEXT("Volume"),Volume,Pref);GConfig->GetBool(TEXT("Visual"),TEXT("ReducedMotion"),ReducedMotion,Pref);
 WorldArt->ReducedMotion=ReducedMotion;
 if(!UITourFolder.IsEmpty()&&FParse::Param(FCommandLine::Get(),TEXT("EnigmaModelTour"))){UITourIndex=30;UITourNext=1;}
 if(!UITourFolder.IsEmpty()&&FParse::Param(FCommandLine::Get(),TEXT("EnigmaMotionTour"))){UITourIndex=33;UITourNext=1;}
 if(!Game()||!Game()->State.IsValid())Message=Game()?Game()->Error:TEXT("GameInstance não configurado");
 if(!CombatReplayFolder.IsEmpty()){
  FString Manifest;TArray<TSharedPtr<FJsonValue>> Names;if(FFileHelper::LoadFileToString(Manifest,*(CombatReplayFolder/TEXT("manifest.json")))&&FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Manifest),Names)){for(auto Name:Names)CombatReplayNames.Add(Name->AsString());}else{UE_LOG(LogTemp,Error,TEXT("COMBAT_REPLAY_MANIFEST_FAILED"));FPlatformMisc::RequestExitWithStatus(false,1);}
 }
 if(FParse::Param(FCommandLine::Get(),TEXT("EnigmaMigrationQA")))GetWorldTimerManager().SetTimerForNextTick(this,&AEnigmaCampaignMode::RunChecks);
 if(FParse::Param(FCommandLine::Get(),TEXT("EnigmaAkurothTest")))GetWorldTimerManager().SetTimerForNextTick([this](){Start(TEXT("akuroth"));Game()->Command(TEXT("begin"));Auto=true;});
}
void AEnigmaCampaignMode::SavePreferences(){const FString Pref=FPaths::ProjectSavedDir()/TEXT("EnigmaPreferences.ini");GConfig->SetFloat(TEXT("Audio"),TEXT("Volume"),Volume,Pref);GConfig->SetBool(TEXT("Visual"),TEXT("ReducedMotion"),ReducedMotion,Pref);GConfig->Flush(false,Pref);}
void AEnigmaCampaignMode::Start(const FString& Kind,int32 Stage){
 if(!Game()||!Game()->State)return;
 if(Kind==TEXT("campaign")&&N(O(Game()->State,TEXT("save")),TEXT("CompletedStage"))==0&&!B(O(Game()->State,TEXT("save")),TEXT("OpeningSeen"))){Page=TEXT("opening");OpeningShot=0;OpeningAge=0;ReplayOpening=false;PendingStage=Stage;Auto=false;return;}
 auto Request=MakeShared<FJsonObject>();Request->SetStringField(TEXT("command"),TEXT("start"));Request->SetStringField(TEXT("id"),Kind);Request->SetNumberField(TEXT("number"),Stage);Request->SetStringField(TEXT("boss"),TrainingBoss);
 if(!Game()->Execute(Request)){Message=Game()->Error;return;}
 Page=TEXT("battle");MoveMenu=false;Target.Reset();BeforeAction.Reset();AnimationAge=-1;Auto=false;Message.Reset();
 WorldArt->ShowBattle(O(Game()->State,TEXT("battle")),Kind==TEXT("campaign")?(Stage-1)/10:Kind==TEXT("floral")?0:6);
}
void AEnigmaCampaignMode::Act(const FString& Skill,int32 Cell){
 if(AnimationAge>=0||WorldArt->Transforming()||!Game()||!Game()->State)return;
 BeforeAction=Game()->State;
 auto Request=MakeShared<FJsonObject>();Request->SetStringField(TEXT("command"),Cell>=0?TEXT("reposition"):TEXT("act"));Request->SetNumberField(TEXT("number"),Cell);Request->SetStringField(TEXT("id"),Skill);Request->SetStringField(TEXT("target"),Target);
 if(!Game()->Execute(Request)){Message=Game()->Error;BeforeAction.Reset();return;}
 MoveMenu=false;PresentLastAction();
}
void AEnigmaCampaignMode::PresentLastAction(){
 Message.Reset();AnimationAge=0;auto Battle=O(Game()->State,TEXT("battle"));AnimationDuration=EnigmaCombatTiming::IsAnnihilationDischarge(Battle)?4.6f:2.2f;
 FString Copy;FJsonSerializer::Serialize(Game()->State.ToSharedRef(),TJsonWriterFactory<>::Create(&Copy));FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Copy),AnimationDisplay);
 UpdateDisplayedHealth();WorldArt->PlayAction(O(Game()->State,TEXT("battle")),Volume);AutoAge=0;
}
void AEnigmaCampaignMode::Tick(float Dt){
 Super::Tick(Dt);Clock+=Dt;
 if(ModelPreview)ModelPreview->SetActive(Page==TEXT("reference")&&Selected==TEXT("hajun")&&Reference3D);
 if(!UITourFolder.IsEmpty())TickUITour();
 if(!HomeTourFolder.IsEmpty())TickHomeTour();
 if(!CombatReplayFolder.IsEmpty())TickCombatReplay();
 if(Page==TEXT("opening")&&OpeningAuto){OpeningAge+=Dt;if(OpeningAge>9)Action(TEXT("openingNext"));}
 if(AnimationAge>=0){AnimationAge+=Dt;UpdateDisplayedHealth();if(AnimationAge>=AnimationDuration){AnimationAge=-1;BeforeAction.Reset();AnimationDisplay.Reset();WorldArt->ShowBattle(O(Game()->State,TEXT("battle")),S(Game()->State,TEXT("mode"))==TEXT("campaign")?(int32(N(Game()->State,TEXT("stage")))-1)/10:6);}}
 if(Page==TEXT("battle")&&Auto&&AnimationAge<0&&!WorldArt->Transforming()){AutoAge+=Dt;if(AutoAge>.45f){AutoAge=0;auto Battle=O(Game()->State,TEXT("battle"));if(B(Battle,TEXT("finished"))&&B(Battle,TEXT("won"))&&Repeat&&S(Game()->State,TEXT("mode"))==TEXT("training")&&N(Battle,TEXT("wave"))==3){Start(TEXT("training"));Game()->Command(TEXT("begin"));Auto=true;}else if(B(Battle,TEXT("finished"))&&!(S(Game()->State,TEXT("mode"))==TEXT("training")&&B(Battle,TEXT("won"))&&N(Battle,TEXT("wave"))<3))Auto=false;else Act();}}
}
void AEnigmaCampaignMode::Action(const FString& Name,const FString& Id,int32 Number){
 if(!Game()||!Game()->State)return;
 if(Name==TEXT("moveMenu")){MoveMenu=!MoveMenu;return;}
 if(Name==TEXT("reposition")){Act(TEXT(""),Number);return;}
 if(Name==TEXT("reference3D")){if(Selected!=TEXT("hajun"))return;if(!ModelPreview)ModelPreview=GetWorld()->SpawnActor<AEnigmaModelPreview>();Reference3D=ModelPreview&&ModelPreview->Ready();ReferencePhase=1;if(ModelPreview)ModelPreview->ShowStatic();return;}
 if(Name==TEXT("modelMotion")){Action(TEXT("reference3D"));if(!ModelPreview||!ModelPreview->PlayMotion(Number))Message=TEXT("Animação de teste indisponível");return;}
 if(Name==TEXT("previewRotate")){if(ModelPreview&&Reference3D)ModelPreview->Rotate(Number);return;}
 if(Name==TEXT("referencePhase")){Reference3D=false;ReferencePhase=FMath::Clamp(Number,0,2);return;}
 if(Name==TEXT("homeQuit")){HomeQuitDialog=true;HomeFocus=0;return;}
 if(Name==TEXT("homeCancel")){HomeQuitDialog=false;HomeFocus=0;return;}
 if(Name==TEXT("page")){HomeQuitDialog=false;HomeFocus=0;Reference3D=false;ReferencePhase=0;Auto=false;AnimationAge=-1;BeforeAction.Reset();AnimationDisplay.Reset();Page=Id;ListPage=0;if(Id==TEXT("opening")){ReplayOpening=true;OpeningShot=0;OpeningAge=0;}WorldArt->HideBattle();return;}
 if(Name==TEXT("select")){Reference3D=false;ReferencePhase=0;Selected=Id;return;}
 if(Name==TEXT("target")){Target=Id;return;}
 if(Name==TEXT("list")){ListPage=FMath::Max(0,ListPage+Number);return;}
 if(Name==TEXT("nextRegion")){Region=FMath::Min(6,Region+1);Page=TEXT("campaign");return;}
 if(Name==TEXT("region")){Region=Number;return;}
 if(Name==TEXT("slot")){TeamSlot=Number;return;}
 if(Name==TEXT("start")){Start(Id,Number);return;}
 if(Name==TEXT("train")){TrainingBoss=Id;Start(TEXT("training"));return;}
 if(Name==TEXT("openingPause")){OpeningAuto=!OpeningAuto;return;}
 if(Name==TEXT("openingNext")){OpeningAge=0;if(OpeningShot<7)OpeningShot++;else {if(!ReplayOpening)Game()->Command(TEXT("opening"));Start(ReplayOpening?TEXT("story"):TEXT("campaign"),PendingStage);}return;}
 if(Name==TEXT("openingPrevious")){OpeningShot=FMath::Max(0,OpeningShot-1);OpeningAge=0;return;}
 if(Name==TEXT("openingSkip")){if(!ReplayOpening)Game()->Command(TEXT("opening"));Start(ReplayOpening?TEXT("story"):TEXT("campaign"),PendingStage);return;}
 if(Name==TEXT("act")){Act(Id);return;}
 if(Name==TEXT("auto")){Auto=!Auto;return;}
 if(Name==TEXT("repeat")){Repeat=!Repeat;return;}
 if(Name==TEXT("log")){LogVisible=!LogVisible;return;}
 if(Name==TEXT("battleCell")){if(!Game()->Command(TEXT("move"),Target,Number))Message=Game()->Error;else WorldArt->ShowBattle(O(Game()->State,TEXT("battle")),Region);return;}
 if(Name==TEXT("cell")){
  auto Save=O(Game()->State,TEXT("save"));TArray<TSharedPtr<FJsonValue>> Cells=A(Save,TEXT("FormationCells"));
  if(Cells.IsValidIndex(TeamSlot)){int32 Old=int32(Cells[TeamSlot]->AsNumber());for(int32 I=0;I<Cells.Num();I++)if(I!=TeamSlot&&int32(Cells[I]->AsNumber())==Number)Cells[I]=MakeShared<FJsonValueNumber>(Old);Cells[TeamSlot]=MakeShared<FJsonValueNumber>(Number);auto Req=MakeShared<FJsonObject>();Req->SetStringField(TEXT("command"),TEXT("formation"));Req->SetArrayField(TEXT("cells"),Cells);Game()->Execute(Req);}return;
 }
 if(Name==TEXT("volume")){Volume=FMath::Clamp(Volume+Number*.1f,0.f,1.f);SavePreferences();return;}
 if(Name==TEXT("motion")){ReducedMotion=!ReducedMotion;WorldArt->ReducedMotion=ReducedMotion;SavePreferences();return;}
 if(Name==TEXT("fullscreen")){auto Settings=GEngine->GetGameUserSettings();Settings->SetFullscreenMode(Settings->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);Settings->ApplySettings(false);Settings->SaveSettings();return;}
 if(Name==TEXT("quit")){if(!Game()->Command(TEXT("save"))){Message=Game()->Error;return;}FPlatformMisc::RequestExit(false);return;}
 if(Name==TEXT("legacyBoss")){UGameplayStatics::OpenLevel(this,TEXT("/Engine/Maps/Entry"),true,TEXT("game=/Script/RealityOfEnigmaUE.AkurothArenaMode"));return;}
 if(!Game()->Command(Name,Id,Number))Message=Game()->Error;else Message.Reset();
}
void AEnigmaCampaignMode::Click(FVector2D P){for(auto& B:Buttons)if(B.Enabled&&P.X>=B.Min.X&&P.X<=B.Min.X+B.Size.X&&P.Y>=B.Min.Y&&P.Y<=B.Min.Y+B.Size.Y){Action(B.Name,B.Id,B.Number);return;}}
void AEnigmaCampaignMode::Wheel(int32 D){ListPage=FMath::Max(0,ListPage+D);}
void AEnigmaCampaignMode::Key(int32 I){if(Page!=TEXT("battle")||!Game()->State)return;auto Skills=A(O(Game()->State,TEXT("battle")),TEXT("skills"));if(Skills.IsValidIndex(I))Act(S(Skills[I]->AsObject(),TEXT("id")));}
void AEnigmaCampaignController::BeginPlay(){Super::BeginPlay();bShowMouseCursor=true;FInputModeGameAndUI Input;Input.SetHideCursorDuringCapture(false);Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);SetInputMode(Input);}
void AEnigmaCampaignController::SetupInputComponent(){
 Super::SetupInputComponent();auto Get=[this](){return Cast<AEnigmaCampaignMode>(GetWorld()->GetAuthGameMode());};
 InputComponent->BindKey(EKeys::LeftMouseButton,IE_Pressed,this,&AEnigmaCampaignController::NoOp).KeyDelegate.GetDelegateForManualSet().BindLambda([this,Get](){float X,Y;if(GetMousePosition(X,Y)&&Get())Get()->Click(FVector2D(X,Y));});
 InputComponent->BindKey(EKeys::SpaceBar,IE_Pressed,this,&AEnigmaCampaignController::NoOp).KeyDelegate.GetDelegateForManualSet().BindLambda([Get](){if(Get()){if(Get()->Page!=TEXT("battle"))Get()->MenuAccept();else Get()->Action(TEXT("act"));}});
 InputComponent->BindKey(EKeys::Escape,IE_Pressed,this,&AEnigmaCampaignController::NoOp).KeyDelegate.GetDelegateForManualSet().BindLambda([Get](){if(Get())Get()->MenuBack();});
 for(const FKey& NavKey:{EKeys::Up,EKeys::Gamepad_DPad_Up})InputComponent->BindKey(NavKey,IE_Pressed,this,&AEnigmaCampaignController::NoOp).KeyDelegate.GetDelegateForManualSet().BindLambda([Get](){if(Get())Get()->MenuNavigate(-1);});
 for(const FKey& NavKey:{EKeys::Down,EKeys::Tab,EKeys::Gamepad_DPad_Down})InputComponent->BindKey(NavKey,IE_Pressed,this,&AEnigmaCampaignController::NoOp).KeyDelegate.GetDelegateForManualSet().BindLambda([Get](){if(Get())Get()->MenuNavigate(1);});
 for(const FKey& AcceptKey:{EKeys::Enter,EKeys::Gamepad_FaceButton_Bottom})InputComponent->BindKey(AcceptKey,IE_Pressed,this,&AEnigmaCampaignController::NoOp).KeyDelegate.GetDelegateForManualSet().BindLambda([Get](){if(Get())Get()->MenuAccept();});
 InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right,IE_Pressed,this,&AEnigmaCampaignController::NoOp).KeyDelegate.GetDelegateForManualSet().BindLambda([Get](){if(Get())Get()->MenuBack();});
 const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine};
 for(int32 I=0;I<9;I++)InputComponent->BindKey(Keys[I],IE_Pressed,this,&AEnigmaCampaignController::NoOp).KeyDelegate.GetDelegateForManualSet().BindLambda([Get,I](){if(Get())Get()->Key(I);});
}
void AEnigmaCampaignHUD::Text(const FString& V,float X,float Y,FLinearColor C,float Z){
 HomeX=ScreenX;HomeY=ScreenY;MenuText(V,X,Y,14*Z,C,Z>=1.3f);
}
void AEnigmaCampaignHUD::Box(float X,float Y,float W,float H,FLinearColor C){DrawRect(C,ScreenX+X*Scale,ScreenY+Y*Scale,W*Scale,H*Scale);}
void AEnigmaCampaignHUD::Button(const FString& Label,float X,float Y,float W,const FString& Name,const FString& Id,int32 Number,bool Enabled,float H){
 const bool Battle=Mode->Page==TEXT("battle");
 const int32 Index=Mode->Buttons.Num();float MX=0,MY=0;auto PC=GetOwningPlayerController();
 const FVector2D Min(ScreenX+X*Scale,ScreenY+Y*Scale),Size(W*Scale,H*Scale);
 const bool Over=PC&&PC->GetMousePosition(MX,MY)&&MX>=Min.X&&MY>=Min.Y&&MX<Min.X+Size.X&&MY<Min.Y+Size.Y;
 if(!Battle&&Enabled&&Over&&FVector2D::DistSquared(LastHomeMouse,FVector2D(MX,MY))>1)Mode->HomeFocus=Index;
 bool Selected=(Name==TEXT("select")&&Id==Mode->Selected)||(Name==TEXT("slot")&&Number==Mode->TeamSlot)||(Name==TEXT("region")&&Number==Mode->Region)||(Name==TEXT("target")&&Id==Mode->Target);
 const bool Focus=Enabled&&(Battle?Over:Index==Mode->HomeFocus);
 float& A=HomeHover.FindOrAdd(Name+TEXT("/")+Id+FString::FromInt(Number)+FString::FromInt(Index));
 A=Mode->ReducedMotion?(Focus?1.f:0.f):FMath::FInterpTo(A,Focus?1.f:0.f,FMath::Min(GetWorld()->GetDeltaSeconds(),.05f),12.f);
 Box(X,Y,W,H,Enabled?FLinearColor(.022f+.035f*A,.043f+.035f*A,.052f+.028f*A,.92f):FLinearColor(.013f,.023f,.03f,.86f));
 const FLinearColor Edge=Enabled?FLinearColor(.83f,.62f,.32f,Selected?.85f:.23f+.6f*A):FLinearColor(.19f,.25f,.27f,.4f);
 Box(X,Y,W,1,Edge);Box(X,Y+H-1,W,1,FLinearColor(Edge.R,Edge.G,Edge.B,Edge.A*.5f));
 if(Selected||Focus)Box(X,Y,2,H,Edge);
 float FontSize=Battle?12.5f:13.f;
 const float MaxWidth=W-26;
 FString Visible=Label;
 const float OriginalWidth=MenuTextWidth(Label,FontSize);
 if(OriginalWidth>MaxWidth)FontSize=FMath::Max(11.f,FontSize*MaxWidth/OriginalWidth);
 while(!Visible.IsEmpty()&&MenuTextWidth(Visible,FontSize)>MaxWidth)Visible.LeftChopInline(1);
 if(Visible.Len()<Label.Len()&&Visible.Len()>3)Visible=Visible.LeftChop(3)+TEXT("...");
 Text(Visible,X+12,Y+(H-FontSize)*.5f-1,Enabled?(Selected||Focus?FLinearColor(1,.84f,.52f):FLinearColor(.78f,.83f,.81f)):FLinearColor(.4f,.47f,.49f),FontSize/14.f);
 Mode->Buttons.Add({Min,Size,Name,Id,Number,Enabled});
}
void AEnigmaCampaignHUD::Wrap(const FString& V,float X,float Y,int32 Width,int32 MaxLines){
 TArray<FString> Words;V.Replace(TEXT("\n"),TEXT(" \n ")).ParseIntoArray(Words,TEXT(" "),true);
 FString Line;int32 Row=0;const float MaxWidth=Width*6.8f;
 for(const auto& Word:Words){
  if(Word==TEXT("\n")||(!Line.IsEmpty()&&MenuTextWidth(Line+Word,13.3f)>MaxWidth)){
   if(Row==MaxLines-1){
    Line.TrimEndInline();while(!Line.IsEmpty()&&MenuTextWidth(Line+TEXT("..."),13.3f)>MaxWidth)Line.LeftChopInline(1);
    Text(Line+TEXT("..."),X,Y+Row*23,FLinearColor(.74f,.81f,.81f),.95f);return;
   }
   Text(Line,X,Y+Row*23,FLinearColor(.74f,.81f,.81f),.95f);Line.Reset();++Row;
  }
  if(Word!=TEXT("\n"))Line+=Word+TEXT(" ");
 }
 if(Row<MaxLines)Text(Line,X,Y+Row*23,FLinearColor(.74f,.81f,.81f),.95f);
}
UTexture2D* AEnigmaCampaignHUD::Texture(const FString& Name){if(auto Found=Art.Find(Name))return *Found;const FString Path=TEXT("/Game/GameArt/")+Name+TEXT(".")+Name;auto Value=LoadObject<UTexture2D>(nullptr,*Path);Art.Add(Name,Value);return Value;}
void AEnigmaCampaignHUD::DrawHUD(){
 Super::DrawHUD();Mode=Cast<AEnigmaCampaignMode>(GetWorld()->GetAuthGameMode());if(!Mode)return;Scale=FMath::Min(Canvas->ClipX/1280.f,Canvas->ClipY/720.f);ScreenX=(Canvas->ClipX-1280*Scale)*.5f;ScreenY=(Canvas->ClipY-720*Scale)*.5f;Mode->Buttons.Reset();
 auto Game=Mode->Game();if(!Game||!Game->State){Box(0,0,1280,720,FLinearColor(.02f,.01f,.04f));Wrap(Mode->Message,50,140,110,15);return;}
 auto State=Mode->DisplayState();auto Save=O(State,TEXT("save"));auto Catalog=O(State,TEXT("catalog"));auto Inventory=O(Save,TEXT("Inventory"));
 auto HeroName=[&](const FString& Id){const FString Name=S(Find(A(Catalog,TEXT("Characters")),Id),TEXT("Name"));return Name.IsEmpty()?Id:Name;};
 const FString Page=Mode->Page;
 if(Page==TEXT("home")||Page==TEXT("extras")){DrawHome();return;}
 if(Page!=TEXT("battle"))DrawMenuFrame(Page);
 Text(TEXT("REALITY OF ENIGMA"),40,24,FLinearColor(.83f,.62f,.32f),1.5f);
 Text(FString::Printf(TEXT("OURO  %.0f     XP  %.0f     TICKETS  %.0f     JORNADA  %.0f/70"),N(Inventory,TEXT("Gold")),N(Inventory,TEXT("Xp")),N(Inventory,TEXT("Tickets")),N(Save,TEXT("CompletedStage"))),615,30,FLinearColor(.72f,.77f,.73f),.85f);
 if(Page!=TEXT("home")&&Page!=TEXT("battle"))Button(TEXT("Início"),1120,65,120,TEXT("page"),TEXT("home"));
 if(Page==TEXT("campaign")||Page==TEXT("ending")){
  auto Regions=A(State,TEXT("regions"));int32 Max=FMath::Clamp(int32(N(Save,TEXT("CompletedStage")))/10,0,6);Mode->Region=FMath::Clamp(Mode->Region,0,Max);auto R=Regions[Mode->Region]->AsObject();
  Text(S(R,TEXT("Name")),40,100,FLinearColor(.95f,.8f,.55f),1.6f);Wrap(S(R,TEXT("Summary")),40,147,120,3);
  if(Page==TEXT("ending")){Wrap(S(R,TEXT("Ending")),50,210,110,15);Button(TEXT("Retornar ao atlas"),45,605,250,TEXT("page"),TEXT("campaign"));if(Mode->Region<6&&N(Save,TEXT("CompletedStage"))>=(Mode->Region+1)*10)Button(TEXT("Explorar próxima região"),325,605,250,TEXT("nextRegion"));}
  else {
   for(int32 I=0;I<10;I++){int32 Stage=Mode->Region*10+I+1;int32 Done=N(Save,TEXT("CompletedStage"));bool Allowed=Stage<=Done+1&&(Stage>Done||Stage%10!=0);FString L=FString::Printf(TEXT("%02d  ·  %s"),Stage,Stage%10==0?TEXT("Confronto final"):Stage<=Done?TEXT("Concluída"):Stage==Done+1?TEXT("Próxima travessia"):TEXT("Não explorada"));Button(L,45+(I%5)*235,260+(I/5)*95,220,TEXT("start"),TEXT("campaign"),Stage,Allowed,65);}
   if(N(Save,TEXT("CompletedStage"))>=(Mode->Region+1)*10)Button(TEXT("Crônicas da região"),45,475,260,TEXT("page"),TEXT("ending"));
   for(int32 I=0;I<7;I++)Button(FString::Printf(TEXT("Região %d"),I+1),45+I*170,550,155,TEXT("region"),TEXT(""),I,I<=Max);
  }
 }
 else if(Page==TEXT("opening")){
  Text(FString::Printf(TEXT("Prólogo · %d / 8"),Mode->OpeningShot+1),45,118,FLinearColor(.95f,.77f,.5f),1.6f);
  auto Lines=A(State,TEXT("opening"));if(auto T=Texture(Mode->OpeningShot<4?TEXT("OpeningAtlas"):TEXT("EncounterAtlas")))DrawTexture(T,ScreenX+45*Scale,ScreenY+155*Scale,1190*Scale,330*Scale,(Mode->OpeningShot%2)*.5f,((Mode->OpeningShot%4)/2)*.5f,.5f,.5f);
  Box(45,421,1190,115,FLinearColor(.008f,.022f,.03f,.94f));
  if(Lines.IsValidIndex(Mode->OpeningShot))Wrap(Lines[Mode->OpeningShot]->AsString(),65,433,120,4);
  Button(TEXT("Anterior"),45,565,200,TEXT("openingPrevious"));Button(TEXT("Próxima cena"),270,565,230,TEXT("openingNext"));Button(TEXT("Pular prólogo"),525,565,250,TEXT("openingSkip"));Button(Mode->OpeningAuto?TEXT("Pausar"):TEXT("Continuar"),800,565,250,TEXT("openingPause"));
 }
 else if(Page==TEXT("team")||Page==TEXT("cards")){
  Text(Page==TEXT("team")?TEXT("Heróis da jornada"):TEXT("Compêndio"),45,107,FLinearColor(.95f,.77f,.5f),1.4f);
  TArray<TSharedPtr<FJsonValue>> Entries;if(Page==TEXT("team"))Entries=A(Save,TEXT("Roster"));else for(auto C:A(Catalog,TEXT("Characters")))if(!S(C->AsObject(),TEXT("Id")).StartsWith(TEXT("enemy"))&&S(C->AsObject(),TEXT("Role"))!=TEXT("Summon"))Entries.Add(C);
  int32 Pages=FMath::Max(1,(Entries.Num()+7)/8);Mode->ListPage=FMath::Clamp(Mode->ListPage,0,Pages-1);
  for(int32 I=Mode->ListPage*8;I<FMath::Min(Entries.Num(),Mode->ListPage*8+8);I++){
   auto E=Entries[I]->AsObject();FString Id=S(E,TEXT("Id"));auto Def=Find(A(Catalog,TEXT("Characters")),Id);FString Label=S(Def,TEXT("Name"));if(Page==TEXT("team"))Label+=FString::Printf(TEXT(" • Nv.%.0f • %.0f estrelas"),N(E,TEXT("Level")),N(E,TEXT("Stars")));Button(Label,45,165+(I%8)*46,310,TEXT("select"),Id);
  }
  Button(TEXT("Anterior"),45,549,145,TEXT("list"),TEXT(""),-1,Mode->ListPage>0);Button(TEXT("Próxima"),210,549,145,TEXT("list"),TEXT(""),1,Mode->ListPage+1<Pages);
  auto Hero=Find(A(Catalog,TEXT("Characters")),Mode->Selected);auto Stats=O(Hero,TEXT("Stats"));auto Owned=Find(A(Save,TEXT("Roster")),Mode->Selected);float Growth=1+.03f*(FMath::Max(1,int32(N(Owned,TEXT("Level"))))-1);
  Text(S(Hero,TEXT("Name")),395,165,FLinearColor(.95f,.77f,.55f),1.8f);Text(S(Hero,TEXT("Role")),395,207,FLinearColor(.75f,.75f,.85f));
  Text(FString::Printf(TEXT("Vida %.0f   Força %.0f   Essência %.0f   Defesa %.0f   Agilidade %.0f"),N(Stats,TEXT("Hp"))*Growth,N(Stats,TEXT("Strength"))*Growth,N(Stats,TEXT("Essence"))*Growth,N(Stats,TEXT("Defense"))*Growth,N(Stats,TEXT("Speed"))),395,239,FLinearColor(.7f,.78f,.79f),.88f);
  if(Page==TEXT("team")){
   Button(TEXT("Aprimorar nível"),395,280,180,TEXT("level"),Mode->Selected);Button(TEXT("Ascender estrela"),590,280,180,TEXT("promote"),Mode->Selected);Button(TEXT("Reconstrução"),785,280,180,TEXT("reconstruct"),Mode->Selected);
   for(int32 I=0;I<5;I++)Button(FString::Printf(TEXT("Posição %d: %s"),I+1,*HeroName(A(Save,TEXT("Team"))[I]->AsString())),395+(I%3)*255,340+(I/3)*46,240,TEXT("slot"),TEXT(""),I);
   Button(TEXT("Designar herói à posição"),395,442,360,TEXT("team"),Mode->Selected,Mode->TeamSlot);Button(TEXT("Organizar formação"),785,442,280,TEXT("page"),TEXT("formation"));
   Button(TEXT("Kael · Caminho marcial"),395,502,310,TEXT("path"),TEXT("martial"));Button(TEXT("Kael · Arsenal"),735,502,260,TEXT("path"),TEXT("arsenal"));
   Text(FString::Printf(TEXT("Posição %d selecionada · Ascensão requer nível máximo e uma cópia ou Coringa."),Mode->TeamSlot+1),395,564,FLinearColor(.63f,.72f,.74f),.85f);
  } else {
   const TSet<FString> PortraitIds={TEXT("kael"),TEXT("savor"),TEXT("aelia"),TEXT("brakk"),TEXT("lyra"),TEXT("raizen"),TEXT("varkas"),TEXT("sevrin"),TEXT("mazakin"),TEXT("hajun"),TEXT("zhujin"),TEXT("tenrin"),TEXT("zevalon")};if(PortraitIds.Contains(Mode->Selected)){if(auto T=Texture(TEXT("Hero_")+Mode->Selected))DrawTexture(T,ScreenX+965*Scale,ScreenY+280*Scale,250*Scale,300*Scale,Mode->Selected==TEXT("varkas")||Mode->Selected==TEXT("sevrin")?0:.55f,0,Mode->Selected==TEXT("varkas")||Mode->Selected==TEXT("sevrin")?1:.45f,1,FLinearColor(.65f,.65f,.65f,1));}
   const TSet<FString> NewIds={TEXT("mazakin"),TEXT("hajun"),TEXT("zhujin"),TEXT("tenrin"),TEXT("zevalon")};
   if(NewIds.Contains(Mode->Selected)){if(auto T=Texture(TEXT("Hero_")+Mode->Selected))DrawTexture(T,ScreenX+965*Scale,ScreenY+280*Scale,250*Scale,300*Scale,.16f,.035f,.33f,.5f,FLinearColor::White);Button(TEXT("Abrir galeria"),970,600,245,TEXT("page"),TEXT("reference"));}
   auto Stories=O(State,TEXT("stories"));auto Story=O(Stories,*Mode->Selected);Wrap(S(Story,TEXT("Title"))+TEXT(" • ")+S(Story,TEXT("Affiliation"))+TEXT("\n")+S(Story,TEXT("History")),395,284,70,6);
   auto Abilities=A(Hero,TEXT("Skills"));for(int32 I=0;I<Abilities.Num()&&I<7;I++){auto Skill=Find(A(Catalog,TEXT("Skills")),Abilities[I]->AsString());Text(S(Skill,TEXT("Name"))+(N(Skill,TEXT("Cooldown"))==0?TEXT(" · Sem recarga"):FString::Printf(TEXT(" · Recarga: %.0f rodadas"),N(Skill,TEXT("Cooldown")))),395,442+I*23,FLinearColor(.87f,.8f,.67f),.92f);}
   if(Mode->Selected==TEXT("varkas")||Mode->Selected==TEXT("sevrin")||NewIds.Contains(Mode->Selected))Button(TEXT("Entrar na arena"),395,600,260,TEXT("start"),Mode->Selected);
  }
 }
 else if(Page==TEXT("reference")){
  Text(S(Find(A(Catalog,TEXT("Characters")),Mode->Selected),TEXT("Name"))+TEXT(" · Galeria"),45,100,FLinearColor(.95f,.77f,.5f),1.2f);
  const bool HasPhases=Mode->Selected==TEXT("hajun")||Mode->Selected==TEXT("zhujin");
  FString ArtName=TEXT("Hero_")+Mode->Selected;
  if(HasPhases&&Mode->ReferencePhase>0)ArtName+=FString::Printf(TEXT("_phase%d"),Mode->ReferencePhase);
  if(HasPhases){Button(TEXT("Arte conceitual"),45,155,185,TEXT("referencePhase"),TEXT(""),0);Button(TEXT("Fase 1"),45,205,185,TEXT("referencePhase"),TEXT(""),1);Button(TEXT("Fase 2"),45,255,185,TEXT("referencePhase"),TEXT(""),2);Text(Mode->ReferencePhase?FString::Printf(TEXT("FASE %d"),Mode->ReferencePhase):TEXT("ARTE CONCEITUAL"),45,315,FLinearColor(.95f,.77f,.5f));}
  Box(250,140,770,440,FLinearColor(.025f,.025f,.035f,.98f));
  if(Mode->Selected==TEXT("hajun")){Button(TEXT("Modelo 3D • fase 1"),45,365,185,TEXT("reference3D"));}
  if(Mode->Reference3D&&Mode->ModelPreview){DrawTexture(Mode->ModelPreview->Texture(),ScreenX+422.5f*Scale,ScreenY+150*Scale,425*Scale,425*Scale,0,0,1,1,FLinearColor::White,BLEND_Opaque);Button(TEXT("Girar à esquerda"),45,420,185,TEXT("previewRotate"),TEXT(""),-45);Button(TEXT("Girar à direita"),45,470,185,TEXT("previewRotate"),TEXT(""),45);Text(Mode->ModelPreview->ShowingMotion()?TEXT("TESTE DE ATAQUE • MODELO TRIPO"):TEXT("MODELO 3D • SEM ANIMAÇÃO"),270,558,FLinearColor(.95f,.77f,.5f));}
  else if(auto T=Texture(ArtName)){float Fit=FMath::Min(750.f/T->GetSizeX(),425.f/T->GetSizeY());float W=T->GetSizeX()*Fit,H=T->GetSizeY()*Fit;DrawTexture(T,ScreenX+(635-W*.5f)*Scale,ScreenY+(362.5f-H*.5f)*Scale,W*Scale,H*Scale,0,0,1,1,FLinearColor::White);}
  else Text(TEXT("Arte indisponível"),390,330,FLinearColor(.9f,.3f,.3f));
  Button(TEXT("Retornar ao compêndio"),45,590,185,TEXT("page"),TEXT("cards"));
  if(Mode->Selected==TEXT("hajun")){const TCHAR* Labels[]={TEXT("Repouso"),TEXT("Ataque 1"),TEXT("Ataque 2"),TEXT("Combo"),TEXT("Carregado")};for(int32 I=0;I<5;I++)Button(Labels[I],270+I*145,590,135,TEXT("modelMotion"),TEXT(""),I);}
 }
 else if(Page==TEXT("formation")){
  Text(TEXT("Formação de batalha"),45,118,FLinearColor(.95f,.8f,.5f),1.3f);
  auto Team=A(Save,TEXT("Team"));auto Cells=A(Save,TEXT("FormationCells"));
  for(int32 I=0;I<Team.Num();I++)Button(FString::Printf(TEXT("%d • %s"),I+1,*HeroName(Team[I]->AsString())),45,180+I*55,320,TEXT("slot"),TEXT(""),I);
  for(int32 C=0;C<9;C++){FString Label=FString::Printf(TEXT("Posição livre %d"),C+1);for(int32 I=0;I<Cells.Num();I++)if(int32(Cells[I]->AsNumber())==C)Label=HeroName(Team[I]->AsString());Button(Label,430+(C%3)*220,185+(C/3)*105,200,TEXT("cell"),TEXT(""),C,true,85);}
  Button(TEXT("Retornar aos heróis"),45,540,320,TEXT("page"),TEXT("team"));
 }
 else if(Page==TEXT("summon")){
  Text(TEXT("Invocação"),45,118,FLinearColor(.95f,.8f,.5f),1.4f);auto Banner=O(Catalog,TEXT("Summon"));
  Button(FString::Printf(TEXT("Invocar um aliado · %.0f de ouro"),N(Banner,TEXT("SingleCost"))),45,205,430,TEXT("summon"),TEXT(""),1);Button(FString::Printf(TEXT("Invocar dez aliados · %.0f de ouro"),N(Banner,TEXT("TenCost"))),45,267,430,TEXT("summon"),TEXT(""),10);Button(TEXT("Resgatar recompensas de ausência"),45,350,430,TEXT("collect"));
  Wrap(TEXT("Atravesse o limiar e encontre novos aliados. Duplicatas concedem cópias para evolução; heróis com seis estrelas concedem Coringas. Reúna fragmentos no treinamento para recrutar chefes."),540,205,72,9);
 }
 else if(Page==TEXT("training")){
  Text(TEXT("Treinamento"),45,118,FLinearColor(.95f,.8f,.5f),1.4f);auto Bosses=A(Catalog,TEXT("Bosses"));auto Unlocked=A(Save,TEXT("Training"));int32 Row=0;
  for(auto V:Bosses){auto Boss=V->AsObject();if(!B(Boss,TEXT("Recruitable")))continue;FString Id=S(Boss,TEXT("Id"));bool Enabled=false;for(auto U:Unlocked)Enabled|=U->AsString()==Id;Button(S(Find(A(Catalog,TEXT("Characters")),S(Boss,TEXT("CharacterId"))),TEXT("Name")),45,180+Row*66,470,TEXT("train"),Id,0,Enabled);Button(TEXT("Simular encontro · 1 ticket"),540,180+Row*66,270,TEXT("simulate"),Id,0,Enabled);Row++;}
  Wrap(TEXT("Os marcos da campanha liberam cada chefe. HP e estados persistem entre as ondas. A recompensa só é concedida após a terceira onda e apenas uma vez por encontro."),45,510,110,4);
 }
 else if(Page==TEXT("arenas")){
  const FString Labels[]={TEXT("Primeiro encontro"),TEXT("Encontro do elenco"),TEXT("Fenrath"),TEXT("Sevrin: sangramento"),TEXT("Velmora: veneno e flores"),TEXT("Maltherion: ascensão"),TEXT("Kael · Oitavo estágio"),TEXT("Akuroth · Fim das Realidades"),TEXT("Mazakin: Submundo"),TEXT("Hajun: Carnificina"),TEXT("Zhu Jin: Flor do Veneno"),TEXT("Tenrin: Iluminação"),TEXT("Zevalon: Senhor dos Céus")};
  const FString Ids[]={TEXT("demo"),TEXT("cast"),TEXT("varkas"),TEXT("sevrin"),TEXT("floral"),TEXT("eclipse"),TEXT("sandbox"),TEXT("akuroth"),TEXT("mazakin"),TEXT("hajun"),TEXT("zhujin"),TEXT("tenrin"),TEXT("zevalon")};
  for(int32 I=0;I<UE_ARRAY_COUNT(Ids);I++)Button(Labels[I],45+(I%3)*405,160+(I/3)*67,380,TEXT("start"),Ids[I]);
  Button(TEXT("Akuroth · Arena experimental"),45,530,650,TEXT("legacyBoss"));Button(TEXT("Rever prólogo"),735,530,350,TEXT("page"),TEXT("opening"));
 }
 else if(Page==TEXT("settings")){
  Text(TEXT("Configurações"),45,118,FLinearColor(.95f,.8f,.5f),1.4f);Text(FString::Printf(TEXT("Efeitos sonoros · %.0f%%"),Mode->Volume*100),45,210);
  Button(TEXT("Diminuir volume"),45,255,200,TEXT("volume"),TEXT(""),-1);Button(TEXT("Aumentar volume"),270,255,200,TEXT("volume"),TEXT(""),1);Button(TEXT("Modo de exibição · Alternar tela cheia"),45,335,425,TEXT("fullscreen"));Button(Mode->ReducedMotion?TEXT("Movimento reduzido · Ativado"):TEXT("Movimento reduzido · Desativado"),45,405,425,TEXT("motion"));
 }
 else if(Page==TEXT("battle")){
  auto Battle=O(State,TEXT("battle"));Box(0,62,1280,85,FLinearColor(.008f,.022f,.03f,.94f));Text(FString::Printf(TEXT("Rodada %.0f • Onda %.0f"),N(Battle,TEXT("round")),N(Battle,TEXT("wave"))),25,74,FLinearColor(.95f,.8f,.6f));
  FString Turns=TEXT("Ordem: ");for(auto T:A(Battle,TEXT("turns")))Turns+=T->AsString()+TEXT("   ›   ");Text(Turns.Left(135),25,118,FLinearColor(.75f,.75f,.85f),.85f);
  Button(TEXT("Início"),1120,65,120,TEXT("page"),TEXT("home"));
  auto Allies=A(Battle,TEXT("allies"));auto Enemies=A(Battle,TEXT("enemies"));
  for(auto E:Enemies){auto U=E->AsObject();FString Id=S(U,TEXT("id"));if(Id==TEXT("akuroth")||Id==TEXT("mazakin")||Id==TEXT("hajun")||Id==TEXT("zhujin")){FString Title=Id==TEXT("akuroth")?(B(U,TEXT("ascended"))?TEXT("AKUROTH • Soberano do Absoluto"):TEXT("AKUROTH • O Portador do Vazio")):Id==TEXT("mazakin")?(B(U,TEXT("ascended"))?TEXT("MAZAKIN • Rei do Abismo"):TEXT("MAZAKIN • Senhor do Submundo")):Id==TEXT("hajun")?(B(U,TEXT("ascended"))?TEXT("HAJUN • Ascensão Absoluta"):TEXT("HAJUN • Carrasco do Submundo")):(B(U,TEXT("blooming"))?TEXT("ZHU JIN • Florescimento"):B(U,TEXT("crimson"))?TEXT("ZHU JIN • Flor Carmesim"):TEXT("ZHU JIN • Flor do Veneno"));Text(Title,380,73,FLinearColor(.95f,.72f,.45f),1.1f);Box(380,100,535,7,FLinearColor(.15f,.01f,.025f));Box(380,100,535*float(N(U,TEXT("hp"))/N(U,TEXT("maxHp"))),7,FLinearColor(.8f,.03f,.08f));}}
  for(int32 Side=0;Side<2;Side++){auto Units=Side==0?Allies:Enemies;for(int32 I=0;I<Units.Num()&&I<9;I++){auto U=Units[I]->AsObject();float X=Side?940:20,Y=165+I*40;FString Key=S(U,TEXT("key"));Button(((S(U,TEXT("id"))==TEXT("akuroth")||S(U,TEXT("id"))==TEXT("mazakin")||S(U,TEXT("id"))==TEXT("hajun")||S(U,TEXT("id"))==TEXT("tenrin")||S(U,TEXT("id"))==TEXT("zevalon"))?S(U,TEXT("id")).ToUpper():S(U,TEXT("id"))==TEXT("zhujin")?TEXT("ZHU JIN"):S(U,TEXT("name")))+FString::Printf(TEXT(" %.0f/%.0f"),N(U,TEXT("hp")),N(U,TEXT("maxHp"))),X,Y,320,TEXT("target"),Key,0,B(U,TEXT("alive")),34);Box(X,Y+32,320*float(N(U,TEXT("hp"))/FMath::Max(1.0,N(U,TEXT("maxHp")))),2,Key==Mode->Target?FLinearColor(.95f,.75f,.25f):Side?FLinearColor(.75f,.04f,.1f):FLinearColor(.1f,.7f,.4f));}}
  Box(0,550,1280,170,FLinearColor(.008f,.022f,.03f,.96f));
  if(!B(Battle,TEXT("locked"))){Button(TEXT("Iniciar combate"),30,565,220,TEXT("begin"));for(int32 I=0;I<9;I++)Button(FString::Printf(TEXT("Casa %d"),I+1),390+(I%3)*155,220+(I/3)*57,145,TEXT("battleCell"),TEXT(""),I);}
  else Button(TEXT("Próxima ação"),30,565,220,TEXT("act"));
  Button(Mode->Auto?TEXT("Parar automático"):TEXT("Automático"),275,565,220,TEXT("auto"));Button(TEXT("Registro"),520,565,150,TEXT("log"));Button(Mode->Repeat?TEXT("Repetir treino: sim"):TEXT("Repetir treino: não"),690,565,260,TEXT("repeat"));
  if(B(Battle,TEXT("locked"))&&B(Battle,TEXT("allyTurn"))&&!B(Battle,TEXT("finished")))Button(Mode->MoveMenu?TEXT("Cancelar movimento"):TEXT("Reposicionar"),980,565,250,TEXT("moveMenu"),TEXT(""),0,Mode->AnimationAge<0);
  if(Mode->MoveMenu){Box(370,175,490,200,FLinearColor(.01f,.01f,.025f,.95f));for(int32 Cell=0;Cell<9;Cell++){bool Free=true;for(auto V:Allies){auto U=V->AsObject();Free&=!(B(U,TEXT("alive"))&&int32(N(U,TEXT("layer")))*3+int32(N(U,TEXT("column")))==Cell);}Button(FString::Printf(TEXT("Casa %d"),Cell+1),385+(Cell%3)*155,195+(Cell/3)*52,145,TEXT("reposition"),TEXT(""),Cell,Free,42);}}
  auto Actor=Find(Allies,S(Battle,TEXT("actor")),TEXT("key"));if(!S(Actor,TEXT("key")).IsEmpty()){FString Detail;if(S(Actor,TEXT("id"))==TEXT("zevalon"))Detail=FString::Printf(TEXT("Energia celestial: %.0f/5"),N(Actor,TEXT("energy")));if(S(Actor,TEXT("id"))==TEXT("tenrin"))Detail=FString::Printf(TEXT("Aura do Leão: %.0f/5"),N(Actor,TEXT("divineFocus")));if(!Detail.IsEmpty())Text(Detail,360,480,FLinearColor(.98f,.78f,.35f),1.1f);}
  auto Skills=A(Battle,TEXT("skills"));bool Allowed=B(Battle,TEXT("locked"))&&B(Battle,TEXT("allyTurn"))&&!B(Battle,TEXT("finished"))&&Mode->AnimationAge<0;
  for(int32 I=0;I<Skills.Num()&&I<9;I++){auto Ability=Skills[I]->AsObject();Button(FString::Printf(TEXT("%d • %s"),I+1,*S(Ability,TEXT("name"))),30+(I%5)*245,613+(I/5)*43,230,TEXT("act"),S(Ability,TEXT("id")),0,Allowed&&S(Ability,TEXT("reason")).IsEmpty());}
  if(B(Battle,TEXT("finished"))){Text(B(Battle,TEXT("won"))?TEXT("VITÓRIA"):TEXT("DERROTA"),500,170,B(Battle,TEXT("won"))?FLinearColor(.95f,.8f,.4f):FLinearColor(.9f,.2f,.2f),2);if(S(State,TEXT("mode"))==TEXT("campaign")){int32 Stage=N(State,TEXT("stage"));if(B(Battle,TEXT("won"))&&Stage<70)Button(TEXT("Próxima fase"),460,232,330,TEXT("start"),TEXT("campaign"),Stage+1);if(Stage%10==0&&B(Battle,TEXT("won"))){Mode->Region=(Stage-1)/10;Button(TEXT("Crônicas da região"),460,279,330,TEXT("page"),TEXT("ending"));}}}
  if(Mode->LogVisible){Box(350,320,580,210,FLinearColor(.015f,.01f,.025f,.96f));auto Logs=A(Battle,TEXT("log"));for(int32 I=0;I<Logs.Num()&&I<9;I++)Text(Logs[I]->AsString().Left(78),362,330+I*22,FLinearColor(.85f,.78f,.65f),.85f);}
  if(!Mode->Target.IsEmpty()){auto U=Find(Allies,Mode->Target,TEXT("key"));if(S(U,TEXT("key")).IsEmpty())U=Find(Enemies,Mode->Target,TEXT("key"));FString Status=TEXT("Alvo: ")+S(U,TEXT("name"));for(auto V:A(U,TEXT("statuses")))Status+=TEXT(" • ")+S(V->AsObject(),TEXT("id"))+FString::Printf(TEXT(" (%.0f)"),N(V->AsObject(),TEXT("turns")));Text(Status.Left(100),360,505,FLinearColor(.9f,.8f,.65f),.9f);}
 }
 const FString Notice=Mode->Message.IsEmpty()?S(Game->State,TEXT("notice")):Mode->Message;
 if(Page!=TEXT("battle")){if(Notice!=TEXT("Jornada carregada."))Wrap(Notice,45,651,96,2);float MX=0,MY=0;if(GetOwningPlayerController()&&GetOwningPlayerController()->GetMousePosition(MX,MY))LastHomeMouse=FVector2D(MX,MY);}else Text(Notice.Replace(TEXT("\n"),TEXT(" • ")).Left(145),25,145,FLinearColor(.95f,.72f,.4f),.8f);
}
void AEnigmaCampaignMode::RunChecks(){
 int32 Checks=0,Failures=0;auto Check=[&](bool Ok,const TCHAR* Name){Checks++;if(!Ok){Failures++;UE_LOG(LogTemp,Error,TEXT("MIGRATION_QA_FAIL %s: %s"),Name,*Game()->Error);}};
 auto G=Game();Check(G&&G->State.IsValid(),TEXT("native_library_loaded"));if(!G||!G->State){FPlatformMisc::RequestExitWithStatus(false,1);return;}
 for(const TCHAR* ArtName:{TEXT("Hero_hajun_phase1"),TEXT("Hero_hajun_phase2"),TEXT("Hero_zhujin_phase1"),TEXT("Hero_zhujin_phase2")}){FString Name(ArtName);Check(LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/GameArt/")+Name+TEXT(".")+Name))!=nullptr,ArtName);}
 auto Preview=GetWorld()->SpawnActor<AEnigmaModelPreview>();Check(Preview&&Preview->Ready(),TEXT("hajun_static_model_and_material"));Check(Preview&&Preview->MotionReady(),TEXT("hajun_rig_and_retargeted_clips"));if(Preview)Preview->Destroy();
 Check(A(O(G->State,TEXT("catalog")),TEXT("Stages")).Num()==70,TEXT("seventy_stages"));Check(A(G->State,TEXT("regions")).Num()==7,TEXT("seven_regions"));Check(A(G->State,TEXT("opening")).Num()==8,TEXT("eight_opening_shots"));
 Check(A(O(G->State,TEXT("save")),TEXT("Roster")).Num()==5,TEXT("starting_roster"));Check(G->Command(TEXT("level"),TEXT("kael")),TEXT("level_call"));Check(N(Find(A(O(G->State,TEXT("save")),TEXT("Roster")),TEXT("kael")),TEXT("Level"))==2,TEXT("level_rule"));
 const double Gold=N(O(O(G->State,TEXT("save")),TEXT("Inventory")),TEXT("Gold"));Check(G->Command(TEXT("summon"),TEXT(""),10),TEXT("summon_ten"));Check(N(O(O(G->State,TEXT("save")),TEXT("Inventory")),TEXT("Gold"))==Gold-9000,TEXT("summon_cost"));
 Check(!G->Command(TEXT("start"),TEXT("campaign"),70),TEXT("locked_campaign"));Check(G->Command(TEXT("start"),TEXT("campaign"),1),TEXT("campaign_entry"));Check(A(O(G->State,TEXT("battle")),TEXT("allies")).Num()==5,TEXT("five_individual_allies"));
 Check(G->Command(TEXT("begin")),TEXT("formation_lock"));Check(B(O(G->State,TEXT("battle")),TEXT("locked")),TEXT("locked_flag"));Check(G->Command(TEXT("act")),TEXT("campaign_action"));
 for(auto Kind:{TEXT("demo"),TEXT("cast"),TEXT("varkas"),TEXT("sevrin"),TEXT("floral"),TEXT("eclipse"),TEXT("sandbox"),TEXT("akuroth"),TEXT("mazakin"),TEXT("hajun"),TEXT("zhujin"),TEXT("tenrin"),TEXT("zevalon")}){
  Check(G->Command(TEXT("start"),Kind),Kind);WorldArt->ShowBattle(O(G->State,TEXT("battle")),6);Check(G->Command(TEXT("begin")),TEXT("begin_arena"));int32 I=0;for(;I<1000&&!B(O(G->State,TEXT("battle")),TEXT("finished"));I++)if(!G->Command(TEXT("act")))break;
  Check(B(O(G->State,TEXT("battle")),TEXT("finished")),TEXT("arena_finishes"));Check(I<1000,TEXT("bounded_turns"));
 }
 Check(G->Command(TEXT("save")),TEXT("save_roundtrip_write"));Check(WorldArt&&WorldArt->Ready,TEXT("skeletal_assets_loaded"));
 UE_LOG(LogTemp,Display,TEXT("ENIGMA_MIGRATION_QA_RESULT checks=%d failures=%d"),Checks,Failures);FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
}




void AEnigmaCampaignMode::CaptureUI(const FString& Name){
 if(!UITourFolder.IsEmpty())UITourCaptures++;
 for(auto& Button:Buttons){int32 W,H;UGameplayStatics::GetPlayerController(this,0)->GetViewportSize(W,H);if(Button.Min.X<0||Button.Min.Y<0||Button.Min.X+Button.Size.X>W+.5f||Button.Min.Y+Button.Size.Y>H+.5f){UIWarnings++;UE_LOG(LogTemp,Error,TEXT("UI_QA_BOUNDS %s %s"),*Name,*Button.Name);}}
 const FString File=FPaths::ConvertRelativePathToFull((CombatReplayFolder.IsEmpty()?UITourFolder:CombatReplayFolder/TEXT("captures"))/(Name+TEXT(".png")));FScreenshotRequest::RequestScreenshot(File,true,false);UE_LOG(LogTemp,Display,TEXT("UI_QA_CAPTURE %s buttons=%d"),*Name,Buttons.Num());
}
void AEnigmaCampaignMode::TickUITour(){
 if(Clock<UITourNext)return;UITourNext=Clock+3.5f;
 const FString Names[]={TEXT("home"),TEXT("campaign"),TEXT("team"),TEXT("formation"),TEXT("cards"),TEXT("summon"),TEXT("training"),TEXT("settings"),TEXT("arenas"),TEXT("opening-1"),TEXT("opening-8"),TEXT("battle-formation"),TEXT("battle-ready"),TEXT("battle-action"),TEXT("floral"),TEXT("akuroth"),TEXT("mazakin"),TEXT("hajun"),TEXT("zhujin"),TEXT("tenrin"),TEXT("zevalon"),TEXT("mazakin-art"),TEXT("hajun-art"),TEXT("zhujin-art"),TEXT("tenrin-art"),TEXT("zevalon-art"),TEXT("hajun-phase1-art"),TEXT("hajun-phase2-art"),TEXT("zhujin-phase1-art"),TEXT("zhujin-phase2-art"),TEXT("hajun-3d-front"),TEXT("hajun-3d-back"),TEXT("hajun-3d-exit"),TEXT("hajun-idle"),TEXT("hajun-attack1"),TEXT("hajun-attack2"),TEXT("hajun-combo"),TEXT("hajun-charged")};
 if(UITourIndex>=UE_ARRAY_COUNT(Names)){UE_LOG(LogTemp,Display,TEXT("ENIGMA_UI_QA_RESULT captures=%d failures=%d"),UITourCaptures,UIWarnings);FPlatformMisc::RequestExitWithStatus(false,UIWarnings?1:0);return;}
 FString Name=Names[UITourIndex];
 if(UITourIndex<9){Action(TEXT("page"),Name);}
 else if(UITourIndex==9){Action(TEXT("page"),TEXT("opening"));OpeningAuto=false;}
 else if(UITourIndex==10)OpeningShot=7;
 else if(UITourIndex==11)Start(TEXT("demo"));
 else if(UITourIndex==12){auto Button=Buttons.FindByPredicate([](const FButton& B){return B.Name==TEXT("begin");});if(Button)Click(Button->Min+Button->Size*.5f);else UIWarnings++;}
 else if(UITourIndex==13){
  auto Battle=O(Game()->State,TEXT("battle"));FString Enemy=S(A(Battle,TEXT("enemies"))[0]->AsObject(),TEXT("key"));auto TargetButton=Buttons.FindByPredicate([&](const FButton& B){return B.Name==TEXT("target")&&B.Id==Enemy;});if(TargetButton)Click(TargetButton->Min+TargetButton->Size*.5f);else UIWarnings++;
  auto SkillButton=Buttons.FindByPredicate([](const FButton& B){return B.Name==TEXT("act")&&!B.Id.IsEmpty()&&B.Enabled;});if(SkillButton)Click(SkillButton->Min+SkillButton->Size*.5f);else UIWarnings++;
 }
 else if(UITourIndex==14){Start(TEXT("floral"));Game()->Command(TEXT("begin"));}
 else if(UITourIndex==15){Start(TEXT("akuroth"));Game()->Command(TEXT("begin"));}
 else if(UITourIndex<21){Start(Name);Game()->Command(TEXT("begin"));}
 else if(UITourIndex<26){Selected=Name.LeftChop(4);Action(TEXT("page"),TEXT("reference"));}
 else if(UITourIndex>=33){
  Selected=TEXT("hajun");Action(TEXT("page"),TEXT("reference"));const int32 Clip=UITourIndex-33;FTimerHandle MotionHandle;GetWorldTimerManager().SetTimer(MotionHandle,[this,Clip](){auto Choice=Buttons.FindByPredicate([Clip](const FButton& B){return B.Name==TEXT("modelMotion")&&B.Number==Clip&&B.Enabled;});if(Choice)Click(Choice->Min+Choice->Size*.5f);else UIWarnings++;if(!ModelPreview||!ModelPreview->MotionReady()){UIWarnings++;return;}if(Clip>0){FTimerHandle First;GetWorldTimerManager().SetTimer(First,[this](){const FVector Before=ModelPreview->HandLocation();FTimerHandle Second;GetWorldTimerManager().SetTimer(Second,[this,Before](){const FVector After=ModelPreview->HandLocation();const float Delta=(After-Before).Size();if(After.ContainsNaN()||Delta<1.f||Delta>350.f)UIWarnings++;UE_LOG(LogTemp,Display,TEXT("HAJUN_MOTION_HAND_DELTA %.3f"),Delta);},.4f,false);},.1f,false);}},.3f,false);
 }
 else if(UITourIndex>=30){
  Selected=TEXT("hajun");Action(TEXT("page"),TEXT("reference"));const int32 Step=UITourIndex;FTimerHandle PreviewHandle;GetWorldTimerManager().SetTimer(PreviewHandle,[this,Step](){auto Choice=Buttons.FindByPredicate([](const FButton& B){return B.Name==TEXT("reference3D")&&B.Enabled;});if(Choice)Click(Choice->Min+Choice->Size*.5f);else UIWarnings++;if(Step==31)Action(TEXT("previewRotate"),TEXT(""),180);if(Step==32){Action(TEXT("referencePhase"),TEXT(""),2);if(Reference3D)UIWarnings++;}if(!ModelPreview||!ModelPreview->Ready())UIWarnings++;},.3f,false);
 }
 else{
  Selected=UITourIndex<28?TEXT("hajun"):TEXT("zhujin");Action(TEXT("page"),TEXT("reference"));
  int32 Phase=(UITourIndex-26)%2+1;FTimerHandle PhaseHandle;GetWorldTimerManager().SetTimer(PhaseHandle,[this,Phase](){auto Choice=Buttons.FindByPredicate([Phase](const FButton& B){return B.Name==TEXT("referencePhase")&&B.Number==Phase&&B.Enabled;});if(Choice)Click(Choice->Min+Choice->Size*.5f);else UIWarnings++;},.3f,false);
 }
 UITourIndex++;FTimerHandle Handle;GetWorldTimerManager().SetTimer(Handle,[this,Name](){CaptureUI(Name);},.85f,false);
}

void AEnigmaCampaignMode::UpdateDisplayedHealth(){
 if(!AnimationDisplay||!BeforeAction)return;auto Current=O(Game()->State,TEXT("battle"));auto Before=O(BeforeAction,TEXT("battle"));auto Display=O(AnimationDisplay,TEXT("battle"));
 const FString Skill=S(Current,TEXT("lastSkill"));
 auto All=A(Current,TEXT("allies"));All.Append(A(Current,TEXT("enemies")));auto Old=A(Before,TEXT("allies"));Old.Append(A(Before,TEXT("enemies")));auto Visible=A(Display,TEXT("allies"));Visible.Append(A(Display,TEXT("enemies")));
 for(auto V:Visible){auto Unit=V->AsObject();FString Key=S(Unit,TEXT("key"));auto Start=Find(Old,Key,TEXT("key"));auto Final=Find(All,Key,TEXT("key"));if(S(Start,TEXT("key")).IsEmpty())continue;double HP=EnigmaCombatTiming::HealthAt(N(Start,TEXT("hp")),N(Final,TEXT("hp")),N(Unit,TEXT("maxHp")),Key,Skill,A(Current,TEXT("impacts")),AnimationAge,AnimationDuration);
  Unit->SetNumberField(TEXT("hp"),HP);Unit->SetBoolField(TEXT("alive"),HP>0);
 }
 if(AnimationAge<AnimationDuration-.1f)Display->SetBoolField(TEXT("finished"),false);
}

void AEnigmaCampaignMode::CheckReplayHealth(TSharedPtr<FJsonObject> Sample,const FString& Name){
 auto Battle=O(DisplayState(),TEXT("battle"));auto Units=A(Battle,TEXT("allies"));Units.Append(A(Battle,TEXT("enemies")));
 for(auto& Expected:O(Sample,TEXT("health"))->Values){CombatReplayChecks++;FString Key(*Expected.Key);auto Unit=Find(Units,Key,TEXT("key"));if(!FMath::IsNearlyEqual(N(Unit,TEXT("hp")),Expected.Value->AsNumber(),.1)){CombatReplayFailures++;UE_LOG(LogTemp,Error,TEXT("COMBAT_REPLAY_HP_FAIL %s target=%s expected=%.2f actual=%.2f age=%.3f"),*Name,*Key,Expected.Value->AsNumber(),N(Unit,TEXT("hp")),AnimationAge);}}
 CaptureUI(Name);
}
void AEnigmaCampaignMode::TickCombatReplay(){
 if(Clock<CombatReplayNext)return;
 if(CombatReplayIndex>=CombatReplayNames.Num()){UE_LOG(LogTemp,Display,TEXT("ENIGMA_COMBAT_REPLAY_RESULT fixtures=%d checks=%d failures=%d"),CombatReplayIndex,CombatReplayChecks,CombatReplayFailures+UIWarnings);if(!FParse::Param(FCommandLine::Get(),TEXT("EnigmaKeepReplayOpen")))FPlatformMisc::RequestExitWithStatus(false,CombatReplayFailures+UIWarnings?1:0);CombatReplayNext=FLT_MAX;return;}
 FString Name=CombatReplayNames[CombatReplayIndex++],Text;TSharedPtr<FJsonObject> Fixture;
 if(!FFileHelper::LoadFileToString(Text,*(CombatReplayFolder/(Name+TEXT(".json"))))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Fixture)){CombatReplayFailures++;CombatReplayNext=Clock+1;return;}
 Auto=false;Page=TEXT("battle");BeforeAction=O(Fixture,TEXT("before"));Game()->State=BeforeAction;AnimationAge=-1;AnimationDisplay.Reset();WorldArt->ShowBattle(O(BeforeAction,TEXT("battle")),6);
 float Delay=WorldArt->Transforming()?3.3f:.1f;CombatReplayNext=Clock+Delay+5.5f;
 FTimerHandle Handle;GetWorldTimerManager().SetTimer(Handle,[this,Fixture,Name](){
  Game()->State=O(Fixture,TEXT("after"));PresentLastAction();
  CombatReplayChecks++;if(!FMath::IsNearlyEqual(AnimationDuration,float(N(Fixture,TEXT("duration"))),.001f)){CombatReplayFailures++;UE_LOG(LogTemp,Error,TEXT("COMBAT_REPLAY_DURATION_FAIL %s duration=%.2f"),*Name,AnimationDuration);}
  for(auto Value:A(Fixture,TEXT("audioTimes"))){float At=Value->AsNumber();CombatReplayChecks++;if(!WorldArt->HasImpactCueAt(At)){CombatReplayFailures++;UE_LOG(LogTemp,Error,TEXT("COMBAT_REPLAY_AUDIO_FAIL %s at=%.2f"),*Name,At);}}
  int32 Index=0;for(auto Value:A(Fixture,TEXT("samples"))){auto Sample=Value->AsObject();FString Capture=Name+FString::Printf(TEXT("-%d"),Index++);FTimerHandle SampleHandle;GetWorldTimerManager().SetTimer(SampleHandle,[this,Sample,Capture](){CheckReplayHealth(Sample,Capture);},float(N(Sample,TEXT("at"))),false);}
 },Delay,false);
}
