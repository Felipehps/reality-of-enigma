#include "EnigmaStoryEnvironment.h"
#include "ProceduralMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/TextureCube.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Dom/JsonObject.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

namespace Scenery {
// Geometry is generated once per location/phase. Ambient particles are instanced.
struct Mesh {
 TArray<FVector> V,N;TArray<int32> T;TArray<FVector2D> UV;TArray<FLinearColor> C;TArray<FProcMeshTangent> Tangents;
 void Tri(FVector A,FVector B,FVector D){
  const int32 I=V.Num();const FVector Normal=FVector::CrossProduct(B-A,D-A).GetSafeNormal();
  const bool ProjectX=FMath::Abs(Normal.X)>FMath::Abs(Normal.Z)&&FMath::Abs(Normal.X)>FMath::Abs(Normal.Y);
  const bool ProjectZ=FMath::Abs(Normal.Z)>=FMath::Abs(Normal.X)&&FMath::Abs(Normal.Z)>=FMath::Abs(Normal.Y);
  FVector Tangent=ProjectX?FVector::YAxisVector:FVector::XAxisVector;Tangent=(Tangent-Normal*FVector::DotProduct(Tangent,Normal)).GetSafeNormal();
  for(const FVector& P:{A,B,D}){V.Add(P);N.Add(Normal);UV.Add((ProjectZ?FVector2D(P.X,P.Y):ProjectX?FVector2D(P.Y,P.Z):FVector2D(P.X,P.Z))*.002);Tangents.Add(FProcMeshTangent(Tangent,false));}
  T.Append({I,I+1,I+2});
 }
 void Quad(FVector A,FVector B,FVector D,FVector E){Tri(A,B,D);Tri(A,D,E);}
 void Box(FVector P,FVector Size,FRotator R=FRotator::ZeroRotator){
  FVector Q[8];for(int I=0;I<8;I++)Q[I]=P+R.RotateVector(FVector(I&1?Size.X:-Size.X,I&2?Size.Y:-Size.Y,I&4?Size.Z:-Size.Z)*.5);
  Quad(Q[0],Q[2],Q[3],Q[1]);Quad(Q[4],Q[5],Q[7],Q[6]);Quad(Q[0],Q[1],Q[5],Q[4]);Quad(Q[2],Q[6],Q[7],Q[3]);Quad(Q[0],Q[4],Q[6],Q[2]);Quad(Q[1],Q[3],Q[7],Q[5]);
 }
 void Tube(FVector A,FVector B,float R0,float R1,int Sides=9){
  const FVector Axis=(B-A).GetSafeNormal();FVector U,W;Axis.FindBestAxisVectors(U,W);
  for(int I=0;I<Sides;I++){
   const float X=I*2*PI/Sides,Y=(I+1)*2*PI/Sides;
   FVector D=U*FMath::Cos(X)+W*FMath::Sin(X),E=U*FMath::Cos(Y)+W*FMath::Sin(Y);
   Quad(A+D*R0,A+E*R0,B+E*R1,B+D*R1);Tri(A,A+E*R0,A+D*R0);Tri(B,B+D*R1,B+E*R1);
  }
 }
 void Arch(FVector Center,float Radius,float Width,float Depth,float Start=0,float End=PI,int Segments=28){
  for(int I=0;I<Segments;I++){
   const float A=FMath::Lerp(Start,End,float(I)/Segments),B=FMath::Lerp(Start,End,float(I+1)/Segments);
   FVector Q[8];for(int J=0;J<8;J++){const float Angle=J&1?B:A,R=J&2?Radius+Width:Radius;Q[J]=Center+FVector(FMath::Cos(Angle)*R,J&4?Depth*.5:-Depth*.5,FMath::Sin(Angle)*R);}
   Quad(Q[0],Q[1],Q[3],Q[2]);Quad(Q[4],Q[6],Q[7],Q[5]);Quad(Q[0],Q[4],Q[5],Q[1]);Quad(Q[2],Q[3],Q[7],Q[6]);Quad(Q[0],Q[2],Q[6],Q[4]);Quad(Q[1],Q[5],Q[7],Q[3]);
  }
 }
 void Rock(FVector P,FVector Size,int Seed){
  FRandomStream Rand(Seed);constexpr int S=9;FVector Rings[3][S];
  for(int J=0;J<3;J++)for(int I=0;I<S;I++){float A=I*2*PI/S,R=Rand.FRandRange(.75f,1.15f)*(J==2?.38f:1.f);Rings[J][I]=P+FVector(FMath::Cos(A)*Size.X*R,FMath::Sin(A)*Size.Y*R,Size.Z*(J*.5f)+Rand.FRandRange(-.08f,.08f)*Size.Z);}
  for(int I=0;I<S;I++){int K=(I+1)%S;for(int J=0;J<2;J++)Quad(Rings[J][I],Rings[J][K],Rings[J+1][K],Rings[J+1][I]);Tri(P+FVector(0,0,Size.Z),Rings[2][I],Rings[2][K]);}
 }
 void Leaf(FVector P,float A,float Length,float Width,float Lift){
  FVector D(FMath::Cos(A),FMath::Sin(A),0),W(-D.Y,D.X,0);
  FVector Tip=P+D*Length+FVector(0,0,Lift),Mid=P+D*Length*.43+FVector(0,0,Lift*.8+12);
  Tri(P,Mid-W*Width,Mid+FVector(0,0,7));Tri(Mid-W*Width,Tip,Mid+FVector(0,0,7));Tri(Tip,Mid+W*Width,Mid+FVector(0,0,7));Tri(Mid+W*Width,P,Mid+FVector(0,0,7));
 }
};
FLinearColor Color(TSharedPtr<FJsonObject> O,const TCHAR* Key){const auto& A=O->GetArrayField(Key);return FLinearColor(A[0]->AsNumber(),A[1]->AsNumber(),A[2]->AsNumber());}
}
const TArray<FEnigmaEnvironment>& AEnigmaStoryEnvironment::Definitions(){
 static TArray<FEnigmaEnvironment> Rows;if(Rows.Num())return Rows;
 FString Data;TArray<TSharedPtr<FJsonValue>> Values;
 if(!FFileHelper::LoadFileToString(Data,*(FPaths::ProjectContentDir()/TEXT("GameData/environments.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),Values))return Rows;
 for(auto V:Values){auto O=V->AsObject();FEnigmaEnvironment D;D.Id=O->GetStringField(TEXT("Id"));D.Title=O->GetStringField(TEXT("Title"));D.Story=O->GetStringField(TEXT("Story"));D.Theme=O->GetStringField(TEXT("Theme"));D.Region=O->GetIntegerField(TEXT("Region"));D.HasPhase=O->GetBoolField(TEXT("HasPhase"));D.Stone=Scenery::Color(O,TEXT("Stone"));D.Accent=Scenery::Color(O,TEXT("Accent"));D.Vegetation=Scenery::Color(O,TEXT("Vegetation"));D.Sky=Scenery::Color(O,TEXT("Sky"));Rows.Add(D);}
 return Rows;
}
FString AEnigmaStoryEnvironment::ForBattle(int32 Region,const TArray<TSharedPtr<FJsonValue>>& Enemies){
 for(auto E:Enemies){FString Id;E->AsObject()->TryGetStringField(TEXT("id"),Id);for(const auto& D:Definitions())if(D.HasPhase&&D.Id==Id)return Id;}
 for(const auto& D:Definitions())if(D.Region==Region)return D.Id;
 return TEXT("forest");
}
AEnigmaStoryEnvironment::AEnigmaStoryEnvironment(){
 PrimaryActorTick.bCanEverTick=true;
 Architecture=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("StoryArchitecture"));SetRootComponent(Architecture);Architecture->SetCollisionEnabled(ECollisionEnabled::NoCollision);Architecture->bUseAsyncCooking=true;
 Horizon=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DistantHorizon"));Horizon->SetupAttachment(RootComponent);Horizon->SetCastShadow(false);Horizon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 AmbientLight=CreateDefaultSubobject<USkyLightComponent>(TEXT("StoryAmbientLight"));AmbientLight->SetupAttachment(RootComponent);AmbientLight->SourceType=SLS_SpecifiedCubemap;AmbientLight->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap")));AmbientLight->SetIntensity(.9f);AmbientLight->SetCastShadows(false);
 Motes=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("AmbientMotes"));Motes->SetupAttachment(RootComponent);Motes->SetCollisionEnabled(ECollisionEnabled::NoCollision);Motes->SetCastShadow(false);Motes->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
 Atmosphere=CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("StoryAtmosphere"));Atmosphere->SetupAttachment(RootComponent);Atmosphere->SetRelativeLocation(FVector(0,0,-150));Atmosphere->SetFogDensity(.014f);Atmosphere->SetFogHeightFalloff(.3f);Atmosphere->SetStartDistance(1300);Atmosphere->SetFogMaxOpacity(.65f);
}
FLinearColor AEnigmaStoryEnvironment::KeyColor() const{return Phase?FLinearColor(.78f,.23f,.18f):Definition.Theme==TEXT("snow")?FLinearColor(.55f,.72f,1):FLinearColor(.88f,.78f,.62f);}
FLinearColor AEnigmaStoryEnvironment::RimColor() const{return Phase?FLinearColor(.65f,.08f,.3f):Definition.Accent;}
bool AEnigmaStoryEnvironment::Configure(const FString& Id,bool PhaseTwo){
 if(CurrentId==Id&&Phase==PhaseTwo&&TriangleCount>0)return true;
 const FEnigmaEnvironment* Found=Definitions().FindByPredicate([&](const FEnigmaEnvironment& D){return D.Id==Id;});if(!Found)return false;
 Definition=*Found;CurrentId=Id;Phase=PhaseTwo&&Found->HasPhase;AmbientTime=0;Pulse=0;
 Architecture->ClearAllMeshSections();Horizon->ClearAllMeshSections();for(auto Lamp:Lamps)Lamp->DestroyComponent();Lamps.Reset();Materials.Reset();Motes->ClearInstances();MoteSeeds.Reset();
 using namespace Scenery;Mesh Geo[8];FRandomStream Rand(1741+GetTypeHash(Id)%10000);
 const FString Theme=Definition.Theme;bool Forest=Theme==TEXT("forest")||Theme==TEXT("meadow"),Snow=Theme==TEXT("snow"),Water=Theme==TEXT("archive")||Theme==TEXT("garden"),Infernal=Theme==TEXT("tribunal")||Theme==TEXT("abyss")||Theme==TEXT("void");
 FLinearColor Stone=Definition.Stone*(Phase?.85f:1.f),Accent=Phase?FLinearColor(1,.018f,.055f):Definition.Accent,Plant=Phase?FLinearColor(.30f,.008f,.04f):Definition.Vegetation;
 FLinearColor Palette[]={Stone,Stone*1.32f,Plant,FLinearColor(.11f,.055f,.028f),Accent,Phase?FLinearColor(.035f,.002f,.012f):Definition.Sky,FLinearColor(.008f,.055f,.075f),FLinearColor(.009f,.005f,.017f)};
 auto Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Akuroth/Materials/M_Combat.M_Combat"));if(!Parent)return false;
 auto StoneParent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/StoryEnvironments/M_StoryStone.M_StoryStone"));
 Atmosphere->SetFogInscatteringColor(Phase?FLinearColor(.08f,.002f,.012f):Definition.Sky);
 AmbientLight->SetLightColor(Phase?FLinearColor(.7f,.55f,.62f):Snow?FLinearColor(.65f,.8f,1):FLinearColor(.72f,.78f,.85f));AmbientLight->SetIntensity(Phase?1.5f:2.2f);
 for(int I=0;I<8;I++){auto M=UMaterialInstanceDynamic::Create(I<2&&StoneParent?StoneParent:Parent,this);M->SetVectorParameterValue(TEXT("Color"),Palette[I]);M->SetScalarParameterValue(TEXT("Glow"),I==4?1.8f:I==5?.15f:0.f);Materials.Add(M);}
 // A real sky bowl behind the architecture replaces the flat regional image in combat.
 for(int I=0;I<48;I++){
  const float A=I*2*PI/48,B=(I+1)*2*PI/48;FVector X(FMath::Cos(A)*6200,FMath::Sin(A)*6200,-650),Y(FMath::Cos(B)*6200,FMath::Sin(B)*6200,-650);
  Geo[5].Quad(X+FVector(0,0,4200),Y+FVector(0,0,4200),Y,X);
 }
 // Individually laid flagstones: subtle height variation keeps the combat footprint flat.
 for(int X=-8;X<8;X++)for(int Y=-5;Y<5;Y++){
  if((Forest||Snow||Theme==TEXT("ash"))&&FMath::Abs(Y)>1)continue;
  FVector P((X+.5)*150,(Y+.5)*150,-28-Rand.FRandRange(0,3));
  Geo[(X+Y)%4==0?1:0].Box(P,FVector(147,147,48),FRotator(0,Rand.FRandRange(-.4f,.4f),0));
 }
 // The foundation stays below even the lowest flagstone; overlapping coplanar tops flicker.
 Geo[Forest?2:Snow?1:0].Box(FVector(0,0,-89),FVector(2550,1650,150));
 for(int I=0;I<60;I++){float A=I*2*PI/60;Geo[1].Box(FVector(FMath::Cos(A)*1300,FMath::Sin(A)*850,-35),FVector(120,90,60),FRotator(0,FMath::RadiansToDegrees(A),0));}
 // Hills/cliffs are faceted original meshes, placed outside both teams' movement lanes.
 for(int I=0;I<24;I++){
  const bool Wild=Forest||Snow||Theme==TEXT("canyon")||Theme==TEXT("ash");
  float X=(I%12-5.5f)*510,Y=(Wild?-2100.f:-3000.f)-(I/12)*650;float H=Rand.FRandRange(260,Snow?1800:Wild?850:600);
  Geo[0].Rock(FVector(X,Y,-140),FVector(Rand.FRandRange(220,460),360,H),I*31+77);
  if(Snow)Geo[1].Rock(FVector(X,Y,H*.68f),FVector(220,230,H*.25f),I*23+3);
 }
 auto Column=[&](FVector P,float H){
  Geo[1].Box(P+FVector(0,0,18),FVector(145,145,36));Geo[0].Tube(P+FVector(0,0,36),P+FVector(0,0,H),42,36,12);Geo[1].Box(P+FVector(0,0,H),FVector(130,130,38));
  for(int J=0;J<8;J++){float A=J*PI/4;FVector O(FMath::Cos(A)*39,FMath::Sin(A)*39,0);Geo[1].Tube(P+O+FVector(0,0,70),P+O+FVector(0,0,H-20),3,3,5);}
 };
 auto Root=[&](FVector P,float Angle,float Length,float Height){
  FVector Prev=P;for(int J=1;J<=12;J++){float T=J/12.f;FVector Next=P+FVector(FMath::Cos(Angle)*Length*T,FMath::Sin(Angle)*Length*T,FMath::Sin(T*PI)*Height);Geo[3].Tube(Prev,Next,24*(1-(T-1.f/12)*.85f),24*(1-T*.85f),7);Prev=Next;}
 };
 auto Flower=[&](FVector P,float Size){Geo[3].Tube(P,P+FVector(0,0,Size*.6f),Size*.045,Size*.025,6);for(int J=0;J<9;J++)Geo[2].Leaf(P+FVector(0,0,Size*.6),J*2*PI/9,Size,Size*.24f,-Size*.15f);Geo[4].Rock(P+FVector(0,0,Size*.64),FVector(Size*.18,Size*.18,Size*.12),15);};
 if(Forest){
  for(int I=0;I<23;I++){
   FVector P((I%12-5.5f)*440,-1000-(I/12)*700,0);float H=Rand.FRandRange(600,1100);
   FVector Crown=P+FVector(Rand.FRandRange(-80,80),20,H);Geo[3].Tube(P,Crown,58,16,12);
   for(int J=0;J<5;J++){float A=J*2*PI/5+I;FVector End=Crown+FVector(FMath::Cos(A)*260,FMath::Sin(A)*200,J*45-150);Geo[3].Tube(P+FVector(0,0,H*.65f),End,22,3,7);Geo[2].Rock(End,FVector(220,200,140),I*41+J);}
   for(int J=0;J<4;J++)Root(P,J*PI/2+I,240,20);
  }
  for(int I=0;I<120;I++){FVector P(Rand.FRandRange(-2200,2200),Rand.FRandRange(-1200,-800),5);for(int J=0;J<4;J++)Geo[2].Leaf(P,J*PI*.5f,I%3==0?75:40,6,35);}
  if(Theme==TEXT("meadow")){
   Geo[1].Box(FVector(-1450,-950,170),FVector(500,360,340));Geo[3].Box(FVector(-1450,-950,400),FVector(610,460,65),FRotator(0,0,18));
   for(int I=0;I<12;I++){Geo[3].Box(FVector(650+I*95,-970,65),FVector(14,15,130));}Geo[3].Box(FVector(1150,-970,105),FVector(1250,12,14));
  }else for(int I=0;I<6;I++)Geo[4].Rock(FVector(1800-I*65,-1000,30+I*35),FVector(30,22,110),I+99);
 }
 if(Theme==TEXT("canyon")){
  for(int I=0;I<6;I++){Geo[0].Rock(FVector(I%2?1750:-1750,-500-(I/2)*650,-800),FVector(360,480,1600),I+121);}
  for(int I=0;I<22;I++)Geo[3].Box(FVector(-1200+I*115,-920,-15),FVector(105,280,30),FRotator(0,0,FMath::Sin(I*.3f)*4));
  for(int Side:{-1,1})for(int I=0;I<24;I++){FVector A(-1280+I*110,-920+Side*145,110-FMath::Sin(I*PI/24)*90),B=A+FVector(110,0,-FMath::Cos(I*PI/24)*10);Geo[3].Tube(A,B,6,6,6);}
  Column(FVector(1250,-900,0),550);Geo[4].Arch(FVector(1250,-880,500),75,4,4,0,2*PI,36);
 }
 if(Theme==TEXT("ash")){
  for(int I=0;I<5;I++){FVector P((I-2)*590,-1250,0);Geo[0].Box(P+FVector(0,0,120),FVector(430,75,240));Geo[0].Box(P+FVector(-200,120,100),FVector(60,310,200));
   for(int J=0;J<5;J++)Geo[3].Box(P+FVector(-190+J*85,0,350-J*38),FVector(20,25,460-J*55),FRotator(0,0,20+J*12));
   Geo[4].Rock(P+FVector(0,80,5),FVector(120,100,15),I+8);
  }
  Geo[3].Box(FVector(-1600,-850,25),FVector(140,80,50));Geo[3].Tube(FVector(-1650,-850,20),FVector(-1480,-850,20),6,6);
 }
 if(Snow){for(int I=0;I<20;I++){float X=(I-9.5f)*190;Geo[1].Rock(FVector(X,-1000,0),FVector(80,70,180+I%5*90),I+13);}for(int I=0;I<9;I++)Geo[1].Box(FVector(1500,-600-I*100,I*30),FVector(350,110,50));Geo[1].Arch(FVector(1250,-1100,250),220,45,80);}
 if(Water){
  Geo[6].Box(FVector(0,-250,-70),FVector(6200,5000,12));
  for(int I=0;I<9;I++){float X=(I-4)*410;Column(FVector(X,-1150,0),570+I%3*150);if(I<8)Geo[1].Arch(FVector(X+205,-1150,570),165,32,90);}
  for(int I=0;I<18;I++){FVector P((I%9-4)*430,-1250-(I/9)*400,0);Root(P,I*.7f,500,130);}
  if(Theme==TEXT("archive")){
   for(int I=0;I<5;I++){FVector P((I-2)*660,-1550,220);Geo[3].Box(P,FVector(420,85,420));for(int J=0;J<4;J++){Geo[1].Box(P+FVector(0,35,J*95-150),FVector(410,100,12));for(int K=0;K<12;K++)Geo[1].Box(P+FVector(K*30-165,45,J*95-110),FVector(16,55,65),FRotator(0,0,(K%3-1)*8));}}
  }else{
   for(int I=0;I<32;I++)Flower(FVector(Rand.FRandRange(-2200,2200),Rand.FRandRange(-1600,-900),10),Rand.FRandRange(60,140));
   Geo[3].Tube(FVector(800,-1300,0),FVector(800,-1300,420),100,60,12);for(int J=0;J<14;J++)Geo[2].Leaf(FVector(800,-1300,420),J*2*PI/14,Phase?650:450,100,Phase?240:80);
   Geo[4].Arch(FVector(800,-1320,400),Phase?320:190,8,12,0,2*PI,48);
  }
 }
 if(Theme==TEXT("eclipse")||Theme==TEXT("palace")){
  for(int I=0;I<8;I++){FVector P((I-3.5f)*480,-1200,0);Column(P,I%3==0?480:840);if(I<7)Geo[1].Arch(P+FVector(240,0,820),195,38,100,0,I==3?PI*.7f:PI);}
  Geo[7].Rock(FVector(0,-3700,1650),FVector(430,100,440),14);Geo[4].Arch(FVector(0,-3550,1650),440,9,10,0,Theme==TEXT("palace")?PI*1.55f:2*PI,80);
  if(Theme==TEXT("palace")){for(int I=0;I<7;I++)Geo[1].Box(FVector(0,-900-I*90,I*20-20),FVector(680,100,40));Geo[1].Box(FVector(0,-1500,180),FVector(330,200,360));for(int J=0;J<12;J++){float A=J*2*PI/12;Geo[4].Tube(FVector(0,-1560,650)+FVector(FMath::Cos(A)*210,0,FMath::Sin(A)*210),FVector(0,-1560,650)+FVector(FMath::Cos(A)*290,0,FMath::Sin(A)*290),5,0);}}
  else {for(int I=0;I<12;I++)Geo[4].Rock(FVector((I-5.5f)*290,-1350,300+I%4*80),FVector(40,35,170),I+199);}
 }
 if(Infernal){
  // Basalt buttresses, deep fissures and suspended chains link the three underworld chapters.
  Geo[7].Box(FVector(0,0,-500),FVector(8000,7000,80));
  for(int I=0;I<13;I++){FVector P((I-6)*410,-1280,0);Column(P,Theme==TEXT("tribunal")?700:950);for(int J=0;J<12;J++){FVector A=P+FVector(0,0,650+J*57);Geo[3].Arch(A,28,5,7,J%2?PI*.15f:0,2*PI,12);}Geo[4].Rock(P+FVector(0,70,5),FVector(110,70,10),I+71);}
  for(int I=0;I<22;I++){float A=I*2*PI/22;FVector P(FMath::Cos(A)*1470,FMath::Sin(A)*1050,-160);Root(P,A+PI,150,30);Geo[4].Tube(P,P+FVector(120,10,160),4,1,5);}
  if(Theme==TEXT("tribunal")){
   for(int I=0;I<6;I++)Geo[1].Box(FVector(0,-1300-I*130,I*65),FVector(2100,140,85));
   Geo[4].Arch(FVector(0,-1450,730),340,8,8,0,2*PI,64);Geo[3].Tube(FVector(0,-1450,250),FVector(0,-1450,1200),18,18,8);
   for(int I=0;I<6;I++){FVector P((I-2.5f)*400,-1120,200);for(int J=0;J<6;J++)Geo[3].Tube(P+FVector(J*22-55,0,0),P+FVector(J*22-55,0,280),4,4,6);}
  }else if(Theme==TEXT("abyss")){
   for(int I=0;I<7;I++)Geo[1].Box(FVector(0,-1150-I*90,I*30),FVector(1100,100,50));
   Geo[0].Box(FVector(0,-1700,300),FVector(560,140,600));Geo[1].Box(FVector(0,-1550,130),FVector(580,410,80));
   for(int I=0;I<9;I++)Geo[1].Tube(FVector((I-4)*75,-1700,480),FVector((I-4)*95,-1700,850-FMath::Abs(I-4)*65),23,0,8);
   Geo[4].Arch(FVector(0,-1750,550),460,11,12,0,2*PI,72);
  }else{
   Geo[7].Rock(FVector(0,-3000,1150),FVector(590,140,590),74);Geo[4].Arch(FVector(0,-2850,1150),600,12,15,0,2*PI,90);
   for(int I=0;I<32;I++){float A=I*2*PI/32;FVector P(FMath::Cos(A)*1900,-1600,FMath::Sin(A)*650+650);Geo[1].Rock(P,FVector(100,90,90+I%3*80),I+177);}
   for(int I=0;I<5;I++)Geo[4].Tube(FVector(-1300+I*640,-2300,-100),FVector(-1120+I*600,-2300,1900),4,1,5);
  }
 }
 if(Phase&&Infernal){
  for(int I=0;I<18;I++){float A=I*2*PI/18;FVector P(FMath::Cos(A)*1700,-1300+FMath::Sin(A)*350,120);Geo[7].Tube(P,P+FVector(FMath::Cos(A)*90,0,380+I%4*80),55,0,7);Geo[4].Tube(P+FVector(0,0,25),P+FVector(0,0,300),3,1,5);}
 }
 // Small foreground ruins frame the shot, while the central battle lanes stay unobstructed.
 for(int Side:{-1,1}){if(!Forest)Column(FVector(Side*1650,200,-30),320);else Geo[3].Tube(FVector(Side*1600,180,-10),FVector(Side*1630,200,100),45,40,9);for(int I=0;I<6;I++)Geo[0].Rock(FVector(Side*(1400+I*85),200-I*170,-30),FVector(80,80,70+I*15),I+333);}
 TriangleCount=0;
 for(int I=0;I<8;I++){auto Target=I==5?Horizon.Get():Architecture.Get();const int Section=I==5?0:I;Target->CreateMeshSection_LinearColor(Section,Geo[I].V,Geo[I].T,Geo[I].N,Geo[I].UV,Geo[I].C,Geo[I].Tangents,false);Target->SetMaterial(Section,Materials[I]);TriangleCount+=Geo[I].T.Num()/3;}
 Motes->SetMaterial(0,Materials[4]);
 for(int I=0;I<96;I++){FVector Seed(Rand.FRandRange(-2400,2400),Rand.FRandRange(-1500,800),Rand.FRandRange(30,1100));MoteSeeds.Add(Seed);Motes->AddInstance(FTransform(FRotator::ZeroRotator,Seed,FVector(.025f,.025f,.025f)));}
 for(int I=0;I<4;I++){auto Lamp=NewObject<UPointLightComponent>(this);AddInstanceComponent(Lamp);Lamp->SetupAttachment(RootComponent);Lamp->SetRelativeLocation(FVector((I-1.5f)*800,-850,200));Lamp->SetLightColor(Accent);Lamp->SetIntensity(16000);Lamp->SetAttenuationRadius(900);Lamp->SetCastShadows(false);Lamp->RegisterComponent();Lamps.Add(Lamp);}
 UE_LOG(LogTemp,Display,TEXT("STORY_ENVIRONMENT id=%s phase=%d triangles=%d"),*CurrentId,Phase?2:1,TriangleCount);return TriangleCount>0;
}
void AEnigmaStoryEnvironment::Tick(float Delta){
 Super::Tick(Delta);if(IsHidden()||TriangleCount==0)return;
 const bool Snow=Definition.Theme==TEXT("snow"),Petals=Definition.Theme==TEXT("garden")||Definition.Theme==TEXT("archive"),Fire=Definition.Theme==TEXT("ash")||Definition.Theme==TEXT("abyss")||Definition.Theme==TEXT("tribunal")||Definition.Theme==TEXT("void");
 AmbientTime+=ReducedMotion?0:Delta;Pulse=FMath::Max(0.f,Pulse-Delta*1.8f);
 for(int I=0;I<MoteSeeds.Num();I++){FVector P=MoteSeeds[I];float T=AmbientTime;
  P.X+=FMath::Sin(T*.7f+I)*35;P.Y+=FMath::Cos(T*.4f+I)*25;
  P.Z=Snow?1100-FMath::Fmod(P.Z+T*48,1100.f):FMath::Fmod(P.Z+T*(Fire?65:18),1100.f);
  Motes->UpdateInstanceTransform(I,FTransform(FRotator(T*35+I*37,T*24,0),P,Petals?FVector(.06f,.13f,.015f):Snow?FVector(.035f):FVector(.018f,.018f,Fire?.09f:.018f)),false,false,true);
 }
 Motes->MarkRenderStateDirty();
 for(int I=0;I<Lamps.Num();I++)Lamps[I]->SetIntensity(16000*(1+(Fire&&!ReducedMotion?.12f*FMath::Sin(AmbientTime*5+I):0)+Pulse*.35f));
 if(Materials.IsValidIndex(4))Materials[4]->SetScalarParameterValue(TEXT("Glow"),1.8f+Pulse*.5f);
}
