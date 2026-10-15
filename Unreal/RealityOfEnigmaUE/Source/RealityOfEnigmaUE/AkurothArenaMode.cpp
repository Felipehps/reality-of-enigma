#include "AkurothArenaMode.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/PointLight.h"
#include "Engine/Canvas.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "TimerManager.h"
#include "InputCoreTypes.h"
#include "DrawDebugHelpers.h"
#include "NiagaraFunctionLibrary.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "UnrealClient.h"
#include "AkurothPresentation.h"
#include "AudioMixerBlueprintLibrary.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Sound/SoundWave.h"

namespace {
struct Skill {const TCHAR* Id;const TCHAR* Name;float Damage;int32 Cooldown;};
const Skill Skills[]={
    {TEXT("akuroth_cut"),TEXT("Corte do Vazio"),280.8f,0},
    {TEXT("akuroth_waves"),TEXT("Ondas da Escuridão"),144,3},
    {TEXT("akuroth_prison"),TEXT("Prisão das Almas"),117,4},
    {TEXT("akuroth_meteor"),TEXT("Meteoro do Eclipse"),351,4},
    {TEXT("akuroth_rift"),TEXT("Rasgo na Realidade"),207,5},
    {TEXT("akuroth_chaos"),TEXT("Dança do Caos"),405,0},
    {TEXT("akuroth_collapse"),TEXT("Colapso Dimensional"),144,3},
    {TEXT("akuroth_rain"),TEXT("Chuva de Eclipses"),540,4},
    {TEXT("akuroth_rewrite"),TEXT("Reescrita da Existência"),63,5},
    {TEXT("akuroth_annihilation"),TEXT("Aniquilação Absoluta"),0,6}};
}
AAkurothArenaMode::AAkurothArenaMode(){
    PrimaryActorTick.bCanEverTick=true;DefaultPawnClass=nullptr;
    PlayerControllerClass=AAkurothController::StaticClass();HUDClass=AAkurothHUD::StaticClass();
    static ConstructorHelpers::FObjectFinder<UStaticMesh> C(TEXT("/Engine/BasicShapes/Cube.Cube"));Cube=C.Object;
    static ConstructorHelpers::FObjectFinder<UStaticMesh> S(TEXT("/Engine/BasicShapes/Sphere.Sphere"));Sphere=S.Object;
    Cooldowns.Init(0,10);
}
AActor* AAkurothArenaMode::Shape(UStaticMesh* Mesh,FVector Position,FVector Scale){
    AActor* A=GetWorld()->SpawnActor<AActor>(AActor::StaticClass(),Position,FRotator::ZeroRotator);
    UStaticMeshComponent* M=NewObject<UStaticMeshComponent>(A);A->SetRootComponent(M);A->AddInstanceComponent(M);
    M->SetMobility(EComponentMobility::Movable);M->SetStaticMesh(Mesh);M->RegisterComponent();
    A->SetActorLocation(Position);A->SetActorScale3D(Scale);return A;
}
void AAkurothArenaMode::BeginPlay(){
    Super::BeginPlay();
    Hero=Shape(Cube,HeroHome,FVector(.6,.6,1.8));Boss=Shape(Cube,FVector(550,0,190),FVector(1.3,1,3.8));
    APointLight* Light=GetWorld()->SpawnActor<APointLight>(APointLight::StaticClass(),FVector(0,0,700),FRotator::ZeroRotator);
    UPointLightComponent* Lamp=Cast<UPointLightComponent>(Light->GetLightComponent());
    Lamp->SetMobility(EComponentMobility::Movable);Lamp->SetLightColor(FLinearColor(.8f,.16f,.22f));Lamp->SetIntensity(3000000);
    Lamp->SetAttenuationRadius(4000);
    ACameraActor* Camera=GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),FVector(0,-2200,1650),FRotator::ZeroRotator);
    Camera->SetActorRotation((FVector(0,0,140)-Camera->GetActorLocation()).Rotation());
    if(APlayerController* PC=UGameplayStatics::GetPlayerController(this,0))PC->SetViewTarget(Camera);
    Presentation=GetWorld()->SpawnActor<AAkurothPresentation>();Presentation->Initialize(this,Hero,Boss,Camera);
    Showcase=FParse::Param(FCommandLine::Get(),TEXT("AkurothShowcase"));FParse::Value(FCommandLine::Get(),TEXT("AkurothRecord="),RecordFolder);
    if(!RecordFolder.IsEmpty()){IFileManager::Get().MakeDirectory(*RecordFolder,true);GEngine->SetMaxFPS(24);Showcase=true;}
    Cooldowns[2]=2;Cooldowns[3]=3;Cooldowns[4]=4;
    if(FParse::Param(FCommandLine::Get(),TEXT("AkurothQA")))
        GetWorldTimerManager().SetTimer(ActionTimer,this,&AAkurothArenaMode::RunPrototypeChecks,.2f,false);
    FString Screenshot;
    if(FParse::Value(FCommandLine::Get(),TEXT("AkurothScreenshot="),Screenshot)){
        FTimerHandle CaptureTimer,ExitTimer;
        float CaptureAt=5.f;FParse::Value(FCommandLine::Get(),TEXT("AkurothCaptureAt="),CaptureAt);
        if(FParse::Param(FCommandLine::Get(),TEXT("AkurothCaptureUltimate"))){PhaseTwo=true;Charging=true;Busy=true;BossTurn();}
        GetWorldTimerManager().SetTimer(CaptureTimer,FTimerDelegate::CreateLambda([Screenshot](){
            FScreenshotRequest::RequestScreenshot(Screenshot,true,false);}),CaptureAt,false);
        GetWorldTimerManager().SetTimer(ExitTimer,FTimerDelegate::CreateLambda([](){
            FPlatformMisc::RequestExit(false);}),CaptureAt+3.f,false);
    }
}
void AAkurothArenaMode::RunPrototypeChecks(){
    int32 Checks=0,Failures=0;
    auto Check=[&](bool Result,const TCHAR* Name){++Checks;if(!Result)++Failures;
        UE_LOG(LogTemp,Display,TEXT("AKUROTH_QA %s: %s"),Result?TEXT("PASS"):TEXT("FAIL"),Name);};
    auto Reset=[&](){GetWorldTimerManager().ClearTimer(ActionTimer);BossHP=36000;HeroHP=10000;
        PhaseTwo=false;Charging=false;Busy=false;Finished=false;Protected=false;TransformationAge=-1;OrbAge=-1;
        CompressionDone=false;ExplosionDone=false;Cooldowns.Init(0,10);
        CastAge=-1;PendingSkill=-1;ForcedSkill=-1;CastDamageDone=false;
        CastHits=0;BossTurnCount=0;PrisonTurns=0;RiftTurns=0;RewriteTurns=0;HazardTurns=0;
        PlayerCastAge=-1;PendingPlayerDamage=0;
        if(Orb){Orb->Destroy();Orb=nullptr;}Hero->SetActorLocation(HeroHome);};
    Check(Hero&&Boss&&Cube&&Sphere,TEXT("Arena actors and engine meshes exist"));
    Check(Presentation&&Presentation->AssetsReady,TEXT("Five skeletal heroes, boss, animation and materials loaded"));
    Reset();PlayerAttack();Check(BossHP==36000&&Busy,TEXT("Hero advance starts before boss takes damage"));
    PlayerAttack();Check(BossHP==36000,TEXT("Attack blocked while boss action is pending"));Tick(.7f);Check(BossHP==33500,TEXT("Group damage lands at contact"));
    GetWorldTimerManager().ClearTimer(ActionTimer);BossTurn();Check(Busy&&HeroHP==10000,TEXT("AI winds up before applying damage"));Tick(2.5f);Check(!Busy&&HeroHP<10000,TEXT("AI responds and returns control"));
    Reset();BossHP=10801;EndPlayerTurn();Check(!PhaseTwo,TEXT("No transformation above 30 percent"));
    Reset();BossHP=10800;EndPlayerTurn();Check(PhaseTwo&&Busy,TEXT("Transformation starts at exactly 30 percent"));
    Tick(1.f);Check(TransformationAge>=0&&Busy,TEXT("Transformation holds the turn"));
    Tick(2.1f);Check(TransformationAge<0&&Charging&&!Busy,TEXT("Transformation completes and ultimate is telegraphed"));
    float Before=HeroHP;Interrupt();Check(!Charging&&Busy&&Cooldowns[9]==6&&HeroHP==Before,TEXT("Interrupt cancels ultimate without damage"));
    GetWorldTimerManager().ClearTimer(ActionTimer);
    for(int32 I=0;I<9;++I){Reset();PhaseTwo=I>=5;Cooldowns.Init(99,10);Cooldowns[I]=0;Busy=true;BossTurn();
        Check(HeroHP==10000&&Presentation->LastSkill==I,TEXT("Skill visual starts before damage"));Tick(2.5f);
        Check(FMath::IsNearlyEqual(HeroHP,10000-Skills[I].Damage,.02f)&&!Busy,Skills[I].Name);}
    Reset();PrisonTurns=1;PlayerAttack();Check(BossHP==36000&&PrisonTurns==0&&Busy,TEXT("Soul prison consumes the movement action"));
    Reset();RewriteTurns=1;PlayerAttack();Tick(.7f);Check(BossHP==34750&&RewriteTurns==0,TEXT("Existence rewrite weakens next group attack"));
    Reset();RiftTurns=2;Guard();Check(HeroHP==9980&&RiftTurns==1,TEXT("Reality rift persists across turns and respects guard"));
    Reset();PhaseTwo=true;PlayerAttack();Tick(.7f);Check(BossHP==34500,TEXT("Phase two resistance extends the fight without healing boss"));
    for(int32 Phase=0;Phase<2;++Phase){Reset();PhaseTwo=Phase==1;bool Seen[10]={};
        for(int32 Round=0;Round<12;++Round){Busy=true;BossTurn();if(Presentation->LastSkill>=0)Seen[Presentation->LastSkill]=true;Tick(4.7f);}
        for(int32 I=Phase*5;I<Phase*5+5;++I)Check(Seen[I],TEXT("Natural AI rotation includes every skill in its phase"));}
    Reset();PhaseTwo=true;Busy=true;BossTurn();Check(Charging&&!Busy&&HeroHP==10000,TEXT("Ultimate charge deals no damage"));
    Guard();GetWorldTimerManager().ClearTimer(ActionTimer);BossTurn();
    Check(Orb&&OrbAge==0&&Busy,TEXT("Guard allows ultimate release with protection"));
    Tick(.9f);Check(HeroHP==10000&&Hero->GetActorLocation().X>HeroHome.X,TEXT("Orb pulls hero before first damage"));
    Tick(.3f);Check(FMath::IsNearlyEqual(HeroHP,9918.5f),TEXT("Compression applies protected damage once"));
    Tick(.5f);Check(FMath::IsNearlyEqual(HeroHP,9918.5f),TEXT("Compression does not repeat each frame"));
    Tick(1.4f);Check(FMath::IsNearlyEqual(HeroHP,9556.f)&&!Protected,TEXT("Explosion applies protected second hit"));
    Tick(1.6f);Check(!Orb&&OrbAge<0&&!Busy&&Hero->GetActorLocation().Equals(HeroHome),TEXT("Ultimate restores position and control"));
    Reset();BossHP=2500;PlayerAttack();Tick(.7f);Check(Finished&&BossHP==0&&!Busy,TEXT("Victory ends fight without another boss turn"));
    Reset();HeroHP=1;BossTurn();Tick(2.5f);Check(Finished&&HeroHP==0,TEXT("Hero defeat ends fight"));
    UE_LOG(LogTemp,Display,TEXT("AKUROTH_QA_RESULT checks=%d failures=%d"),Checks,Failures);
    FPlatformMisc::RequestExitWithStatus(false,Failures?1:0);
}
void AAkurothArenaMode::PlayerAttack(){if(Busy||Finished)return;if(PrisonTurns>0){PrisonTurns=0;Message=TEXT("Grupo rompe a Prisão das Almas — ação consumida");EndPlayerTurn();return;}Presentation->PlayHeroes();PendingPlayerDamage=(RewriteTurns>0?1250.f:2500.f)*(PhaseTwo?.6f:1.f);RewriteTurns=0;PlayerCastAge=0;Busy=true;Message=TEXT("O grupo avança para atacar");}
void AAkurothArenaMode::Guard(){if(Busy||Finished)return;Protected=true;Message=TEXT("Proteção: reduz o próximo ataque em 50%");EndPlayerTurn();}
void AAkurothArenaMode::Interrupt(){if(Busy||Finished||!Charging)return;Charging=false;Cooldowns[9]=6;Message=TEXT("Preparação interrompida");EndPlayerTurn();}
void AAkurothArenaMode::TestPhaseTwo(){if(Busy||Finished||PhaseTwo)return;BossHP=10800;EndPlayerTurn();}
void AAkurothArenaMode::EndPlayerTurn(){Busy=true;if(RiftTurns>0){DamageHero(40);--RiftTurns;}if(HazardTurns>0){DamageHero(100);--HazardTurns;}ResolveEnd();if(Finished)return;if(!PhaseTwo&&BossHP<=10800.001f){Transform();return;}GetWorldTimerManager().SetTimer(ActionTimer,this,&AAkurothArenaMode::BossTurn,1.6f,false);}
void AAkurothArenaMode::TestSkill(int32 Index){if(Busy||Finished||Index<0||Index>9)return;PhaseTwo=Index>=5;if(Index!=9)Charging=false;ForcedSkill=Index;Busy=true;BossTurn();}
void AAkurothArenaMode::Transform(){PhaseTwo=true;BossTurnCount=0;TransformationAge=0;Charging=false;Cooldowns[6]=0;Cooldowns[7]=1;Cooldowns[8]=2;Cooldowns[9]=1;Message=TEXT("SOBERANO DO ABSOLUTO — transformação, resistência +40%");if(auto* Wave=LoadObject<USoundWave>(nullptr,TEXT("/Game/Akuroth/Audio/akuroth-transform.akuroth-transform")))UGameplayStatics::PlaySound2D(this,Wave,.7f);}
void AAkurothArenaMode::DamageHero(float Damage){HeroHP=FMath::Max(0.f,HeroHP-Damage*(Protected?.5f:1.f));if(auto* Wave=LoadObject<USoundWave>(nullptr,TEXT("/Game/Akuroth/Audio/akuroth-impact.akuroth-impact")))UGameplayStatics::PlaySound2D(this,Wave,Protected?.12f:.22f,Damage>300?.8f:1.1f);}
void AAkurothArenaMode::ResolveEnd(){if(BossHP<=0||HeroHP<=0){Finished=true;Busy=false;Charging=false;Message=BossHP<=0?TEXT("AKUROTH derrotado"):TEXT("Herói derrotado");}}
void AAkurothArenaMode::BossTurn(){
    if(Finished)return;for(int32& CD:Cooldowns)CD=FMath::Max(0,CD-1);
    int32 Index=PhaseTwo?5:0;for(int32 I=PhaseTwo?9:4;I>Index;I--)if(Cooldowns[I]==0){Index=I;break;}
    ++BossTurnCount;if(!Charging&&BossTurnCount%3==0)Index=PhaseTwo?5:0;
    if(Charging)Index=9;Message=Skills[Index].Name;
    if(ForcedSkill>=0){Index=ForcedSkill;ForcedSkill=-1;Message=Skills[Index].Name;}
    if(TObjectPtr<UNiagaraSystem>* FX=AbilityEffects.Find(FName(Skills[Index].Id)))if(*FX)UNiagaraFunctionLibrary::SpawnSystemAtLocation(this,FX->Get(),Hero->GetActorLocation());
    if(Index==9){
        if(!Charging){Charging=true;Busy=false;Message=TEXT("Aniquilação preparada — proteja ou interrompa antes da próxima ação!");return;}
        Charging=false;Cooldowns[9]=6;OrbAge=0;CompressionDone=false;ExplosionDone=false;
        Presentation->PlaySkill(9);Orb=Shape(Sphere,FVector(0,0,200),FVector(.1));Cast<UStaticMeshComponent>(Orb->GetRootComponent())->SetVisibility(false);return;
    }
    Cooldowns[Index]=Skills[Index].Cooldown;PendingSkill=Index;CastAge=0;CastHits=0;CastDamageDone=false;Busy=true;Presentation->PlaySkill(Index);
}
void AAkurothArenaMode::Tick(float Dt){
    Super::Tick(Dt);
    TickShowcase(Dt);
    if(PlayerCastAge>=0){PlayerCastAge+=Dt;if(PlayerCastAge>=.65f){PlayerCastAge=-1;BossHP=FMath::Max(0.f,BossHP-PendingPlayerDamage);if(auto* Wave=LoadObject<USoundWave>(nullptr,TEXT("/Game/Akuroth/Audio/akuroth-impact.akuroth-impact")))UGameplayStatics::PlaySound2D(this,Wave,.3f,.7f);Message=FString::Printf(TEXT("Grupo ataca: %.0f"),PendingPlayerDamage);ResolveEnd();if(!Finished){if(Showcase)Busy=false;else EndPlayerTurn();}}}
    if(CastAge>=0){CastAge+=Dt;int32 Hits=PendingSkill==0?3:PendingSkill==5?5:PendingSkill==7?9:1;
        float FirstHit=(PendingSkill==0||PendingSkill==5)?.55f:(PendingSkill==3||PendingSkill==7)?1.3f:1.1f;
        float Interval=PendingSkill==7?.07f:.24f;
        while(!Finished&&CastHits<Hits&&CastAge>=FirstHit+CastHits*Interval){DamageHero(Skills[PendingSkill].Damage/Hits);++CastHits;ResolveEnd();}
        if(CastHits==Hits&&!CastDamageDone){CastDamageDone=true;Protected=false;
            if(PendingSkill==2){PrisonTurns=1;BossHP=FMath::Min(36000.f,BossHP+100);}
            if(PendingSkill==4)RiftTurns=2;if(PendingSkill==6)HazardTurns=1;if(PendingSkill==8)RewriteTurns=1;}
        if(CastAge>=2.4f){CastAge=-1;PendingSkill=-1;Busy=false;}}
    if(TransformationAge>=0){TransformationAge+=Dt;float T=TransformationAge;
        Boss->SetActorLocation(FVector(550,0,190+FMath::Sin(FMath::Min(T/3.f,1.f)*PI)*130));
        if(T>=3){TransformationAge=-1;BossTurn();}}
    if(OrbAge>=0){OrbAge+=Dt;float T=OrbAge;
        float Pull=T<3?FMath::SmoothStep(.3f,1.5f,T):1-FMath::SmoothStep(3.f,4.3f,T);
        Hero->SetActorLocation(FMath::Lerp(HeroHome,FVector(-60,0,200),Pull));
        if(Orb)Orb->SetActorScale3D(FVector(T<3?.2f+FMath::Min(T/2,1.f)*2.8f:3+FMath::Min(T-3,1.f)*10));
        if(T>=1.1f&&!CompressionDone){CompressionDone=true;DamageHero(180*.35f+10000*.01f);}
        if(T>=3&&!ExplosionDone){ExplosionDone=true;DamageHero(180*1.25f+10000*.05f);Protected=false;ResolveEnd();}
        if(T>=4.6f){OrbAge=-1;Hero->SetActorLocation(HeroHome);if(Orb){Orb->Destroy();Orb=nullptr;}Busy=false;}}
}
void AAkurothArenaMode::FinishRecording(){
    if(!Recording)return;Recording=false;
    if(RecordFrame>0){RecordTimeline.Add(FString::Printf(TEXT("file 'frame-%05d.bmp'\nduration 0.041667\n"),RecordFrame-1));}
    FFileHelper::SaveStringToFile(FString::Join(RecordTimeline,TEXT("")),*(RecordFolder/TEXT("frames.txt")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);
    UAudioMixerBlueprintLibrary::StopRecordingOutput(this,EAudioRecordingExportType::WavFile,TEXT("battle-audio"),RecordFolder);
    UE_LOG(LogTemp,Display,TEXT("AKUROTH_RECORD_COMPLETE frames=%d skills=%d"),RecordFrame,ShowcaseIndex);
    FTimerHandle Exit;GetWorldTimerManager().SetTimer(Exit,FTimerDelegate::CreateLambda([](){FPlatformMisc::RequestExit(false);}),2.f,false);
}
void AAkurothArenaMode::TickShowcase(float Dt){
    if(!Showcase)return;ShowcaseClock+=Dt;
    if(!RecordFolder.IsEmpty()){
        if(!Recording){if(auto* CV=IConsoleManager::Get().FindConsoleVariable(TEXT("au.NeverDisableSubmixes")))CV->Set(1);Recording=true;RecordStarted=FPlatformTime::Seconds();LastRecordTime=RecordStarted;UAudioMixerBlueprintLibrary::StartRecordingOutput(this,60);}
        double Now=FPlatformTime::Seconds();if(!FScreenshotRequest::IsScreenshotRequested()){
            if(RecordFrame>0)RecordTimeline.Add(FString::Printf(TEXT("file 'frame-%05d.bmp'\nduration %.6f\n"),RecordFrame-1,Now-LastRecordTime));
            FScreenshotRequest::RequestScreenshot(RecordFolder/FString::Printf(TEXT("frame-%05d.bmp"),RecordFrame++),true,false);LastRecordTime=Now;}
    }
    if(!Busy&&ShowcaseClock>=ShowcaseNext){
        if(ShowcaseIndex==5&&!ShowcaseTransformed){ShowcaseTransformed=true;TestPhaseTwo();ShowcaseNext=ShowcaseClock+4;}
        else if(ShowcaseIndex<10){UE_LOG(LogTemp,Display,TEXT("AKUROTH_SHOWCASE skill=%d"),ShowcaseIndex);TestSkill(ShowcaseIndex++);ShowcaseNext=ShowcaseClock+3.6f;}
        else if(Charging){Guard();ShowcaseNext=ShowcaseClock+7;}
        else{Showcase=false;Message=TEXT("Demonstração concluída — todas as habilidades executadas");FinishRecording();}
    }else if(!Busy&&!Charging&&ShowcaseIndex>0&&ShowcaseIndex<10&&ShowcaseHeroStep!=ShowcaseIndex&&ShowcaseClock>=ShowcaseNext-1.2f){ShowcaseHeroStep=ShowcaseIndex;Presentation->PlayHeroes();PendingPlayerDamage=600;PlayerCastAge=0;Busy=true;}
}
void AAkurothController::SetupInputComponent(){Super::SetupInputComponent();InputComponent->BindKey(EKeys::SpaceBar,IE_Pressed,this,&AAkurothController::Attack);InputComponent->BindKey(EKeys::G,IE_Pressed,this,&AAkurothController::Protect);InputComponent->BindKey(EKeys::I,IE_Pressed,this,&AAkurothController::Cancel);InputComponent->BindKey(EKeys::F2,IE_Pressed,this,&AAkurothController::Phase);
    const FKey Keys[]={EKeys::One,EKeys::Two,EKeys::Three,EKeys::Four,EKeys::Five,EKeys::Six,EKeys::Seven,EKeys::Eight,EKeys::Nine,EKeys::Zero};
    for(int32 N=0;N<10;++N){auto& Binding=InputComponent->BindKey(Keys[N],IE_Pressed,this,&AAkurothController::Phase);Binding.KeyDelegate.GetDelegateForManualSet().BindLambda([this,N](){if(auto* M=Cast<AAkurothArenaMode>(GetWorld()->GetAuthGameMode()))M->TestSkill(N);});}
    auto& Restart=InputComponent->BindKey(EKeys::R,IE_Pressed,this,&AAkurothController::Phase);Restart.KeyDelegate.GetDelegateForManualSet().BindLambda([this](){UGameplayStatics::OpenLevel(this,TEXT("/Engine/Maps/Entry"));});
}
void AAkurothController::Attack(){if(auto* M=Cast<AAkurothArenaMode>(GetWorld()->GetAuthGameMode()))M->PlayerAttack();}
void AAkurothController::Protect(){if(auto* M=Cast<AAkurothArenaMode>(GetWorld()->GetAuthGameMode()))M->Guard();}
void AAkurothController::Cancel(){if(auto* M=Cast<AAkurothArenaMode>(GetWorld()->GetAuthGameMode()))M->Interrupt();}
void AAkurothController::Phase(){if(auto* M=Cast<AAkurothArenaMode>(GetWorld()->GetAuthGameMode()))M->TestPhaseTwo();}
void AAkurothHUD::DrawHUD(){Super::DrawHUD();auto* M=Cast<AAkurothArenaMode>(GetWorld()->GetAuthGameMode());if(!M||!Canvas)return;
    FLinearColor Gold(.65f,.36f,.16f),Black(.018f,.009f,.025f,.95f),Red(.8f,.015f,.035f);
    float Width=Canvas->SizeX*.6f,X=(Canvas->SizeX-Width)/2;
    DrawRect(Black,X-12,10,Width+24,84);DrawRect(Gold,X-8,58,Width+16,18);DrawRect(Black,X-5,61,Width+10,12);DrawRect(Red,X,61,Width*M->BossHP/36000,12);
    DrawText(TEXT("A K U R O T H"),FLinearColor(.95f,.82f,.65f),X+Width*.34f,13,nullptr,1.5f);
    DrawText(M->PhaseTwo?TEXT("SOBERANO DO ABSOLUTO"):TEXT("O PORTADOR DO VAZIO"),FLinearColor::White,X+Width*.34f,39);
    DrawText(FString::Printf(TEXT("%.0f / 36000"),M->BossHP),FLinearColor::White,X+Width-110,80);
    DrawRect(Black,0,Canvas->SizeY-130,Canvas->SizeX,130);
    DrawText(FString::Printf(TEXT("VIDA DO GRUPO  %.0f / 10000"),M->HeroHP),FLinearColor::White,20,Canvas->SizeY-124);
    DrawRect(Gold,20,Canvas->SizeY-100,240,10);DrawRect(Red,21,Canvas->SizeY-99,238*FMath::Clamp(M->HeroHP/10000,0.f,1.f),8);
    DrawText(M->Message,M->Charging?FLinearColor(1,.25f,.15f):FLinearColor::White,290,Canvas->SizeY-122);
    const TCHAR* Names[]={TEXT("Corte"),TEXT("Ondas"),TEXT("Prisão"),TEXT("Meteoro"),TEXT("Rasgo"),TEXT("Dança"),TEXT("Colapso"),TEXT("Chuva"),TEXT("Reescrita"),TEXT("Aniquilação")};
    float Slot=(Canvas->SizeX-40)/10;
    for(int32 N=0;N<10;++N){float Left=20+N*Slot;DrawRect(N<5?Red:FLinearColor(.4f,.04f,.6f),Left,Canvas->SizeY-77,Slot-6,2);DrawRect(FLinearColor(.06f,.025f,.075f),Left,Canvas->SizeY-74,Slot-6,34);DrawText(FString::Printf(TEXT("%d  %s"),(N+1)%10,Names[N]),FLinearColor::White,Left+5,Canvas->SizeY-65);}
    DrawText(TEXT("ESPAÇO atacar • G proteger • I interromper • F2 transformação • 1–0 testar habilidades • R reiniciar"),FLinearColor(.85f,.72f,.58f),20,Canvas->SizeY-28);
}
