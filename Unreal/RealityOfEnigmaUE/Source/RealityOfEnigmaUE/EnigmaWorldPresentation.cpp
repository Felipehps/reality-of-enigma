#include "EnigmaWorldPresentation.h"
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/AudioComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimSequence.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "Engine/World.h"
#include "Engine/Texture.h"
namespace EW {
 FString S(TSharedPtr<FJsonObject> P,const TCHAR* K){FString V;if(P)P->TryGetStringField(K,V);return V;}
 double N(TSharedPtr<FJsonObject> P,const TCHAR* K){double V=0;if(P)P->TryGetNumberField(K,V);return V;}
 bool B(TSharedPtr<FJsonObject> P,const TCHAR* K){bool V=false;if(P)P->TryGetBoolField(K,V);return V;}
 const TArray<TSharedPtr<FJsonValue>>& A(TSharedPtr<FJsonObject> P,const TCHAR* K){static TArray<TSharedPtr<FJsonValue>> Empty;const TArray<TSharedPtr<FJsonValue>>* V=nullptr;return P&&P->TryGetArrayField(K,V)?*V:Empty;}
}
using namespace EW;
AEnigmaWorldPresentation::AEnigmaWorldPresentation(){
 PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
 Sphere=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Akuroth/Materials/M_Combat.M_Combat"));
 Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));Attack=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
 auto Batch=[&](const TCHAR* Name,UStaticMesh* Mesh,FLinearColor Color,float Glow){auto C=CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetMaterial(0,Material);return C;};
 Effects=Batch(TEXT("EffectLines"),Cylinder,FLinearColor(.35f,.14f,.9f),2.5f);Crimson=Batch(TEXT("CrimsonLines"),Cylinder,FLinearColor(.65f,.015f,.035f),2.5f);Dark=Batch(TEXT("VoidOrbs"),Sphere,FLinearColor(.003f,.001f,.009f),0);ShadowLimbs=Batch(TEXT("ShadowLimbs"),Cylinder,FLinearColor(.008f,.003f,.015f),0);
 WhiteHair=Batch(TEXT("WhiteHair"),Cylinder,FLinearColor(.82f,.8f,.86f),0);
 Light=CreateDefaultSubobject<UPointLightComponent>(TEXT("ArenaLight"));Light->SetupAttachment(RootComponent);Light->SetRelativeLocation(FVector(0,400,800));Light->SetIntensity(65000);Light->SetAttenuationRadius(4500);Light->SetLightColor(FLinearColor(.9f,.85f,.8f));
 Ready=Material&&Idle&&Attack&&Sphere&&Cylinder&&Cube;
}
void AEnigmaWorldPresentation::BeginPlay(){
 Super::BeginPlay();
 FString Data;if(FFileHelper::LoadFileToString(Data,*(FPaths::ProjectContentDir()/TEXT("GameData/audio.json"))))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),SoundProfiles);
 if(FFileHelper::LoadFileToString(Data,*(FPaths::ProjectContentDir()/TEXT("GameData/audio-samples.json"))))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),SampleProfiles);
 auto Set=[&](UInstancedStaticMeshComponent* Batch,FLinearColor Color,float Glow){auto M=UMaterialInstanceDynamic::Create(Material,this);M->SetVectorParameterValue(TEXT("Color"),Color);M->SetScalarParameterValue(TEXT("Glow"),Glow);Batch->SetMaterial(0,M);};
 Set(Effects,FLinearColor(.35f,.14f,.9f),2.5f);Set(Crimson,FLinearColor(.65f,.015f,.035f),2.5f);Set(Dark,FLinearColor(.003f,.001f,.009f),0);Set(ShadowLimbs,FLinearColor(.008f,.003f,.015f),0);Set(WhiteHair,FLinearColor(.82f,.8f,.86f),0);
}
void AEnigmaWorldPresentation::Shape(UStaticMesh* Mesh,FVector P,FVector V,FLinearColor Color,FRotator R){
 auto C=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();C->SetWorldLocation(P);C->SetWorldScale3D(V);C->SetWorldRotation(R);auto M=UMaterialInstanceDynamic::Create(Material,this);M->SetVectorParameterValue(TEXT("Color"),Color);M->SetScalarParameterValue(TEXT("Glow"),0);C->SetMaterial(0,M);Scenery.Add(C);
}
void AEnigmaWorldPresentation::ShowBattle(TSharedPtr<FJsonObject> Battle,int32 Region){
 if(!Battle||A(Battle,TEXT("allies")).Num()==0){HideBattle();return;}
 TArray<TSharedPtr<FJsonValue>> Units=A(Battle,TEXT("allies"));Units.Append(A(Battle,TEXT("enemies")));bool Rebuild=Units.Num()!=Keys.Num();for(int32 I=0;I<Units.Num()&&!Rebuild;I++)Rebuild|=S(Units[I]->AsObject(),TEXT("key"))!=Keys[I];
 if(!Camera){Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->SetActorLocation(FVector(0,2450,1150));Camera->SetActorRotation((FVector(0,0,170)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(58);if(auto PC=UGameplayStatics::GetPlayerController(this,0))PC->SetViewTarget(Camera);}
 if(Rebuild){Ascended=false;TransformAge=-1;Age=-1;StopSounds();
  for(auto C:Bodies)C->DestroyComponent();Bodies.Reset();Keys.Reset();Homes.Reset();Colors.Reset();
  auto Manny=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));auto Quinn=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));Ready=Ready&&Manny&&Quinn;
  const FLinearColor Pal[]={FLinearColor(.02f,.42f,.3f),FLinearColor(.05f,.25f,.72f),FLinearColor(.6f,.3f,.05f),FLinearColor(.35f,.025f,.65f),FLinearColor(.65f,.025f,.045f)};
  for(int32 I=0;I<Units.Num();I++){
   auto U=Units[I]->AsObject();FString Id=S(U,TEXT("id"));bool Enemy=I>=A(Battle,TEXT("allies")).Num();auto Body=NewObject<USkeletalMeshComponent>(this);AddInstanceComponent(Body);Body->SetupAttachment(RootComponent);Body->SetSkeletalMesh(Id==TEXT("aelia")||Id==TEXT("lyra")?Quinn:Manny);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Body->RegisterComponent();
   FLinearColor Color=Enemy?Id==TEXT("akuroth")?FLinearColor(.018f,.009f,.025f):FLinearColor(.25f,.025f,.06f):Pal[I%5];auto Mat=UMaterialInstanceDynamic::Create(Material,this);Mat->SetVectorParameterValue(TEXT("Color"),Color);Mat->SetScalarParameterValue(TEXT("Glow"),.02f);for(int32 Slot=0;Slot<Body->GetNumMaterials();Slot++)Body->SetMaterial(Slot,Mat);
   const float Size=Id==TEXT("akuroth")?2.5f:B(U,TEXT("boss"))?1.8f:1.2f;Body->SetWorldScale3D(FVector(Size));Body->SetWorldRotation(FRotator(0,Enemy?90:-90,0));Body->PlayAnimation(Idle,true);Bodies.Add(Body);Keys.Add(S(U,TEXT("key")));Colors.Add(Color);Homes.Add(FVector((Enemy?1:-1)*(450+float(N(U,TEXT("layer")))*120),(float(N(U,TEXT("column")))-1)*190,0));
  }
  for(auto C:Scenery)C->DestroyComponent();Scenery.Reset();Light->SetLightColor(FLinearColor(.9f,.85f,.8f));
  auto Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
  Shape(Plane,FVector(0,-1850,600),FVector(50,27,1),FLinearColor::White,FRotator(0,0,90));Backdrop=Scenery.Last();
  if(auto Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/GameArt/M_RegionBackdrop.M_RegionBackdrop"))){auto Mat=UMaterialInstanceDynamic::Create(Parent,this);const FString TexName=FString::Printf(TEXT("Region_%d"),FMath::Clamp(Region,0,6)+1);auto Tex=LoadObject<UTexture>(nullptr,*(TEXT("/Game/GameArt/")+TexName+TEXT(".")+TexName));Mat->SetTextureParameterValue(TEXT("Backdrop"),Tex);Backdrop->SetMaterial(0,Mat);}

  const FLinearColor Ground[]={FLinearColor(.035f,.09f,.055f),FLinearColor(.09f,.075f,.055f),FLinearColor(.06f,.035f,.035f),FLinearColor(.15f,.23f,.28f),FLinearColor(.07f,.05f,.1f),FLinearColor(.025f,.04f,.08f),FLinearColor(.035f,.016f,.025f)};
  Region=FMath::Clamp(Region,0,6);Shape(Cube,FVector(0,0,-55),FVector(23,17,.9f),Ground[Region]);
  for(int32 I=0;I<28;I++){float X=(I%7-3)*360.f,Y=-520-(I/7)*170.f;
   if(Region==0){Shape(Cylinder,FVector(X,Y,160),FVector(.4f,.4f,4),FLinearColor(.06f,.035f,.02f));Shape(Sphere,FVector(X,Y,395),FVector(2.8,2.8,3.5),FLinearColor(.03f,.12f,.065f));}
   else if(Region==1){Shape(Cube,FVector(X,Y,190),FVector(2.5,1.2,5),FLinearColor(.15f,.1f,.065f),FRotator(I%3*7,I*23,0));}
   else if(Region==3){Shape(Cube,FVector(X,Y,165),FVector(.6f,.6f,4.6),FLinearColor(.2f,.3f,.42f),FRotator(I%3*11,I*29,10));}
   else {Shape(Cube,FVector(X,Y,130+I%4*70),FVector(.75f,.85f,3.6+I%4),FLinearColor(.045f,.032f,.06f),FRotator(I%3*8,I*17,0));Shape(Cube,FVector(X,Y,325+I%4*105),FVector(1.6,1.1,.4f),FLinearColor(.05f,.04f,.07f));}
  }
 }
 for(int32 I=0;I<Units.Num();I++){auto U=Units[I]->AsObject();bool Enemy=I>=A(Battle,TEXT("allies")).Num();Homes[I]=FVector((Enemy?1:-1)*(450+float(N(U,TEXT("layer")))*120),(float(N(U,TEXT("column")))-1)*190,0);Bodies[I]->SetVisibility(true);if(S(U,TEXT("id"))==TEXT("akuroth")&&B(U,TEXT("ascended"))&&!Ascended){Ascended=true;TransformAge=0;PlayCue(TEXT("akuroth-transform"),1,.7f);if(Backdrop)Backdrop->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Akuroth/Environment/M_InfernoBackdrop.M_InfernoBackdrop")));}}
 Snapshot=Battle;SetActorHiddenInGame(false);
}
void AEnigmaWorldPresentation::HideBattle(){SetActorHiddenInGame(true);Age=-1;Snapshot.Reset();StopSounds();}
void AEnigmaWorldPresentation::PlayAction(TSharedPtr<FJsonObject> Battle,float Volume){
 if(!Battle)return;
 const FString Actor=S(Battle,TEXT("lastActor"));Attacker=Keys.IndexOfByKey(Actor);Skill=S(Battle,TEXT("lastSkill"));Operator=S(Battle,TEXT("lastOperator"));Duration=Skill==TEXT("akuroth_annihilation")&&A(Battle,TEXT("impacts")).Num()>0?4.6f:2.2f;Victims.Reset();
 for(auto Hit:A(Battle,TEXT("impacts"))){int32 I=Keys.IndexOfByKey(S(Hit->AsObject(),TEXT("target")));if(I>=0)Victims.AddUnique(I);}
 for(auto Buff:A(Battle,TEXT("buffTargets"))){int32 I=Keys.IndexOfByKey(Buff->AsString());if(I>=0)Victims.AddUnique(I);}
 if(Victims.Num()==0){int32 I=Keys.IndexOfByKey(S(Battle,TEXT("lastTarget")));if(I>=0)Victims.Add(I);}
 if(Attacker>=0&&!Skill.IsEmpty())Bodies[Attacker]->PlayAnimation(Attack,false);Age=0;PlayVolume=Volume;SoundEvents.Reset();LastStep=-1;
 FString ActorId;if(Attacker>=0){auto Units=A(Battle,TEXT("allies"));Units.Append(A(Battle,TEXT("enemies")));for(auto U:Units)if(S(U->AsObject(),TEXT("key"))==Actor)ActorId=S(U->AsObject(),TEXT("id"));}
 TSharedPtr<FJsonObject> Profile;const TSharedPtr<FJsonObject>* Found=nullptr;
 if(SoundProfiles){if(SoundProfiles->TryGetObjectField(TEXT("skill:")+Skill,Found))Profile=*Found;else if(SoundProfiles->TryGetObjectField(ActorId.StartsWith(TEXT("enemy"))?TEXT("enemy_fenda"):ActorId,Found))Profile=*Found;}
 FString Cast=Profile?S(Profile,TEXT("Cast")):TEXT("swing"),Impact=Profile?S(Profile,TEXT("Impact")):TEXT("kick");float Pitch=Profile?N(Profile,TEXT("Pitch")):1;
 StepCue=Profile?S(Profile,TEXT("Step")):TEXT("leather");StepPitch=Pitch;
 if(Operator==TEXT("heal")){Cast=TEXT("chime");Impact=TEXT("heal");}
 else if(Operator==TEXT("buff")){Cast=TEXT("chime");Impact=ActorId==TEXT("savor")?TEXT("sizzle"):TEXT("heal");}
 else if(Operator==TEXT("split")){Cast=TEXT("growl");Impact=TEXT("void");}
 else if(Operator==TEXT("horizon")){Cast=TEXT("charge");Impact=TEXT("beam");}
 if(!Skill.IsEmpty()){SoundEvents.Add({.14f,Cast,Pitch,.4f});TSet<int32> Hits;for(auto V:A(Battle,TEXT("impacts"))){auto Hit=V->AsObject();int32 Index=N(Hit,TEXT("hit"));if(!Hits.Contains(Index)){Hits.Add(Index);SoundEvents.Add({.65f+FMath::Min(Index,10)*.08f,Impact,Pitch,.3f});if(B(Hit,TEXT("critical")))SoundEvents.Add({.65f, TEXT("critical"),1,.2f});}}}
 Snapshot=Battle;
}
void AEnigmaWorldPresentation::Ball(UInstancedStaticMeshComponent* Batch,FVector P,float Radius){Batch->AddInstance(FTransform(FRotator::ZeroRotator,P,FVector(Radius/50.f)),true);}
void AEnigmaWorldPresentation::Beam(UInstancedStaticMeshComponent* Batch,FVector A,FVector B,float Width){FVector D=B-A;Batch->AddInstance(FTransform(FQuat::FindBetweenNormals(FVector::UpVector,D.GetSafeNormal()),(A+B)*.5f,FVector(Width/50,Width/50,D.Size()/100)),true);}
void AEnigmaWorldPresentation::Ring(UInstancedStaticMeshComponent* Batch,FVector P,float Radius){for(int32 I=0;I<48;I++){float A=I*2*PI/48,B=(I+1)*2*PI/48;Beam(Batch,P+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,0),P+FVector(FMath::Cos(B)*Radius,FMath::Sin(B)*Radius,0),3);}}
void AEnigmaWorldPresentation::Tick(float Dt){
 Super::Tick(Dt);Clock+=Dt;Effects->ClearInstances();Crimson->ClearInstances();Dark->ClearInstances();ShadowLimbs->ClearInstances();WhiteHair->ClearInstances();if(!Snapshot||IsHidden())return;
 if(Age>=0){Age+=Dt;for(int32 I=SoundEvents.Num()-1;I>=0;I--)if(Age>=SoundEvents[I].At){PlayCue(SoundEvents[I].Cue,SoundEvents[I].Pitch,SoundEvents[I].Gain);SoundEvents.RemoveAt(I);}if(Age<1.3f&&Age-LastStep>.22f&&Attacker>=0&&Victims.Num()>0&&!Skill.IsEmpty()&&Operator!=TEXT("heal")){LastStep=Age;PlayCue(StepCue,StepPitch,.16f);}}if(TransformAge>=0)TransformAge+=Dt;
 TArray<TSharedPtr<FJsonValue>> Units=A(Snapshot,TEXT("allies"));Units.Append(A(Snapshot,TEXT("enemies")));if(Units.Num()!=Bodies.Num())return;
 FVector Center=FVector(0,0,220);bool Ultimate=Skill==TEXT("akuroth_annihilation")&&A(Snapshot,TEXT("impacts")).Num()>0;
 for(int32 I=0;I<Bodies.Num();I++){
  FVector P=Homes[I];auto U=Units[I]->AsObject();if(S(U,TEXT("name")).Contains(TEXT("Espírito")))P+=FVector(0,85,25);bool Boss=S(U,TEXT("id"))==TEXT("akuroth");
  if(I==Attacker&&Age>=0&&Age<1.5f&&!Boss&&!Skill.IsEmpty()&&Victims.Num()>0&&Operator!=TEXT("heal")&&Operator!=TEXT("buff"))P=FMath::Lerp(P,Homes[Victims[0]]+FVector(I<A(Snapshot,TEXT("allies")).Num()?-110:110,0,0),FMath::Sin(FMath::Clamp(Age/1.5f,0.f,1.f)*PI));
  if(Victims.Contains(I)&&Age>=.65f&&Age<1.4f)P.X+=FMath::Sin((Age-.65f)*10)*20;
  if(Ultimate&&Age>=0&&Age<Duration&&I<A(Snapshot,TEXT("allies")).Num())P=FMath::Lerp(P,Center+FVector(-50,(I-2)*30,-110),FMath::Sin(FMath::Clamp(Age/Duration,0.f,1.f)*PI)*.9f);
  if(Boss&&TransformAge>=0&&TransformAge<3)P.Z+=FMath::Sin(TransformAge/3*PI)*120;
  Bodies[I]->SetWorldLocation(P);Bodies[I]->SetWorldRotation(FRotator(0,I<A(Snapshot,TEXT("allies")).Num()?-90:90,(B(U,TEXT("alive"))||Age>=0&&Age<(Ultimate?3.f:.65f)&&Victims.Contains(I))?0:75));
  if(Age>=Duration&&I==Attacker)Bodies[I]->PlayAnimation(Idle,true);
  for(auto V:A(U,TEXT("statuses"))){FString Status=S(V->AsObject(),TEXT("id"));if(Status.Contains(TEXT("poison")))Ring(Effects,P+FVector(0,0,10),75);if(Status.Contains(TEXT("bleed")))Beam(Crimson,P+FVector(0,0,80),P+FVector(0,0,20),4);}
  if(N(U,TEXT("ice"))>0||N(U,TEXT("freeze"))>0)Ring(Effects,P+FVector(0,0,45),80);
  if(Boss){
   FVector Head=Bodies[I]->GetBoneLocation(TEXT("head"));for(int32 H=0;H<2;H++)Beam(ShadowLimbs,Head+FVector(0,H?27:-27,12),Head+FVector(30,H?70:-70,95),10);
   if(B(U,TEXT("ascended"))){for(int32 Arm=0;Arm<6;Arm++){float A=Clock*.6f+Arm*PI/3;FVector Origin=P+FVector(0,0,235),Elbow=Origin+FVector(0,FMath::Cos(A)*140,FMath::Sin(A)*100),Hand=Elbow+FVector(-90,FMath::Cos(A)*90,35);Beam(ShadowLimbs,Origin,Elbow,14);Beam(ShadowLimbs,Elbow,Hand,10);Ring(Crimson,Hand,20);}for(int32 H=0;H<16;H++){FVector Root=Head+FVector(H%4*8-16,H/4*9-18,15);for(int32 J=0;J<6;J++)Beam(WhiteHair,Root+FVector(J*12,FMath::Sin(Clock*2+H+J)*10,-J*28),Root+FVector((J+1)*12,FMath::Sin(Clock*2+H+J+1)*10,-(J+1)*28),3);}}
  }
 }
 if(Age>=0&&Age<Duration&&Attacker>=0&&!Skill.IsEmpty()){
  FVector Source=Bodies[Attacker]->GetComponentLocation()+FVector(0,0,145);auto Batch=Skill.StartsWith(TEXT("akuroth_"))?Crimson:Effects;
  for(int32 V:Victims){FVector P=Homes[V]+FVector(0,0,100);
   if(Operator==TEXT("heal")||Operator==TEXT("buff")||Operator==TEXT("split")||Operator==TEXT("eclipse_guard")){Ring(Effects,Homes[V]+FVector(0,0,20+Age*70),65);for(int32 I=0;I<8;I++)Beam(Effects,P+FVector(FMath::Cos(float(I))*45,FMath::Sin(float(I))*45,-100),P+FVector(FMath::Cos(float(I))*45,FMath::Sin(float(I))*45,Age*85),2);}
   else if(Skill.Contains(TEXT("meteor"))||Skill.Contains(TEXT("rain"))){float T=FMath::Clamp(Age/1.3f,0.f,1.f);FVector M=P+FVector(120*(1-T),0,650*(1-T));Ball(Dark,M,55);Ring(Crimson,M,62);if(T>=1)Ring(Crimson,Homes[V]+FVector(0,0,10),(Age-1.3f)*200+65);}
   else if(Skill.Contains(TEXT("prison"))||Operator==TEXT("floral_thorns")){for(int32 I=0;I<6;I++){FVector Offset=FVector(FMath::Cos(I*PI/3)*70,FMath::Sin(I*PI/3)*70,0);Beam(Batch,Homes[V]+Offset,Homes[V]+Offset+FVector(0,0,240),3);}Ring(Batch,Homes[V]+FVector(0,0,230),75);}
   else if(Skill.Contains(TEXT("rift"))||Skill.Contains(TEXT("rewrite"))){Beam(Batch,P-FVector(0,0,130),P+FVector(FMath::Sin(Age*3)*45,0,210),12);Ring(Effects,Homes[V]+FVector(0,0,20),Age*80);}
   else if(Skill.Contains(TEXT("waves"))||Skill.Contains(TEXT("collapse"))||Operator==TEXT("aoe")){float T=FMath::Clamp(Age/1.1f,0.f,1.f);Ring(Batch,FMath::Lerp(Source,P,T)-FVector(0,0,80),35+T*100);}
   else {float T=FMath::Clamp(Age/.65f,0.f,1.f);FVector Tip=FMath::Lerp(Source,P,T);Beam(Batch,Tip-FVector(50,0,40),Tip+FVector(50,0,40),5);if(T>=1&&Age<1.1f)for(int32 J=0;J<12;J++)Beam(Batch,P,P+FVector(FMath::Cos(float(J))*100,FMath::Sin(float(J))*100,FMath::Sin(float(J*3))*80),2);}
   if(Age>.65f&&Age<1.5f&&Operator!=TEXT("heal")&&Operator!=TEXT("buff")&&Operator!=TEXT("split")&&Operator!=TEXT("eclipse_guard")&&A(Snapshot,TEXT("impacts")).Num()>0)for(int32 J=0;J<18;J++){float T=Age-.65f;FVector R=Homes[V]+FVector(FMath::Cos(J*2.4f)*T*100,FMath::Sin(J*2.4f)*T*100,110+T*80-T*T*220);Beam(Crimson,R,R+FVector(0,0,4),2);}
  }
  if(Skill==TEXT("akuroth_annihilation")&&!Ultimate){Ball(Dark,Source+FVector(0,0,70),45+Age*20);Ring(Crimson,Source+FVector(0,0,70),70+FMath::Sin(Age*3)*15);}
  if(Ultimate){float T=Age/Duration;Ball(Dark,Center,60+FMath::Sin(T*PI)*130);Ring(Crimson,Center,100+FMath::Sin(T*PI)*160);Ring(Effects,Center+FVector(0,0,50),180+FMath::Sin(T*PI)*90);for(int32 I=0;I<30;I++){float A=I*2*PI/30+Age*5;float R=300*(1-T);Beam(Crimson,Center+FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,FMath::Sin(A*3)*40),Center+FVector(FMath::Cos(A+.25f)*R,FMath::Sin(A+.25f)*R,0),3);}if(Age>3.f)Ring(Crimson,FVector(0,0,20),(Age-3.f)*900);}
 }
 if(Age>=Duration)Age=-1;
 if(Camera){float Focus=Transforming()?FMath::Sin(TransformAge/3.f*PI):0;FVector Position=FMath::Lerp(FVector(0,2450,1150),FVector(300,1800,850),Focus*.5f);Camera->SetActorLocation(Position);Camera->SetActorRotation((FMath::Lerp(FVector(0,0,170),FVector(550,0,280),Focus*.5f)-Position).Rotation());Camera->GetCameraComponent()->SetFieldOfView(58-Focus*7);}
 if(Ascended){Light->SetLightColor(FLinearColor(.65f,.1f,.28f));for(int32 I=0;I<65;I++){float X=FMath::Sin(I*16.3f)*1050,Y=FMath::Cos(I*11.7f)*700,Z=FMath::Fmod(Clock*60+I*37,330.f);Beam(Crimson,FVector(X,Y,Z),FVector(X+3,Y,Z+10),2);}if(FMath::Fmod(Clock,4.f)<.18f)for(int32 J=0;J<8;J++)Beam(Effects,FVector(700-J*25,-650,650-J*60),FVector(675-J*25,-650,590-J*60),4);}
}




void AEnigmaWorldPresentation::StopSounds(){SoundEvents.Reset();for(auto Voice:Voices)if(IsValid(Voice))Voice->Stop();Voices.Reset();}
void AEnigmaWorldPresentation::PlayCue(const FString& Cue,float Pitch,float Gain){
 Voices.RemoveAll([](const TObjectPtr<UAudioComponent>& V){return !IsValid(V)||!V->IsPlaying();});if(Voices.Num()>=12)return;
 USoundWave* Wave=nullptr;if(auto Cached=Sounds.Find(Cue))Wave=*Cached;else {const FString Name=TEXT("Cue_")+Cue.Replace(TEXT("-"),TEXT("_"));Wave=LoadObject<USoundWave>(nullptr,*(TEXT("/Game/GameAudio/")+Name+TEXT(".")+Name));Sounds.Add(Cue,Wave);}
 if(!Wave)return;
 const TSharedPtr<FJsonObject>* Sample=nullptr;bool Recorded=SampleProfiles&&SampleProfiles->TryGetObjectField(Cue,Sample);float SampleGain=Recorded?FMath::Pow(10.f,float(N(*Sample,TEXT("Gain")))/20.f):1;
 if(auto Voice=UGameplayStatics::SpawnSound2D(this,Wave,PlayVolume*Gain*SampleGain,Recorded?1.f:Pitch))Voices.Add(Voice);
}
