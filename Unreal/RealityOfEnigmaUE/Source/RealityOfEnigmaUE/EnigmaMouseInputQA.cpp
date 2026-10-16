#include "EnigmaCampaignMode.h"
#include "EnigmaGameInstance.h"
#include "UnrealClient.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "TimerManager.h"

void AEnigmaCampaignMode::TickMouseTour(){
 if(Clock<MouseTourNext)return;MouseTourNext=Clock+1.4f;
 auto Check=[&](bool OK,const TCHAR* Label){MouseChecks++;if(!OK){MouseFailures++;UE_LOG(LogTemp,Error,TEXT("MOUSE_QA_FAIL %s"),Label);}};
 int32 W=0,H=0;GetWorld()->GetFirstPlayerController()->GetViewportSize(W,H);
 // Calculate clicks independently of the stored button rectangles. The battle camera is 16:9.
 const float ActiveW=FMath::Min(float(W),H*16.f/9),ActiveH=FMath::Min(float(H),W*9.f/16);
 const FVector2D Origin((W-ActiveW)*.5f,(H-ActiveH)*.5f);const float Scale=ActiveW/1280.f;
 auto Point=[&](float X,float Y){return Origin+FVector2D(X,Y)*Scale;};
 auto Expect=[&](float X,float Y,const TCHAR* Name,const TCHAR* Id){int32 I=ButtonAtViewport(Point(X,Y));Check(Buttons.IsValidIndex(I)&&Buttons[I].Name==Name&&Buttons[I].Id==Id,TEXT("independent_visual_hit"));};
 FString Shot;
 switch(MouseTourIndex++){
  case 0:Start(TEXT("demo"));Action(TEXT("page"),TEXT("home"));break;
  case 1:
   Check(ButtonOrigin.Equals(Origin,2),TEXT("camera_origin"));Check(ButtonLayoutCurrent(),TEXT("fresh_layout"));
   Expect(263,378,TEXT("start"),TEXT("campaign"));Expect(263,457,TEXT("page"),TEXT("campaign"));Expect(263,525,TEXT("page"),TEXT("team"));
   Expect(134,622,TEXT("page"),TEXT("settings"));Expect(251,622,TEXT("page"),TEXT("extras"));Expect(342,622,TEXT("homeQuit"),TEXT(""));
   Click(Point(134,622));Check(Page==TEXT("settings"),TEXT("real_settings_click"));Shot=TEXT("01-configuracoes");break;
  case 2:Action(TEXT("page"),TEXT("home"));Click(Point(251,622));Check(Page==TEXT("home"),TEXT("stale_page_rejected"));break;
  case 3:
   if(Origin.X>2)Click(FVector2D(Origin.X*.5,H*.5));else if(Origin.Y>2)Click(FVector2D(W*.5,Origin.Y*.5));
   Check(Page==TEXT("home")&&!HomeQuitDialog,TEXT("letterbox_no_action"));Click(Point(251,622));Check(Page==TEXT("extras"),TEXT("extras_click"));break;
  case 4:Click(Point(276,526));Check(Page==TEXT("cards"),TEXT("compendium_click"));Shot=TEXT("02-compendio");break;
  case 5:Click(Point(200,230));Check(Selected==TEXT("savor"),TEXT("roster_click"));Action(TEXT("page"),TEXT("home"));break;
  case 6:{FIntPoint Saved=ButtonViewport;ButtonViewport.X++;Check(ButtonAtViewport(Point(134,622))==INDEX_NONE,TEXT("resize_stale_rejected"));ButtonViewport=Saved;
   Click(Point(342,622));Check(HomeQuitDialog,TEXT("quit_dialog_click"));Click(Point(134,622));Check(HomeQuitDialog&&Page==TEXT("home"),TEXT("stale_modal_rejected"));Shot=TEXT("03-confirmacao");break;}
  case 7:Click(Point(484,409));Check(!HomeQuitDialog,TEXT("cancel_dialog_click"));break;
  case 8:HomeFocus=3;MenuAccept();Check(Page==TEXT("settings"),TEXT("keyboard_still_works"));break;
  default:UE_LOG(LogTemp,Display,TEXT("ENIGMA_MOUSE_QA_RESULT checks=%d failures=%d"),MouseChecks,MouseFailures);FPlatformMisc::RequestExitWithStatus(false,MouseFailures?1:0);return;
 }
 if(!Shot.IsEmpty()){FTimerHandle Handle;GetWorldTimerManager().SetTimer(Handle,[this,Shot](){FScreenshotRequest::RequestScreenshot(FPaths::ConvertRelativePathToFull(MouseTourFolder/(Shot+TEXT(".png"))),true,false);},.3f,false);}
}
