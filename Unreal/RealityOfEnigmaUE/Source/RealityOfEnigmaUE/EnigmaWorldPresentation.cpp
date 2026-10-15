#include "EnigmaWorldPresentation.h"
#include "EnigmaCombatTiming.h"
#include "Components/DirectionalLightComponent.h"
#include "ProceduralMeshComponent.h"
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
 auto Clip=[&](const TCHAR* Path){return LoadObject<UAnimSequence>(nullptr,Path);};
 Attack2=Clip(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_02.MM_Attack_02"));Attack3=Clip(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_03.MM_Attack_03"));ChargedAttack=Clip(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_ChargedAttack.MM_ChargedAttack"));Walk=Clip(TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Walk/MF_Unarmed_Walk_Fwd.MF_Unarmed_Walk_Fwd"));Death=Clip(TEXT("/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01.MM_Death_Front_01"));
 Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));Attack=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
 auto Batch=[&](const TCHAR* Name,UStaticMesh* Mesh,FLinearColor Color,float Glow){auto C=CreateDefaultSubobject<UInstancedStaticMeshComponent>(Name);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetMaterial(0,Material);return C;};
 Effects=Batch(TEXT("EffectLines"),Cylinder,FLinearColor(.35f,.14f,.9f),2.5f);Crimson=Batch(TEXT("CrimsonLines"),Cylinder,FLinearColor(.65f,.015f,.035f),2.5f);Dark=Batch(TEXT("VoidOrbs"),Sphere,FLinearColor(.003f,.001f,.009f),0);ShadowLimbs=Batch(TEXT("ShadowLimbs"),Cylinder,FLinearColor(.008f,.003f,.015f),0);
 Gold=Batch(TEXT("GoldenDivinity"),Cylinder,FLinearColor(.9f,.5f,.06f),1.4f);Lightning=Batch(TEXT("CelestialLightning"),Cylinder,FLinearColor(.15f,.6f,1),2);Mount=Batch(TEXT("InfernalMount"),Sphere,FLinearColor(.055f,.009f,.014f),0);Petals=Batch(TEXT("FlowerPetals"),Sphere,FLinearColor(.7f,.58f,.25f),.1f);
 WhiteHair=Batch(TEXT("WhiteHair"),Cylinder,FLinearColor(.82f,.8f,.86f),0);
 Light=CreateDefaultSubobject<UPointLightComponent>(TEXT("ArenaLight"));Light->SetupAttachment(RootComponent);Light->SetRelativeLocation(FVector(0,400,800));Light->SetIntensity(65000);Light->SetAttenuationRadius(4500);Light->SetLightColor(FLinearColor(.9f,.85f,.8f));
 KeyLight=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("KeyLight"));KeyLight->SetupAttachment(RootComponent);KeyLight->SetRelativeRotation(FRotator(-38,-90,0));KeyLight->SetIntensity(3.2f);KeyLight->SetForwardShadingPriority(1);KeyLight->SetLightColor(FLinearColor(.95f,.87f,.8f));KeyLight->SetCastShadows(true);
 RimLight=CreateDefaultSubobject<UDirectionalLightComponent>(TEXT("RimLight"));RimLight->SetupAttachment(RootComponent);RimLight->SetRelativeRotation(FRotator(-22,90,0));RimLight->SetIntensity(1.4f);RimLight->SetLightColor(FLinearColor(.4f,.5f,1.f));RimLight->SetCastShadows(false);
 Ready=Material&&Idle&&Attack&&Sphere&&Cylinder&&Cube;
}
void AEnigmaWorldPresentation::BeginPlay(){
 Super::BeginPlay();
 FString Data;if(FFileHelper::LoadFileToString(Data,*(FPaths::ProjectContentDir()/TEXT("GameData/audio.json"))))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),SoundProfiles);
 if(FFileHelper::LoadFileToString(Data,*(FPaths::ProjectContentDir()/TEXT("GameData/audio-samples.json"))))FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),SampleProfiles);
 auto Set=[&](UInstancedStaticMeshComponent* Batch,FLinearColor Color,float Glow){auto M=UMaterialInstanceDynamic::Create(Material,this);M->SetVectorParameterValue(TEXT("Color"),Color);M->SetScalarParameterValue(TEXT("Glow"),Glow);Batch->SetMaterial(0,M);};
 Set(Effects,FLinearColor(.35f,.14f,.9f),2.5f);Set(Crimson,FLinearColor(.65f,.015f,.035f),2.5f);Set(Dark,FLinearColor(.003f,.001f,.009f),0);Set(ShadowLimbs,FLinearColor(.055f,.02f,.085f),.04f);Set(WhiteHair,FLinearColor(.82f,.8f,.86f),0);Set(Gold,FLinearColor(.9f,.5f,.06f),1.4f);Set(Lightning,FLinearColor(.15f,.6f,1),2);Set(Mount,FLinearColor(.055f,.009f,.014f),0);Set(Petals,FLinearColor(.7f,.58f,.25f),.1f);
}
void AEnigmaWorldPresentation::Shape(UStaticMesh* Mesh,FVector P,FVector V,FLinearColor Color,FRotator R){
 auto C=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(RootComponent);C->SetStaticMesh(Mesh);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();C->SetWorldLocation(P);C->SetWorldScale3D(V);C->SetWorldRotation(R);auto M=UMaterialInstanceDynamic::Create(Material,this);M->SetVectorParameterValue(TEXT("Color"),Color);M->SetScalarParameterValue(TEXT("Glow"),0);C->SetMaterial(0,M);Scenery.Add(C);
}
void AEnigmaWorldPresentation::ShowBattle(TSharedPtr<FJsonObject> Battle,int32 Region){
 ArenaRegion=Region;
 if(!Battle||A(Battle,TEXT("allies")).Num()==0){HideBattle();return;}
 TArray<TSharedPtr<FJsonValue>> Units=A(Battle,TEXT("allies"));Units.Append(A(Battle,TEXT("enemies")));bool Rebuild=Units.Num()!=Keys.Num();for(int32 I=0;I<Units.Num()&&!Rebuild;I++)Rebuild|=S(Units[I]->AsObject(),TEXT("key"))!=Keys[I];
 if(!Camera){Camera=GetWorld()->SpawnActor<ACameraActor>();Camera->SetActorLocation(FVector(0,2450,1150));Camera->SetActorRotation((FVector(0,0,170)-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->SetFieldOfView(58);if(auto PC=UGameplayStatics::GetPlayerController(this,0))PC->SetViewTarget(Camera);}
 if(Rebuild){for(auto& Garment:Garments)Garment.Value->DestroyComponent();Garments.Reset();RenderAlive.Reset();LastReact.Reset();ReactUntil.Reset();bool KeepForm=Ascended&&!AscendedBossKey.IsEmpty()&&Units.ContainsByPredicate([this](auto V){return S(V->AsObject(),TEXT("key"))==AscendedBossKey;});if(!KeepForm){TransformBoss.Reset();AscendedBossKey.Reset();Ascended=false;TransformAge=-1;}Age=-1;StopSounds();
  for(auto C:Bodies)C->DestroyComponent();Bodies.Reset();Keys.Reset();Homes.Reset();Colors.Reset();
  auto Manny=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));auto Quinn=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));Ready=Ready&&Manny&&Quinn;
  const FLinearColor Pal[]={FLinearColor(.02f,.42f,.3f),FLinearColor(.05f,.25f,.72f),FLinearColor(.6f,.3f,.05f),FLinearColor(.35f,.025f,.65f),FLinearColor(.65f,.025f,.045f)};
  for(int32 I=0;I<Units.Num();I++){
   auto U=Units[I]->AsObject();FString Id=S(U,TEXT("id"));bool Enemy=I>=A(Battle,TEXT("allies")).Num();auto Body=NewObject<USkeletalMeshComponent>(this);AddInstanceComponent(Body);Body->SetupAttachment(RootComponent);Body->SetSkeletalMesh(Id==TEXT("aelia")||Id==TEXT("lyra")?Quinn:Manny);Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);Body->RegisterComponent();
   FLinearColor Color=Enemy?Id==TEXT("akuroth")?FLinearColor(.09f,.045f,.11f):FLinearColor(.3f,.04f,.075f):Id==TEXT("kael")?Pal[0]:Id==TEXT("aelia")?Pal[1]:Id==TEXT("savor")||Id==TEXT("brakk")?Pal[2]:Id==TEXT("lyra")||Id==TEXT("sevrin")?Pal[3]:Pal[4];if(Id==TEXT("tenrin"))Color=FLinearColor(.55f,.38f,.1f);if(Id==TEXT("zevalon"))Color=FLinearColor(.5f,.65f,.8f);if(Id==TEXT("hajun"))Color=FLinearColor(.4f,.13f,.09f);if(Id==TEXT("mazakin"))Color=FLinearColor(.055f,.015f,.025f);if(Id==TEXT("zhujin"))Color=FLinearColor(.16f,.14f,.06f);auto Mat=UMaterialInstanceDynamic::Create(Material,this);Mat->SetVectorParameterValue(TEXT("Color"),Color);Mat->SetScalarParameterValue(TEXT("Glow"),.02f);for(int32 Slot=0;Slot<Body->GetNumMaterials();Slot++)Body->SetMaterial(Slot,Mat);
   const float Size=Id==TEXT("akuroth")?2.5f:B(U,TEXT("boss"))?1.8f:1.2f;Body->SetWorldScale3D(FVector(Size));Body->SetWorldRotation(FRotator(0,Enemy?90:-90,0));Body->PlayAnimation(Idle,true);Bodies.Add(Body);RenderAlive.Add(B(U,TEXT("alive")));LastReact.Add(-1);ReactUntil.Add(-1);Keys.Add(S(U,TEXT("key")));Colors.Add(Color);Homes.Add(FVector((Enemy?1:-1)*(450+float(N(U,TEXT("layer")))*120),(float(N(U,TEXT("column")))-1)*190,0));
  }
  for(auto C:Scenery)C->DestroyComponent();Scenery.Reset();Light->SetLightColor(FLinearColor(.9f,.85f,.8f));KeyLight->SetLightColor(FLinearColor(.95f,.87f,.8f));RimLight->SetLightColor(FLinearColor(.4f,.5f,1.f));
  auto Plane=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane"));
  Shape(Plane,FVector(0,-1850,600),FVector(50,27,1),FLinearColor::White,FRotator(0,0,90));Backdrop=Scenery.Last();
  if(auto Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/GameArt/M_RegionBackdrop.M_RegionBackdrop"))){auto Mat=UMaterialInstanceDynamic::Create(Parent,this);const FString TexName=FString::Printf(TEXT("Region_%d"),FMath::Clamp(Region,0,6)+1);auto Tex=LoadObject<UTexture>(nullptr,*(TEXT("/Game/GameArt/")+TexName+TEXT(".")+TexName));Mat->SetTextureParameterValue(TEXT("Backdrop"),Tex);Backdrop->SetMaterial(0,Mat);}

  const FLinearColor Ground[]={FLinearColor(.035f,.09f,.055f),FLinearColor(.09f,.075f,.055f),FLinearColor(.06f,.035f,.035f),FLinearColor(.15f,.23f,.28f),FLinearColor(.07f,.05f,.1f),FLinearColor(.025f,.04f,.08f),FLinearColor(.035f,.016f,.025f)};
  Region=FMath::Clamp(Region,0,6);Shape(Cube,FVector(0,0,-55),FVector(23,17,.9f),Ground[Region]);
  for(int32 I=0;I<14;I++){float X=(I%7-3)*400.f,Y=-1150-(I/7)*260.f;
   if(Region==0){Shape(Cylinder,FVector(X,Y,160),FVector(.4f,.4f,4),FLinearColor(.06f,.035f,.02f));Shape(Sphere,FVector(X,Y,395),FVector(2.8,2.8,3.5),FLinearColor(.03f,.12f,.065f));}
   else if(Region==1){Shape(Cube,FVector(X,Y,190),FVector(2.5,1.2,5),FLinearColor(.15f,.1f,.065f),FRotator(I%3*7,I*23,0));}
   else if(Region==3){Shape(Cube,FVector(X,Y,165),FVector(.6f,.6f,4.6),FLinearColor(.2f,.3f,.42f),FRotator(I%3*11,I*29,10));}
   else {Shape(Cube,FVector(X,Y,130+I%4*70),FVector(.75f,.85f,3.6+I%4),FLinearColor(.045f,.032f,.06f),FRotator(I%3*8,I*17,0));Shape(Cube,FVector(X,Y,325+I%4*105),FVector(1.6,1.1,.4f),FLinearColor(.05f,.04f,.07f));}
  }
 }
 for(int32 I=0;I<Units.Num();I++){auto U=Units[I]->AsObject();bool Enemy=I>=A(Battle,TEXT("allies")).Num();Homes[I]=FVector((Enemy?1:-1)*(450+float(N(U,TEXT("layer")))*120),(float(N(U,TEXT("column")))-1)*190,0);Bodies[I]->SetVisibility(true);FString Id=S(U,TEXT("id"));bool BossForm=(Id==TEXT("akuroth")||Id==TEXT("mazakin")||Id==TEXT("hajun"))&&B(U,TEXT("ascended"))||Id==TEXT("zhujin")&&B(U,TEXT("crimson"));if(BossForm&&!Ascended){Ascended=true;TransformBoss=Id;AscendedBossKey=S(U,TEXT("key"));TransformAge=0;PlayCue(Id==TEXT("zhujin")?TEXT("chime"):TEXT("akuroth-transform"),1,.7f);if(Backdrop&&Id!=TEXT("zhujin"))Backdrop->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Akuroth/Environment/M_InfernoBackdrop.M_InfernoBackdrop")));}}
 Snapshot=Battle;SyncTripoArt();SetActorHiddenInGame(false);
}
void AEnigmaWorldPresentation::HideBattle(){SetActorHiddenInGame(true);Age=-1;Snapshot.Reset();StopSounds();}
void AEnigmaWorldPresentation::PlayAction(TSharedPtr<FJsonObject> Battle,float Volume){
 if(!Battle)return;
 BeforeHealth.Reset();if(Snapshot){auto Units=A(Snapshot,TEXT("allies"));Units.Append(A(Snapshot,TEXT("enemies")));for(auto Unit:Units)BeforeHealth.Add(S(Unit->AsObject(),TEXT("key")),N(Unit->AsObject(),TEXT("hp")));}
 auto Updated=A(Battle,TEXT("allies"));Updated.Append(A(Battle,TEXT("enemies")));bool Changed=Updated.Num()!=Keys.Num();for(int32 I=0;I<Updated.Num()&&!Changed;I++)Changed=S(Updated[I]->AsObject(),TEXT("key"))!=Keys[I];if(Changed)ShowBattle(Battle,ArenaRegion);
 const FString Actor=S(Battle,TEXT("lastActor"));Attacker=Keys.IndexOfByKey(Actor);Skill=S(Battle,TEXT("lastSkill"));Operator=S(Battle,TEXT("lastOperator"));Duration=EnigmaCombatTiming::IsAnnihilationDischarge(Battle)?4.6f:2.2f;Victims.Reset();
 for(auto Value:A(Battle,TEXT("impacts"))){auto Hit=Value->AsObject();FString Kind=S(Hit,TEXT("kind")),From=S(Hit,TEXT("source"));if(Kind==TEXT("poison")||Kind==TEXT("bleed")||Kind==TEXT("hazard")||Kind==TEXT("reflect")||Kind==TEXT("heal")&&Operator!=TEXT("heal")||!From.IsEmpty()&&From!=Actor)continue;int32 I=Keys.IndexOfByKey(S(Hit,TEXT("target")));if(I>=0)Victims.AddUnique(I);}
 for(auto Buff:A(Battle,TEXT("buffTargets"))){int32 I=Keys.IndexOfByKey(Buff->AsString());if(I>=0)Victims.AddUnique(I);}
 if(Victims.Num()==0){int32 I=Keys.IndexOfByKey(S(Battle,TEXT("lastTarget")));if(I>=0)Victims.Add(I);}
 if(Attacker>=0&&!Skill.IsEmpty()){UAnimSequence* Clip=Operator==TEXT("reposition")?Walk:Skill.Contains(TEXT("final"))||Skill.Contains(TEXT("wrath"))||Skill.Contains(TEXT("roar"))||Skill.Contains(TEXT("judgment"))?ChargedAttack:Skill.Contains(TEXT("triple"))||Skill.Contains(TEXT("chaos"))?Attack3:Skill.Contains(TEXT("lion"))||Skill.Contains(TEXT("axe"))?Attack2:Attack;Bodies[Attacker]->PlayAnimation(IsTripoBody(Attacker)?BodyClip(Attacker,Skill.Contains(TEXT("annihilation"))?TEXT("Ultimate"):Skill.Contains(TEXT("chaos"))?TEXT("Chaos"):Skill.Contains(TEXT("cut"))?TEXT("Slash"):TEXT("Cast")):(Clip?Clip:Attack.Get()),Operator==TEXT("reposition"));}LastReact.Init(-1,Bodies.Num());ReactUntil.Init(-1,Bodies.Num());Age=0;PlayVolume=Volume;SoundEvents.Reset();LastStep=-1;
 FString ActorId;if(Attacker>=0){auto Units=A(Battle,TEXT("allies"));Units.Append(A(Battle,TEXT("enemies")));for(auto U:Units)if(S(U->AsObject(),TEXT("key"))==Actor)ActorId=S(U->AsObject(),TEXT("id"));}
 TSharedPtr<FJsonObject> Profile;const TSharedPtr<FJsonObject>* Found=nullptr;
 if(SoundProfiles){if(SoundProfiles->TryGetObjectField(TEXT("skill:")+Skill,Found))Profile=*Found;else if(SoundProfiles->TryGetObjectField(ActorId.StartsWith(TEXT("enemy"))?TEXT("enemy_fenda"):ActorId,Found))Profile=*Found;}
 FString Cast=Profile?S(Profile,TEXT("Cast")):TEXT("swing"),Impact=Profile?S(Profile,TEXT("Impact")):TEXT("kick");float Pitch=Profile?N(Profile,TEXT("Pitch")):1;
 StepCue=Profile?S(Profile,TEXT("Step")):TEXT("leather");StepPitch=Pitch;
 if(Operator==TEXT("heal")){Cast=TEXT("chime");Impact=TEXT("heal");}
 else if(Operator==TEXT("buff")){Cast=TEXT("chime");Impact=ActorId==TEXT("savor")?TEXT("sizzle"):TEXT("heal");}
 else if(Operator==TEXT("split")){Cast=TEXT("growl");Impact=TEXT("void");}
 else if(Operator==TEXT("horizon")){Cast=TEXT("charge");Impact=TEXT("beam");}
 if(!Skill.IsEmpty()){
  SoundEvents.Add({.14f,Cast,Pitch,.4f});TSet<FString> Events;
  for(auto V:A(Battle,TEXT("impacts"))){auto Hit=V->AsObject();float At=EnigmaCombatTiming::ImpactAt(Skill,Hit);FString Kind=S(Hit,TEXT("kind"));FString Cue=Kind==TEXT("heal")?TEXT("heal"):Kind==TEXT("poison")?TEXT("sizzle"):Kind==TEXT("bleed")?TEXT("kick"):S(Hit,TEXT("effect")).StartsWith(TEXT("mazakin_"))?TEXT("akuroth-meteor-impact"):Impact;
   FString Event=FString::Printf(TEXT("%d:"),FMath::RoundToInt(At*1000))+Cue;
   if(!Events.Contains(Event)){Events.Add(Event);SoundEvents.Add({At,Cue,Pitch,.3f});}
   if(B(Hit,TEXT("critical"))){Event=FString::Printf(TEXT("%d:critical"),FMath::RoundToInt(At*1000));if(!Events.Contains(Event)){Events.Add(Event);SoundEvents.Add({At,TEXT("critical"),1,.2f});}}
  }
 }
 Snapshot=Battle;
}
void AEnigmaWorldPresentation::Ball(UInstancedStaticMeshComponent* Batch,FVector P,float Radius){Batch->AddInstance(FTransform(FRotator::ZeroRotator,P,FVector(Radius/50.f)),true);}
void AEnigmaWorldPresentation::Beam(UInstancedStaticMeshComponent* Batch,FVector A,FVector B,float Width){FVector D=B-A;Batch->AddInstance(FTransform(FQuat::FindBetweenNormals(FVector::UpVector,D.GetSafeNormal()),(A+B)*.5f,FVector(Width/50,Width/50,D.Size()/100)),true);}
void AEnigmaWorldPresentation::Ring(UInstancedStaticMeshComponent* Batch,FVector P,float Radius){for(int32 I=0;I<48;I++){float A=I*2*PI/48,B=(I+1)*2*PI/48;Beam(Batch,P+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,0),P+FVector(FMath::Cos(B)*Radius,FMath::Sin(B)*Radius,0),3);}}
void AEnigmaWorldPresentation::Tick(float Dt){
 Super::Tick(Dt);Clock+=Dt;Effects->ClearInstances();Crimson->ClearInstances();Dark->ClearInstances();ShadowLimbs->ClearInstances();WhiteHair->ClearInstances();Gold->ClearInstances();Lightning->ClearInstances();Mount->ClearInstances();Petals->ClearInstances();if(!Snapshot||IsHidden())return;
 if(Age>=0){Age+=Dt;for(int32 I=SoundEvents.Num()-1;I>=0;I--)if(Age>=SoundEvents[I].At){PlayCue(SoundEvents[I].Cue,SoundEvents[I].Pitch,SoundEvents[I].Gain);SoundEvents.RemoveAt(I);}if(Age<1.3f&&Age-LastStep>.22f&&Attacker>=0&&Victims.Num()>0&&!Skill.IsEmpty()&&Operator!=TEXT("heal")&&Operator!=TEXT("buff")&&!Skill.StartsWith(TEXT("akuroth_"))){LastStep=Age;PlayCue(StepCue,StepPitch,.16f);}}if(TransformAge>=0)TransformAge+=Dt;
 if(TripoTransformPending&&TransformAge>=3){for(int32 I=0;I<Bodies.Num();I++)if(IsTripoBody(I))Bodies[I]->PlayAnimation(BodyClip(I,TEXT("Idle")),true);TripoTransformPending=false;}
 TArray<TSharedPtr<FJsonValue>> Units=A(Snapshot,TEXT("allies"));Units.Append(A(Snapshot,TEXT("enemies")));if(Units.Num()!=Bodies.Num())return;
 FVector Center=FVector(0,0,220);bool Ultimate=EnigmaCombatTiming::IsAnnihilationDischarge(Snapshot);
 for(int32 I=0;I<Bodies.Num();I++){
  FVector P=Homes[I];auto U=Units[I]->AsObject();if(S(U,TEXT("name")).Contains(TEXT("Espírito")))P+=FVector(0,85,25);FString Id=S(U,TEXT("id"));bool Boss=Id==TEXT("akuroth")||Id==TEXT("mazakin")||Id==TEXT("hajun")||Id==TEXT("zhujin");
  if(I==Attacker&&Age>=0&&Age<1.5f&&!Boss&&!Skill.IsEmpty()&&Victims.Num()>0&&Operator!=TEXT("heal")&&Operator!=TEXT("buff")&&Operator!=TEXT("reposition")&&Operator!=TEXT("divine_guard"))P=FMath::Lerp(P,Homes[Victims[0]]+FVector(I<A(Snapshot,TEXT("allies")).Num()?-110:110,0,0),FMath::Sin(FMath::Clamp(Age/1.5f,0.f,1.f)*PI));
  float HitAge=-1;for(auto V:A(Snapshot,TEXT("impacts"))){auto Hit=V->AsObject();if(S(Hit,TEXT("target"))==Keys[I]&&S(Hit,TEXT("kind"))!=TEXT("heal")){float At=EnigmaCombatTiming::ImpactAt(Skill,Hit);if(Age>=At&&Age<At+.75f)HitAge=Age-At;}}
  if(HitAge>=0)P.X+=FMath::Sin(HitAge*10)*20;
  if(Ultimate&&Age>=0&&Age<Duration&&I<A(Snapshot,TEXT("allies")).Num())P=FMath::Lerp(P,Center+FVector(-50,(I-2)*30,-110),FMath::Sin(FMath::Clamp(Age/Duration,0.f,1.f)*PI)*.9f);
  if(I==Attacker&&Operator==TEXT("reposition")&&Age>=0){FVector Goal=FVector((I<A(Snapshot,TEXT("allies")).Num()?-1:1)*(450+float(N(U,TEXT("layer")))*120),(float(N(U,TEXT("column")))-1)*190,0);P=FMath::Lerp(Homes[I],Goal,FMath::SmoothStep(0.f,1.f,FMath::Clamp(Age/.85f,0.f,1.f)));}
  if(I==Attacker&&Skill==TEXT("mazakin_charge")&&Age>=0&&Age<1.5f&&Victims.Num()>0)P=FMath::Lerp(P,Homes[Victims[0]]+FVector(160,0,0),FMath::Sin(FMath::Clamp(Age/1.5f,0.f,1.f)*PI));
  if(Id==TEXT("mazakin")&&B(U,TEXT("ascended")))P.Z+=80;
  if(Id==TEXT("mazakin_orb")){int32 SummonerIndex=Keys.IndexOfByKey(S(U,TEXT("summoner")));if(SummonerIndex>=0){float A=Clock*.6f+float(N(U,TEXT("column")))*2*PI/3;P=Homes[SummonerIndex]+FVector(FMath::Cos(A)*220,FMath::Sin(A)*190,70);}}
  if(Boss&&!ReducedMotion&&TransformAge>=0&&TransformAge<3)P.Z+=FMath::Sin(TransformAge/3*PI)*120;
  const double* Initial=BeforeHealth.Find(Keys[I]);bool AliveOnScreen=Age>=0&&Initial?EnigmaCombatTiming::HealthAt(*Initial,N(U,TEXT("hp")),N(U,TEXT("maxHp")),Keys[I],Skill,A(Snapshot,TEXT("impacts")),Age,Duration)>0:B(U,TEXT("alive"));
  if(Id==TEXT("mazakin")&&B(U,TEXT("ascended"))&&!AliveOnScreen)P.Z-=80;
  Bodies[I]->SetWorldLocation(P);Bodies[I]->SetWorldRotation(FRotator(0,I<A(Snapshot,TEXT("allies")).Num()?-90:IsTripoBody(I)?45:90,AliveOnScreen?(HitAge>=0?FMath::Sin(FMath::Clamp(HitAge/.42f,0.f,1.f)*PI)*-10:0):Death?0:75));
  if(!AliveOnScreen&&RenderAlive[I]&&Death)Bodies[I]->PlayAnimation(IsTripoBody(I)?BodyClip(I,TEXT("Death")):Death.Get(),false);
  RenderAlive[I]=AliveOnScreen;Bodies[I]->SetVisibility(Id!=TEXT("mazakin_orb"));
  if(AliveOnScreen&&I!=Attacker&&HitAge>=0&&Age-HitAge>LastReact[I]+.001f ){LastReact[I]=Age-HitAge;ReactUntil[I]=Age+.42f;}
  if(ReactUntil[I]>0&&Age>=ReactUntil[I]&&AliveOnScreen){ReactUntil[I]=-1;Bodies[I]->PlayAnimation(IsTripoBody(I)?BodyClip(I,TEXT("Idle")):Idle.Get(),true);}
  if(Id==TEXT("mazakin_orb")&&AliveOnScreen){Ball(Dark,P+FVector(0,0,100),65);FaceRing(Crimson,P+FVector(0,0,100),72);}
  DrawNewCharacter(I,U,P);
  if(Age>=Duration&&I==Attacker&&AliveOnScreen)Bodies[I]->PlayAnimation(IsTripoBody(I)?BodyClip(I,TEXT("Idle")):Idle.Get(),true);
  for(auto V:A(U,TEXT("statuses"))){FString Status=S(V->AsObject(),TEXT("id"));if(Status.Contains(TEXT("poison")))Ring(Effects,P+FVector(0,0,10),75);if(Status.Contains(TEXT("bleed")))Beam(Crimson,P+FVector(0,0,80),P+FVector(0,0,20),4);}
  if(N(U,TEXT("ice"))>0||N(U,TEXT("freeze"))>0)Ring(Effects,P+FVector(0,0,45),80);
  if(Id==TEXT("akuroth")&&!IsTripoBody(I)){
   DrawAkurothCostume(I,U);
   FVector Head=Bodies[I]->GetBoneLocation(TEXT("head"));for(int32 H=0;H<2;H++)Beam(ShadowLimbs,Head+FVector(0,H?27:-27,12),Head+FVector(30,H?70:-70,95),10);
   if(B(U,TEXT("ascended"))){for(int32 Arm=0;Arm<6;Arm++){float A=Clock*.6f+Arm*PI/3;FVector Origin=P+FVector(0,0,235),Elbow=Origin+FVector(0,FMath::Cos(A)*140,FMath::Sin(A)*100),Hand=Elbow+FVector(-90,FMath::Cos(A)*90,35);Beam(ShadowLimbs,Origin,Elbow,14);Beam(ShadowLimbs,Elbow,Hand,10);Ring(Crimson,Hand,20);}for(int32 H=0;H<16;H++){FVector Root=Head+FVector(H%4*8-16,H/4*9-18,15);for(int32 J=0;J<6;J++)Beam(WhiteHair,Root+FVector(J*12,FMath::Sin(Clock*2+H+J)*10,-J*28),Root+FVector((J+1)*12,FMath::Sin(Clock*2+H+J+1)*10,-(J+1)*28),3);}}
  }
 }
 if(Age>=0&&Age<Duration&&Attacker>=0&&!Skill.IsEmpty()){
  FVector Source=Bodies[Attacker]->GetComponentLocation()+FVector(0,0,145);auto Batch=Skill.StartsWith(TEXT("akuroth_"))?Crimson:Effects;
  DrawNewSkill(Source);
  for(int32 V:Victims){FVector P=Bodies[V]->GetComponentLocation()+FVector(0,0,100);
   if(Ultimate||Operator==TEXT("reposition")||Operator==TEXT("divine_guard")||Skill.StartsWith(TEXT("tenrin_"))||Skill.StartsWith(TEXT("zevalon_"))){}
   else if(Operator==TEXT("heal")||Operator==TEXT("buff")||Operator==TEXT("split")||Operator==TEXT("eclipse_guard")){Ring(Effects,Homes[V]+FVector(0,0,20+Age*70),65);for(int32 I=0;I<8;I++)Beam(Effects,P+FVector(FMath::Cos(float(I))*45,FMath::Sin(float(I))*45,-100),P+FVector(FMath::Cos(float(I))*45,FMath::Sin(float(I))*45,Age*85),2);}
   else if(Skill.Contains(TEXT("meteor"))||Skill.Contains(TEXT("rain"))){float T=FMath::Clamp(Age/1.3f,0.f,1.f);FVector M=P+FVector(120*(1-T),0,650*(1-T));Ball(Dark,M,55);Ring(Crimson,M,62);if(T>=1)Ring(Crimson,Homes[V]+FVector(0,0,10),(Age-1.3f)*200+65);}
   else if(Skill.Contains(TEXT("prison"))||Skill.Contains(TEXT("chains"))||Operator==TEXT("floral_thorns")){for(int32 I=0;I<6;I++){FVector Offset=FVector(FMath::Cos(I*PI/3)*70,FMath::Sin(I*PI/3)*70,0);Beam(Batch,Homes[V]+Offset,Homes[V]+Offset+FVector(0,0,240),3);}Ring(Batch,Homes[V]+FVector(0,0,230),75);}
   else if(Skill.Contains(TEXT("rift"))||Skill.Contains(TEXT("rewrite"))){Beam(Batch,P-FVector(0,0,130),P+FVector(FMath::Sin(Age*3)*45,0,210),12);Ring(Effects,Homes[V]+FVector(0,0,20),Age*80);}
   else if(Skill.Contains(TEXT("waves"))||Skill.Contains(TEXT("collapse"))||Operator==TEXT("aoe")){float T=FMath::Clamp(Age/1.1f,0.f,1.f);Ring(Batch,FMath::Lerp(Source,P,T)-FVector(0,0,80),35+T*100);}
   else {float T=FMath::Clamp(Age/.65f,0.f,1.f);FVector Tip=FMath::Lerp(Source,P,T);Beam(Batch,Tip-FVector(50,0,40),Tip+FVector(50,0,40),5);if(T>=1&&Age<1.1f)for(int32 J=0;J<12;J++)Beam(Batch,P,P+FVector(FMath::Cos(float(J))*100,FMath::Sin(float(J))*100,FMath::Sin(float(J*3))*80),2);}
   float BloodAge=-1;for(auto HitValue:A(Snapshot,TEXT("impacts"))){auto Hit=HitValue->AsObject();if(S(Hit,TEXT("target"))==Keys[V]&&S(Hit,TEXT("kind"))!=TEXT("heal")&&N(Hit,TEXT("amount"))>0){float At=EnigmaCombatTiming::ImpactAt(Skill,Hit);if(Age>=At&&Age<At+.75f)BloodAge=Age-At;}}
   if(BloodAge>=0)for(int32 J=0;J<18;J++){float T=BloodAge;FVector R=Bodies[V]->GetComponentLocation()+FVector(FMath::Cos(J*2.4f)*T*100,FMath::Sin(J*2.4f)*T*100,110+T*80-T*T*220);Beam(Crimson,R,R+FVector(0,0,4),2);}
  }
  if(Skill==TEXT("akuroth_annihilation")&&!Ultimate){Ball(Dark,Source+FVector(-160,0,175),45+Age*20);Ring(Crimson,Source+FVector(-160,0,175),70+FMath::Sin(Age*3)*15);}
  if(Ultimate){float T=Age/Duration;Ball(Dark,Center,60+FMath::Sin(T*PI)*130);Ring(Crimson,Center,100+FMath::Sin(T*PI)*160);Ring(Effects,Center+FVector(0,0,50),180+FMath::Sin(T*PI)*90);for(int32 I=0;I<30;I++){float A=I*2*PI/30+Age*5;float R=300*(1-T);Beam(Crimson,Center+FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,FMath::Sin(A*3)*40),Center+FVector(FMath::Cos(A+.25f)*R,FMath::Sin(A+.25f)*R,0),3);}if(Age>3.f)Ring(Crimson,FVector(0,0,20),(Age-3.f)*900);}
 }
 if(Age>=Duration){for(int32 I=0;I<Bodies.Num();I++)if(RenderAlive[I])Bodies[I]->PlayAnimation(IsTripoBody(I)?BodyClip(I,TEXT("Idle")):Idle.Get(),true);Age=-1;}
  for(auto Value:A(Snapshot,TEXT("impacts"))){auto Hit=Value->AsObject();float At=EnigmaCombatTiming::ImpactAt(Skill,Hit);int32 TargetIndex=Keys.IndexOfByKey(S(Hit,TEXT("target")));if(TargetIndex<0||Age<At||Age>=At+.65f)continue;FVector P=Bodies[TargetIndex]->GetComponentLocation()+FVector(0,0,110);if(S(Hit,TEXT("kind"))==TEXT("heal")){Ring(Gold,P-FVector(0,0,90),50+(Age-At)*100);}else if(S(Hit,TEXT("effect")).StartsWith(TEXT("mazakin_"))){FaceRing(Crimson,P,50+(Age-At)*100);Beam(Crimson,P+FVector(0,0,300),P,7);}}
 for(auto Hazard:A(Snapshot,TEXT("hazards"))){auto H=Hazard->AsObject();int32 Cell=N(H,TEXT("cell"));FVector P((B(H,TEXT("allyTargets"))?-1:1)*(450+float(Cell/3)*120),(Cell%3-1)*190,5);FaceRing(Crimson,P+FVector(0,0,10),25);Ring(Crimson,P,86);for(int32 J=0;J<12;J++){float A=J*2*PI/12;Beam(Crimson,P+FVector(FMath::Cos(A)*55,FMath::Sin(A)*55,0),P+FVector(FMath::Cos(A)*55,FMath::Sin(A)*55,25+FMath::Sin(Clock*4+J)*20),3);}}
 if(Camera){float Focus=!ReducedMotion&&Transforming()?FMath::Sin(TransformAge/3.f*PI):0;FVector Position=FMath::Lerp(TripoArenaEnabled?FVector(0,2250,2100):FVector(0,2450,1150),TripoArenaEnabled?FVector(300,1950,1750):FVector(300,1800,850),Focus*.5f);Camera->SetActorLocation(Position);Camera->SetActorRotation((FMath::Lerp(FVector(0,0,170),FVector(550,0,280),Focus*.5f)-Position).Rotation());Camera->GetCameraComponent()->SetFieldOfView(58-Focus*7);}
 if(TripoArenaEnabled){for(int32 J=0;J<36;J++){float A=J*2*PI/36;float R=TripoArenaPhase==2?1030:1150;float Z=FMath::Fmod(Clock*75+J*29,150.f);FVector P(FMath::Cos(A)*R,FMath::Sin(A)*R,5+Z);Beam(Crimson,P,P+FVector(FMath::Sin(Clock*5+J)*7,0,12),2);}}
 if(Ascended){Light->SetLightColor(FLinearColor(.65f,.1f,.28f));KeyLight->SetLightColor(TransformBoss==TEXT("zhujin")?FLinearColor(.6f,.9f,.55f):FLinearColor(.9f,.58f,.65f));RimLight->SetLightColor(TransformBoss==TEXT("zhujin")?FLinearColor(.5f,.6f,.12f):FLinearColor(.6f,.22f,.95f));for(int32 I=0;I<65;I++){float X=FMath::Sin(I*16.3f)*1050,Y=FMath::Cos(I*11.7f)*700,Z=FMath::Fmod(Clock*60+I*37,330.f);Beam(Crimson,FVector(X,Y,Z),FVector(X+3,Y,Z+10),2);}if(!ReducedMotion&&FMath::Fmod(Clock,4.f)<.18f)for(int32 J=0;J<8;J++)Beam(Effects,FVector(700-J*25,-650,650-J*60),FVector(675-J*25,-650,590-J*60),4);}
}




void AEnigmaWorldPresentation::StopSounds(){SoundEvents.Reset();for(auto Voice:Voices)if(IsValid(Voice))Voice->Stop();Voices.Reset();}
void AEnigmaWorldPresentation::PlayCue(const FString& Cue,float Pitch,float Gain){
 Voices.RemoveAll([](const TObjectPtr<UAudioComponent>& V){return !IsValid(V)||!V->IsPlaying();});if(Voices.Num()>=12)return;
 USoundWave* Wave=nullptr;if(auto Cached=Sounds.Find(Cue))Wave=*Cached;else {const FString Name=TEXT("Cue_")+Cue.Replace(TEXT("-"),TEXT("_"));Wave=LoadObject<USoundWave>(nullptr,*(TEXT("/Game/GameAudio/")+Name+TEXT(".")+Name));Sounds.Add(Cue,Wave);}
 if(!Wave)return;
 const TSharedPtr<FJsonObject>* Sample=nullptr;bool Recorded=SampleProfiles&&SampleProfiles->TryGetObjectField(Cue,Sample);float SampleGain=Recorded?FMath::Pow(10.f,float(N(*Sample,TEXT("Gain")))/20.f):1;
 if(auto Voice=UGameplayStatics::SpawnSound2D(this,Wave,PlayVolume*Gain*SampleGain,Recorded?1.f:Pitch))Voices.Add(Voice);
}

bool AEnigmaWorldPresentation::HasImpactCueAt(float At) const{return SoundEvents.ContainsByPredicate([At](const FSoundEvent& Event){return FMath::IsNearlyEqual(Event.At,At,.001f);});}
