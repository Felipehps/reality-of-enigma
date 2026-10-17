#include "EnigmaStoryEnvironment.h"
#include "EnigmaEnvironmentGeometry.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/PostProcessComponent.h"
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
 ForestSet=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BlenderForestSet"));ForestSet->SetupAttachment(RootComponent);ForestSet->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 ForestTrees=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BlenderCanopy"));ForestTrees->SetupAttachment(RootComponent);ForestTrees->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 ForestFerns=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("BlenderUnderstory"));ForestFerns->SetupAttachment(RootComponent);ForestFerns->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Horizon=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("DistantHorizon"));Horizon->SetupAttachment(RootComponent);Horizon->SetCastShadow(false);Horizon->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 AmbientLight=CreateDefaultSubobject<USkyLightComponent>(TEXT("StoryAmbientLight"));AmbientLight->SetupAttachment(RootComponent);AmbientLight->SourceType=SLS_SpecifiedCubemap;AmbientLight->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap")));AmbientLight->SetIntensity(.9f);AmbientLight->SetCastShadows(false);
 Motes=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("AmbientMotes"));Motes->SetupAttachment(RootComponent);Motes->SetCollisionEnabled(ECollisionEnabled::NoCollision);Motes->SetCastShadow(false);Motes->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
 Lens=CreateDefaultSubobject<UPostProcessComponent>(TEXT("ArtDirectionLens"));Lens->SetupAttachment(RootComponent);Lens->bUnbound=true;
 Lens->Settings.bOverride_BloomIntensity=true;Lens->Settings.BloomIntensity=.32f;Lens->Settings.bOverride_BloomThreshold=true;Lens->Settings.BloomThreshold=1.5f;
 Lens->Settings.bOverride_VignetteIntensity=true;Lens->Settings.VignetteIntensity=.18f;
 Lens->Settings.bOverride_ColorSaturation=true;Lens->Settings.ColorSaturation=FVector4(.96,.96,.96,1);
 Lens->Settings.bOverride_AmbientOcclusionIntensity=true;Lens->Settings.AmbientOcclusionIntensity=.65f;
 Atmosphere=CreateDefaultSubobject<UExponentialHeightFogComponent>(TEXT("StoryAtmosphere"));Atmosphere->SetupAttachment(RootComponent);Atmosphere->SetRelativeLocation(FVector(0,0,-150));Atmosphere->SetFogDensity(.014f);Atmosphere->SetFogHeightFalloff(.3f);Atmosphere->SetStartDistance(1300);Atmosphere->SetFogMaxOpacity(.65f);
}
FLinearColor AEnigmaStoryEnvironment::KeyColor() const{return Phase?FLinearColor(.78f,.23f,.18f):Definition.Theme==TEXT("forest")?FLinearColor(1,.83f,.58f):Definition.Theme==TEXT("snow")?FLinearColor(.65f,.8f,1):FLinearColor(1,.85f,.66f);}
FLinearColor AEnigmaStoryEnvironment::RimColor() const{if(CurrentId==TEXT("forest"))return FLinearColor(.18f,.35f,.45f);return Phase?FLinearColor(.65f,.08f,.3f):Definition.Accent;}
bool AEnigmaStoryEnvironment::Configure(const FString& Id,bool PhaseTwo){
 if(CurrentId==Id&&Phase==PhaseTwo&&TriangleCount>0)return true;
 const FEnigmaEnvironment* Found=Definitions().FindByPredicate([&](const FEnigmaEnvironment& D){return D.Id==Id;});if(!Found)return false;
 BlenderForestLoaded=false;ForestTreeCount=0;ForestFernCount=0;ForestSet->SetVisibility(false);ForestTrees->SetVisibility(false);ForestFerns->SetVisibility(false);ForestTrees->ClearInstances();ForestFerns->ClearInstances();Atmosphere->SetVolumetricFog(false);Lens->Settings.bOverride_ColorContrast=false;Lens->Settings.ColorSaturation=FVector4(.96f,.96f,.96f,1);
 Definition=*Found;CurrentId=Id;Phase=PhaseTwo&&Found->HasPhase;AmbientTime=0;Pulse=0;
 Architecture->ClearAllMeshSections();Horizon->ClearAllMeshSections();for(auto Lamp:Lamps)Lamp->DestroyComponent();Lamps.Reset();Materials.Reset();Motes->ClearInstances();MoteSeeds.Reset();
 using namespace Scenery;Mesh Geo[17];FRandomStream Rand(1741+GetTypeHash(Id)%10000);
 const FString Theme=Definition.Theme;bool Forest=Theme==TEXT("forest")||Theme==TEXT("meadow"),Snow=Theme==TEXT("snow"),Water=Theme==TEXT("archive")||Theme==TEXT("garden"),Infernal=Theme==TEXT("tribunal")||Theme==TEXT("abyss")||Theme==TEXT("void");
 FLinearColor Stone=Definition.Stone*(Phase?.85f:1.f),Accent=Phase?FLinearColor(1,.018f,.055f):Definition.Accent,Plant=Phase?FLinearColor(.30f,.008f,.04f):Definition.Vegetation;
 FLinearColor Palette[]={Stone,Stone*1.45f,Plant*.65f,FLinearColor(.19f,.105f,.045f),Accent,Phase?FLinearColor(.025f,.003f,.01f):Definition.Sky,FLinearColor(.018f,.09f,.075f),FLinearColor(.002f,.002f,.004f),Forest?FLinearColor(.055f,.085f,.035f):FLinearColor(.09f,.075f,.055f),Plant*.65f,Phase?FLinearColor(.42f,.009f,.025f):FLinearColor(.25f,.012f,.04f),Infernal?FLinearColor(.15f,.09f,.055f):FLinearColor(.52f,.32f,.10f),Accent,FLinearColor(.65f,.75f,.85f),FLinearColor(.4f,.5f,.65f),Phase?FLinearColor(.8f,.025f,.015f):FLinearColor(.95f,.15f,.025f),FLinearColor(.65f,.53f,.34f)};
 auto Parent=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Akuroth/Materials/M_Combat.M_Combat"));if(!Parent)return false;
 const TCHAR* Names[]={TEXT("Stone"),TEXT("Stone"),TEXT("Foliage"),TEXT("Wood"),TEXT(""),TEXT("Sky"),TEXT("Water"),TEXT(""),TEXT("Ground"),TEXT("Foliage"),TEXT("Petal"),TEXT("Metal"),TEXT("Lava"),TEXT("Ground"),TEXT("Mist"),TEXT("Fire"),TEXT("Paper")};
 ArtMaterialCount=0;TSet<FString> ReadyMaterials;
 Atmosphere->SetFogInscatteringColor(Phase?FLinearColor(.055f,.008f,.02f):Definition.Sky*.75f);Atmosphere->SetFogDensity(Forest?.022f:Water?.019f:.012f);Atmosphere->SetStartDistance(900);Atmosphere->SetFogMaxOpacity(.78f);
 AmbientLight->SetLightColor(Phase?FLinearColor(.62f,.5f,.6f):Snow?FLinearColor(.72f,.85f,1):FLinearColor(.78f,.82f,.9f));AmbientLight->SetIntensity(Phase?1.15f:Forest?1.05f:1.25f);
 for(int I=0;I<17;I++){
  UMaterialInterface* Surface=Parent;if(Names[I][0]){FString Path=TEXT("/Game/StoryEnvironments/M_Story")+FString(Names[I]);if(auto Loaded=LoadObject<UMaterialInterface>(nullptr,*(Path+TEXT(".M_Story")+Names[I]))){Surface=Loaded;ReadyMaterials.Add(Names[I]);}}
  auto M=UMaterialInstanceDynamic::Create(Surface,this);M->SetVectorParameterValue(TEXT("Color"),Palette[I]);M->SetScalarParameterValue(TEXT("Glow"),I==4?1.7f:0.f);M->SetScalarParameterValue(TEXT("SceneTime"),0);Materials.Add(M);
 }
 ArtMaterialCount=ReadyMaterials.Num()-int32(ReadyMaterials.Contains(TEXT("Stone")));
 // A real sky bowl behind the architecture replaces the flat regional image in combat.
 for(int I=0;I<48;I++){
  const float A=I*2*PI/48,B=(I+1)*2*PI/48;FVector X(FMath::Cos(A)*6200,FMath::Sin(A)*6200,-650),Y(FMath::Cos(B)*6200,FMath::Sin(B)*6200,-650);
  Geo[5].Quad(X+FVector(0,0,4200),Y+FVector(0,0,4200),Y,X);
 }
 // Continuous terrain gives the architecture a physical setting rather than an isolated rectangle.
 if(!Infernal&&Theme!=TEXT("eclipse")){Geo[Snow?13:8].Terrain(1741,8800,7200,Water);TerrainVertices=Geo[Snow?13:8].V.Num();}else TerrainVertices=0;
 if(!Forest&&!Snow&&Theme!=TEXT("ash")){
  Geo[0].Tube(FVector(0,0,-95),FVector(0,0,-1),1280,1260,96);
  Geo[1].Ring(FVector(0,0,1),1200,42,15);Geo[1].Ring(FVector(0,0,2),990,10,4);
  for(int I=0;I<36;I++){float A=I*2*PI/36;Geo[1].Box(FVector(FMath::Cos(A)*1120,FMath::Sin(A)*1120,-5),FVector(84,18,12),FRotator(0,FMath::RadiansToDegrees(A),0));}
  if(Theme==TEXT("palace")||Infernal){Geo[11].Ring(FVector(0,0,0),720,4,2);Geo[11].Ring(FVector(0,0,0),780,4,2);for(int I=0;I<12;I++){float A=I*PI/6;Geo[11].Tube(FVector(FMath::Cos(A)*780,FMath::Sin(A)*780,-2),FVector(FMath::Cos(A)*900,FMath::Sin(A)*900,-2),2,2,5);}}
 }else{
  for(int I=0;I<52;I++){float X=(I%26-12.5f)*100,Y=(I/26-.5f)*140+FMath::Sin(I*.4f)*40;Geo[0].Rock(FVector(X,Y,-17),FVector(48,68,20),I+188);}
 }
 // Hills/cliffs are faceted original meshes, placed outside both teams' movement lanes.
 for(int I=0;I<24;I++){
  const bool Wild=Forest||Snow||Theme==TEXT("canyon")||Theme==TEXT("ash");
  float X=(I%12-5.5f)*510,Y=(Wild?-2100.f:-3000.f)-(I/12)*650;float H=Rand.FRandRange(260,Snow?1800:Forest?500:Wild?850:600);
  Geo[0].Rock(FVector(X,Y,-140),FVector(Rand.FRandRange(220,460),360,H),I*31+77);
  if(Snow)Geo[1].Rock(FVector(X,Y,H*.68f),FVector(220,230,H*.25f),I*23+3);
 }
 auto Column=[&](FVector P,float H){
  Geo[0].Box(P+FVector(0,0,-65),FVector(160,160,150));Geo[1].Box(P+FVector(0,0,12),FVector(150,150,24));
  Geo[0].Lathe(P,{{64,20},{64,30},{56,36},{49,44},{43,66},{36,H*.16f},{32,H*.76f},{35,H*.82f},{44,H*.86f},{55,H*.9f},{65,H*.94f},{65,H}},32);
  for(float Z:{.14f,.78f,.86f})Geo[11].Ring(P+FVector(0,0,H*Z),38,4,4,32);
  for(int J=0;J<10;J++){float A=J*PI/5;FVector O(FMath::Cos(A)*34,FMath::Sin(A)*34,0);Geo[1].Tube(P+O+FVector(0,0,H*.18),P+O+FVector(0,0,H*.74),2,2,6);}
  for(int J=0;J<8;J++)Geo[1].Leaf(P+FVector(0,0,H*.84f),J*PI/4,60,16,65);
 };
 auto Root=[&](FVector P,float Angle,float Length,float Height){
  TArray<FVector> Curve;for(int J=0;J<=12;J++){float T=J/12.f;Curve.Add(P+FVector(FMath::Cos(Angle)*Length*T,FMath::Sin(Angle)*Length*T,FMath::Sin(T*PI)*Height));}Geo[3].Vine(Curve,24,3.6f,10);
 };
 auto Flower=[&](FVector P,float Size){Geo[3].Tube(P,P+FVector(0,0,Size*.6f),Size*.035,Size*.02,8);for(int J=0;J<9;J++)Geo[10].Leaf(P+FVector(0,0,Size*.6),J*2*PI/9,Size,Size*.26f,-Size*.22f);Geo[11].Ellipsoid(P+FVector(0,0,Size*.65),FVector(Size*.15,Size*.15,Size*.11),12,6);};
 if(Forest){
  for(int I=0;I<20;I++){
   FVector P((I%7-3)*820.f+Rand.FRandRange(-180,180),-1150-(I/7)*790+Rand.FRandRange(-160,160),-25);float H=Rand.FRandRange(950,1500);
   TArray<FVector> Trunk;for(int J=0;J<=12;J++){float T=J/12.f;Trunk.Add(P+FVector(FMath::Sin(T*2+I)*80,FMath::Sin(T*3+I)*65,H*T));}Geo[3].Vine(Trunk,68,13.6f,18);
   for(int J=0;J<4;J++)Root(P,J*PI/2+I,270,35);
   for(int Branch=0;Branch<4;Branch++){
    float A=Branch*PI*.5f+I*.7f;FVector Base=P+FVector(0,0,H*(.55f+Branch*.07f)),Elbow=Base+FVector(FMath::Cos(A)*160,FMath::Sin(A)*160,120),End=Elbow+FVector(FMath::Cos(A)*210,FMath::Sin(A)*210,95);
    Geo[3].Vine({Base,Elbow,End},26,3,14);
    for(int Leaf=0;Leaf<65;Leaf++){FVector Q=End+FVector(Rand.FRandRange(-195,195),Rand.FRandRange(-195,195),Rand.FRandRange(-80,160));Geo[2].Leaf(Q,Rand.FRandRange(0,2*PI),Rand.FRandRange(75,130),Rand.FRandRange(18,32),Rand.FRandRange(-35,40));}
   }
  }
  for(int I=0;I<250;I++){
   FVector P(Rand.FRandRange(-2800,2800),Rand.FRandRange(-2100,1000),-17);if(FMath::Abs(P.X)<1050&&FMath::Abs(P.Y)<420)continue;
   for(int J=0;J<5;J++)Geo[I%4==0?9:2].Leaf(P,J*PI*.4f+I,Rand.FRandRange(45,95),Rand.FRandRange(4,11),Rand.FRandRange(25,65));
  }
  for(int I=0;I<3;I++)Geo[14].Ribbon(FVector(-700+I*800,850,1700),FVector(-1200+I*800,-1300,-20),100+I*40);
  if(Theme==TEXT("meadow")){
   FVector P(-1750,-1400,0);Geo[16].Box(P+FVector(0,0,150),FVector(550,420,300));
   Geo[3].Quad(P+FVector(-330,-260,300),P+FVector(330,-260,300),P+FVector(330,0,490),P+FVector(-330,0,490));
   Geo[3].Quad(P+FVector(-330,0,490),P+FVector(330,0,490),P+FVector(330,260,300),P+FVector(-330,260,300));
   Geo[3].Box(P+FVector(0,215,100),FVector(95,20,200));for(int J=0;J<2;J++)Geo[7].Box(P+FVector(J?170:-170,216,180),FVector(70,10,70));
   for(int I=0;I<17;I++)Geo[3].Box(FVector(450+I*90,-1050,55),FVector(13,18,140),FRotator(0,0,(I%3-1)*3));for(int J=0;J<2;J++)Geo[3].Box(FVector(1170,-1050,35+J*60),FVector(1500,14,13));
  }else for(int I=0;I<7;I++)Geo[4].Rock(FVector(1800-I*55,-1250,10+I*12),FVector(20,17,80+I*12),I+99);
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
  Geo[6].Quad(FVector(-4100,-3800,-36),FVector(4100,-3800,-36),FVector(4100,2400,-36),FVector(-4100,2400,-36));
  for(int I=0;I<9;I++){float X=(I-4)*410;Column(FVector(X,-1150,0),570+I%3*150);if(I<8)Geo[1].Arch(FVector(X+205,-1150,570),165,32,90);}
  for(int I=0;I<18;I++){FVector P((I%9-4)*430,-1250-(I/9)*400,0);Root(P,I*.7f,500,130);}
  if(Theme==TEXT("archive")){
   for(int I=0;I<5;I++){FVector P((I-2)*660,-1550,220);Geo[3].Box(P,FVector(420,85,420));for(int J=0;J<4;J++){Geo[1].Box(P+FVector(0,35,J*95-150),FVector(410,100,12));for(int K=0;K<12;K++){FVector Book=P+FVector(K*30-165,45,J*95-110);Geo[3].Box(Book,FVector(18,55,65),FRotator(0,0,(K%3-1)*8));Geo[16].Box(Book+FVector(0,28,0),FVector(12,3,57));}}}
  }else{
   for(int I=0;I<32;I++)Flower(FVector(Rand.FRandRange(-2200,2200),Rand.FRandRange(-1600,-900),10),Rand.FRandRange(60,140));
   Geo[3].Tube(FVector(800,-1300,-20),FVector(800,-1300,360),85,45,18);
   for(int Layer=0;Layer<3;Layer++)for(int J=0;J<16;J++)Geo[10].Leaf(FVector(800,-1300,340+Layer*40),J*PI/8+Layer*.18f,(Phase?460:340)+Layer*30,85+Layer*10,Layer==0?-120:Layer==1?30:190);
   Geo[7].Ellipsoid(FVector(800,-1300,410),FVector(65,65,50),28,14);
   Geo[4].Arch(FVector(800,-1320,400),Phase?320:190,8,12,0,2*PI,48);
  }
 }
 if(Theme==TEXT("eclipse")||Theme==TEXT("palace")){
  for(int I=0;I<8;I++){FVector P((I-3.5f)*480,-1200,0);Column(P,I%3==0?480:840);if(I<7)Geo[1].Arch(P+FVector(240,0,820),195,38,100,0,I==3?PI*.7f:PI);}
  Geo[7].Ellipsoid(FVector(0,-3600,580),FVector(260,70,260));Geo[4].Arch(FVector(0,-3500,580),270,4,6,0,Theme==TEXT("palace")?PI*1.55f:2*PI,96);
  if(Theme==TEXT("palace")){for(int I=0;I<7;I++)Geo[1].Box(FVector(0,-900-I*90,I*20-20),FVector(680,100,40));Geo[1].Box(FVector(0,-1500,180),FVector(330,200,360));for(int J=0;J<12;J++){float A=J*2*PI/12;Geo[4].Tube(FVector(0,-1560,650)+FVector(FMath::Cos(A)*210,0,FMath::Sin(A)*210),FVector(0,-1560,650)+FVector(FMath::Cos(A)*290,0,FMath::Sin(A)*290),5,0);}}
  else {for(int I=0;I<12;I++)Geo[4].Rock(FVector((I-5.5f)*290,-1350,300+I%4*80),FVector(40,35,170),I+199);}
 }
 if(Infernal){
  // Basalt buttresses, deep fissures and suspended chains link the three underworld chapters.
  Geo[12].Box(FVector(0,-300,-190),FVector(9000,7500,30));Geo[0].Box(FVector(0,-1400,-130),FVector(6000,900,260));
  for(int I=0;I<13;I++){FVector P((I-6)*410,-1280,0);Column(P,Theme==TEXT("tribunal")?700:950);for(int J=0;J<12;J++){FVector A=P+FVector(0,0,650+J*57);Geo[3].Arch(A,28,5,7,J%2?PI*.15f:0,2*PI,12);}Geo[11].Lathe(P+FVector(0,60,0),{{28,0},{22,25},{15,150},{32,165},{38,175}},16);Geo[15].Ribbon(P+FVector(0,60,155),P+FVector(0,60,300),38);}
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
   Geo[7].Ellipsoid(FVector(0,-3600,580),FVector(300,90,300));Geo[4].Arch(FVector(0,-3450,580),310,5,10,0,2*PI,120);
   for(int I=0;I<32;I++){float A=I*2*PI/32;FVector P(FMath::Cos(A)*1900,-1600,FMath::Sin(A)*650+650);Geo[1].Rock(P,FVector(100,90,90+I%3*80),I+177);}
   for(int I=0;I<5;I++)for(int J=0;J<9;J++){FVector P(-1300+I*640+FMath::Sin(J*2.f+I)*38,-2900,J*120-20);Geo[4].Tube(P,P+FVector(FMath::Cos(J*3.f+I)*45,0,125),2,1,6);}
  }
 }
 if(Phase&&Infernal){
  for(int I=0;I<18;I++){float A=I*2*PI/18;FVector P(FMath::Cos(A)*1700,-1300+FMath::Sin(A)*350,120);Geo[7].Tube(P,P+FVector(FMath::Cos(A)*90,0,380+I%4*80),55,0,7);Geo[4].Tube(P+FVector(0,0,25),P+FVector(0,0,300),3,1,5);}
 }
 // Small foreground ruins frame the shot, while the central battle lanes stay unobstructed.
 for(int Side:{-1,1}){if(!Forest)Column(FVector(Side*1650,200,-30),320);else Geo[3].Tube(FVector(Side*1600,180,-10),FVector(Side*1630,200,100),45,40,9);for(int I=0;I<6;I++)Geo[0].Rock(FVector(Side*(1400+I*85),200-I*170,-30),FVector(80,80,70+I*15),I+333);}
 TriangleCount=0;SurfaceFacingErrors=0;BlenderForestLoaded=Id==TEXT("forest")&&ConfigureBlenderForest();
 for(int I=0;I<17;I++){if(BlenderForestLoaded&&I!=5)continue;Geo[I].FinishForUnreal();SurfaceFacingErrors+=Geo[I].FacingErrors();auto Target=I==5?Horizon.Get():Architecture.Get();const int Section=I==5?0:I;Target->CreateMeshSection_LinearColor(Section,Geo[I].V,Geo[I].T,Geo[I].N,Geo[I].UV,Geo[I].C,Geo[I].Tangents,false);Target->SetMaterial(Section,Materials[I]);TriangleCount+=Geo[I].T.Num()/3;}
 Motes->SetMaterial(0,Materials[4]);
 for(int I=0;I<96;I++){FVector Seed(Rand.FRandRange(-2400,2400),Rand.FRandRange(-1500,800),Rand.FRandRange(30,1100));MoteSeeds.Add(Seed);Motes->AddInstance(FTransform(FRotator::ZeroRotator,Seed,FVector(.025f,.025f,.025f)));}
 if(!BlenderForestLoaded)for(int I=0;I<4;I++){auto Lamp=NewObject<UPointLightComponent>(this);AddInstanceComponent(Lamp);Lamp->SetupAttachment(RootComponent);Lamp->SetRelativeLocation(FVector((I-1.5f)*800,-850,200));Lamp->SetLightColor(Accent);Lamp->SetIntensity(16000);Lamp->SetAttenuationRadius(900);Lamp->SetCastShadows(false);Lamp->RegisterComponent();Lamps.Add(Lamp);}
 UE_LOG(LogTemp,Display,TEXT("STORY_ENVIRONMENT id=%s phase=%d triangles=%d art_materials=%d facing_errors=%d"),*CurrentId,Phase?2:1,TriangleCount,ArtMaterialCount,SurfaceFacingErrors);return TriangleCount>0;
}
void AEnigmaStoryEnvironment::SetPresentationVisible(bool Visible){SetActorHiddenInGame(!Visible);if(Lens)Lens->bEnabled=Visible;}
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
 for(int Index:{2,5,6,9,10,12,15})if(Materials.IsValidIndex(Index))Materials[Index]->SetScalarParameterValue(TEXT("SceneTime"),AmbientTime);
 for(int Index=17;Index<Materials.Num();Index++)Materials[Index]->SetScalarParameterValue(TEXT("SceneTime"),AmbientTime);
 for(int I=0;I<Lamps.Num();I++)Lamps[I]->SetIntensity(16000*(1+(Fire&&!ReducedMotion?.12f*FMath::Sin(AmbientTime*5+I):0)+Pulse*.35f));
 if(Materials.IsValidIndex(4))Materials[4]->SetScalarParameterValue(TEXT("Glow"),1.8f+Pulse*.5f);
}
