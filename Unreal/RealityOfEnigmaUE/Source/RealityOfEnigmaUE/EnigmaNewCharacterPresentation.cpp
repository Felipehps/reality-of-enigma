#include "EnigmaWorldPresentation.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
namespace NC {
 FString S(TSharedPtr<FJsonObject> P,const TCHAR* K){FString V;if(P)P->TryGetStringField(K,V);return V;}
 double N(TSharedPtr<FJsonObject> P,const TCHAR* K){double V=0;if(P)P->TryGetNumberField(K,V);return V;}
 bool B(TSharedPtr<FJsonObject> P,const TCHAR* K){bool V=false;if(P)P->TryGetBoolField(K,V);return V;}
 int32 Stacks(TSharedPtr<FJsonObject> P,const TCHAR* Id){int32 Count=0;const TArray<TSharedPtr<FJsonValue>>* Values=nullptr;if(P&&P->TryGetArrayField(TEXT("statuses"),Values))for(auto V:*Values)Count+=S(V->AsObject(),TEXT("id"))==Id;return Count;}
}
using namespace NC;
void AEnigmaWorldPresentation::FaceRing(UInstancedStaticMeshComponent* Batch,FVector P,float Radius){
 for(int32 I=0;I<40;I++){float A=I*2*PI/40,B=(I+1)*2*PI/40;Beam(Batch,P+FVector(FMath::Cos(A)*Radius,0,FMath::Sin(A)*Radius),P+FVector(FMath::Cos(B)*Radius,0,FMath::Sin(B)*Radius),2.5f);}
}
void AEnigmaWorldPresentation::UpdateGarment(int32 I,const FString& Id,FVector P){
 auto Component=Garments.Find(Keys[I]);UProceduralMeshComponent* Cloth=Component?Component->Get():nullptr;
 if(!Cloth){Cloth=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Cloth);Cloth->SetupAttachment(RootComponent);Cloth->SetCollisionEnabled(ECollisionEnabled::NoCollision);Cloth->bUseAsyncCooking=true;Cloth->RegisterComponent();auto Mat=UMaterialInstanceDynamic::Create(Material,this);Mat->SetVectorParameterValue(TEXT("Color"),Id==TEXT("tenrin")||Id==TEXT("zevalon")?FLinearColor(.72f,.6f,.3f):Id==TEXT("zhujin")?FLinearColor(.12f,.11f,.035f):FLinearColor(.09f,.012f,.02f));Mat->SetScalarParameterValue(TEXT("Glow"),.01f);Cloth->SetMaterial(0,Mat);Garments.Add(Keys[I],Cloth);}
 const bool Enemy=I>=Snapshot->GetArrayField(TEXT("allies")).Num();float Back=Enemy?1:-1,Scale=Bodies[I]->GetComponentScale().X;FVector Root=Bodies[I]->GetBoneLocation(TEXT("spine_03"));TArray<FVector> Vertices,Normals;TArray<FVector2D> UV;TArray<int32> Triangles;TArray<FLinearColor> VertexColors;TArray<FProcMeshTangent> Tangents;
 for(int32 Row=0;Row<=8;Row++)for(int32 Col=0;Col<=6;Col++){float T=Row/8.f,U=Col/6.f,Wind=ReducedMotion?0:FMath::Sin(Clock*1.6f+T*4+U*3);Vertices.Add(Root+FVector(Back*(15+T*65+Wind*T*14)*Scale,(U-.5f)*(80+T*65)*Scale,-T*115*Scale+Wind*T*8));Normals.Add(FVector(Back,0,0));UV.Add(FVector2D(U,T));VertexColors.Add(FLinearColor::White);Tangents.Add(FProcMeshTangent(0,1,0));}
 for(int32 Row=0;Row<8;Row++)for(int32 Col=0;Col<6;Col++){int32 A=Row*7+Col,B=A+1,C=A+7,D=C+1;Triangles.Append({A,C,B,B,C,D,B,C,A,D,C,B});}
 if(Cloth->GetNumSections()==0)Cloth->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UV,VertexColors,Tangents,false);else Cloth->UpdateMeshSection_LinearColor(0,Vertices,Normals,UV,VertexColors,Tangents);
}
void AEnigmaWorldPresentation::DrawNewCharacter(int32 I,TSharedPtr<FJsonObject> U,FVector P){
 const FString Id=S(U,TEXT("id"));if(Stacks(U,TEXT("divine_guard"))||Stacks(U,TEXT("celestial_guard"))){FaceRing(Gold,P+FVector(0,0,105),100);Ring(Lightning,P+FVector(0,0,15),80);}
 int32 Divine=Stacks(U,TEXT("divine_mark")),Celestial=Stacks(U,TEXT("celestial_mark"));if(Divine||Celestial){auto Batch=Celestial?Lightning:Gold;for(int32 M=0;M<Divine+Celestial;M++){float A=Clock+M*2*PI/5;Beam(Batch,P+FVector(FMath::Cos(A)*50,FMath::Sin(A)*50,230),P+FVector(FMath::Cos(A)*50,FMath::Sin(A)*50,248),3);}}
 if(Id!=TEXT("mazakin")&&Id!=TEXT("hajun")&&Id!=TEXT("zhujin")&&Id!=TEXT("tenrin")&&Id!=TEXT("zevalon"))return;
 UpdateGarment(I,Id,P);FVector Head=Bodies[I]->GetBoneLocation(TEXT("head")),Hand=Bodies[I]->GetBoneLocation(TEXT("hand_r"));bool DivineHero=Id==TEXT("tenrin")||Id==TEXT("zevalon"),White=DivineHero||Id==TEXT("hajun")&&B(U,TEXT("ascended"));auto Hair=White?WhiteHair:ShadowLimbs;
 for(int32 Strand=0;Strand<14;Strand++)for(int32 J=0;J<5;J++){float Sway=ReducedMotion?0:FMath::Sin(Clock*1.8f+Strand*.7f+J*.5f)*8;FVector A=Head+FVector(J*12,(Strand-7)*3+Sway,8-J*22),B=Head+FVector((J+1)*12,(Strand-7)*3+Sway,8-(J+1)*22);Beam(Hair,A,B,3);}
 if(DivineHero){
  FaceRing(Gold,Head+FVector(0,-22,10),65);FVector Top=Hand+FVector(0,0,175);Beam(Gold,Hand-FVector(0,0,90),Top,5);FaceRing(Gold,Top,32);for(int32 J=0;J<10;J++){float A=J*2*PI/10;Beam(Gold,Top+FVector(FMath::Cos(A)*25,0,FMath::Sin(A)*25),Top+FVector(FMath::Cos(A)*47,0,FMath::Sin(A)*47),4);}
  if(Id==TEXT("zevalon"))for(int32 J=0;J<6;J++)Beam(Lightning,Hand+FVector(J*7,0,J*10),Hand+FVector((J+1)*7,8,(J+1)*10),2);
 }
 if(Id==TEXT("hajun")){
  Beam(ShadowLimbs,Hand-FVector(0,0,60),Hand+FVector(0,0,175),8);
  if(B(U,TEXT("ascended"))){for(int32 J=0;J<7;J++){Beam(WhiteHair,Hand+FVector(-J*9,0,50+J*24),Hand+FVector(-(J+1)*9,0,74+J*24),9);Beam(Crimson,Hand+FVector(-J*9,0,50+J*24),Hand+FVector(-J*9-24,0,70+J*24),4);}}
  else{FaceRing(Crimson,Hand+FVector(0,0,150),65);Beam(ShadowLimbs,Hand+FVector(-55,0,115),Hand+FVector(55,0,190),16);}
 }
 if(Id==TEXT("mazakin")){
  for(int32 H=0;H<2;H++)Beam(ShadowLimbs,Head+FVector(0,H?25:-25,10),Head+FVector(25,H?60:-60,75),10);
  if(B(U,TEXT("ascended"))&&RenderAlive[I]){
   FVector Root=P-FVector(0,0,80);Mount->AddInstance(FTransform(FRotator::ZeroRotator,Root+FVector(0,0,90),FVector(2.5f,.8f,1.25f)),true);Mount->AddInstance(FTransform(FRotator(20,0,0),Root+FVector(-125,0,160),FVector(.65f,.58f,1.15f)),true);Mount->AddInstance(FTransform(FRotator::ZeroRotator,Root+FVector(-160,0,160),FVector(.8f,.5f,.45f)),true);
   Beam(ShadowLimbs,Root+FVector(-70,0,90),Root+FVector(-125,0,150),22);
   for(int32 Leg=0;Leg<4;Leg++){float X=Leg<2?-80:80,Y=Leg%2?-30:30,Swing=Skill==TEXT("mazakin_charge")&&Age>=0?FMath::Sin(Age*14+Leg*PI)*35:0;FVector Hip=Root+FVector(X,Y,80),Knee=Root+FVector(X+Swing,Y,40),Foot=Root+FVector(X-Swing,Y,8);Beam(ShadowLimbs,Hip,Knee,12);Beam(ShadowLimbs,Knee,Foot,8);Ring(Crimson,Foot,13);}
   for(int32 H=0;H<2;H++)Beam(ShadowLimbs,Root+FVector(-130,H?18:-18,195),Root+FVector(-100,H?35:-35,235),7);
  }
 }
 if(Id==TEXT("zhujin")&&B(U,TEXT("crimson"))){
  FVector Center=P+FVector(0,-55,200);for(int32 J=0;J<28;J++){float A=J*2*PI/28,Wave=ReducedMotion?0:FMath::Sin(Clock*1.5f+J)*8;FVector Petal=Center+FVector(FMath::Cos(A)*(120+Wave),0,FMath::Sin(A)*(120+Wave));Petals->AddInstance(FTransform(FRotator(A*180/PI,0,0),Petal,FVector(2.f,.2f,.6f)),true);}FaceRing(Crimson,Center-FVector(0,10,0),75);for(int32 Vine=0;Vine<6;Vine++){float A=Vine*2*PI/6;FVector Tip=P+FVector(FMath::Cos(A)*230,FMath::Sin(A)*150,35);Beam(ShadowLimbs,P+FVector(0,0,50),Tip,10);FaceRing(Gold,Tip,22);}
 }
}
void AEnigmaWorldPresentation::DrawNewSkill(FVector Source){
 bool Divine=Skill.StartsWith(TEXT("tenrin_"))||Skill.StartsWith(TEXT("zevalon_"));auto Batch=Skill.StartsWith(TEXT("zevalon_"))?Lightning:Gold;
 for(int32 V:Victims){FVector Target=Bodies[V]->GetComponentLocation()+FVector(0,0,115);
  if(Divine){
   if(Operator==TEXT("divine_guard")){FaceRing(Gold,Target,85+FMath::Sin(Age*3)*15);Ring(Lightning,Target-FVector(0,0,95),110);continue;}
   float T=FMath::Clamp(Age/.65f,0.f,1.f);FVector Tip=FMath::Lerp(Source,Target,T);for(int32 J=0;J<8;J++){float A=J*2*PI/8+Age*8;Beam(Batch,Tip+FVector(0,FMath::Cos(A)*25,FMath::Sin(A)*25),Tip+FVector(65,FMath::Cos(A+.4f)*35,FMath::Sin(A+.4f)*35),4);}
   if(T>=1&&Age<1.4f){FaceRing(Gold,Target,(Age-.65f)*160+40);if(Skill.StartsWith(TEXT("zevalon_")))for(int32 J=0;J<8;J++)Beam(Lightning,Target+FVector(FMath::Sin(J*3.f)*20,0,420-J*50),Target+FVector(FMath::Sin((J+1)*3.f)*20,0,370-J*50),5);}
  }
  if(Skill==TEXT("zhujin_petals")){for(int32 J=0;J<18;J++){float T=FMath::Clamp(Age/.8f-J*.015f,0.f,1.f);FVector P=FMath::Lerp(Source,Target,T)+FVector(0,FMath::Sin(J+Age*8)*60*T*(1-T),FMath::Cos(J+Age*6)*90*T*(1-T));Petals->AddInstance(FTransform(FRotator(Age*160,J*20,Age*90),P,FVector(.45f,.2f,.06f)),true);}}
  if(Skill==TEXT("mazakin_threat")){Ring(Crimson,Target-FVector(0,0,100),50+Age*60);for(int32 J=0;J<4;J++)Beam(ShadowLimbs,Target-FVector(0,0,100),Target+FVector(FMath::Sin(J*2.f)*55,FMath::Cos(J*2.f)*55,80),6);}
  if(Skill==TEXT("hajun_final")){FaceRing(Crimson,FVector(0,0,220),70+FMath::Sin(Age/Duration*PI)*110);Ball(Dark,FVector(0,0,220),55+FMath::Sin(Age/Duration*PI)*90);}
 }
}

// Costume geometry follows animated bones; no painted attack cards are used.
void AEnigmaWorldPresentation::DrawAkurothCostume(int32 I,TSharedPtr<FJsonObject> U){
 const bool PhaseTwo=B(U,TEXT("ascended"));
 const float Scale=Bodies[I]->GetComponentScale().X;
 const bool WindEnabled=!ReducedMotion&&RenderAlive[I];
 auto Bone=[&](const TCHAR* Name){return Bodies[I]->GetBoneLocation(Name);};
 const FVector Chest=Bone(TEXT("spine_03")),Hip=Bone(TEXT("pelvis")),Head=Bone(TEXT("head"));
 auto Existing=Garments.Find(Keys[I]);UProceduralMeshComponent* Cloth=Existing?Existing->Get():nullptr;
 if(!Cloth){
  Cloth=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Cloth);Cloth->SetupAttachment(RootComponent);
  Cloth->SetCollisionEnabled(ECollisionEnabled::NoCollision);Cloth->RegisterComponent();
  auto Mat=UMaterialInstanceDynamic::Create(Material,this);Mat->SetVectorParameterValue(TEXT("Color"),FLinearColor(.025f,.006f,.012f));Mat->SetScalarParameterValue(TEXT("Glow"),.015f);Cloth->SetMaterial(0,Mat);Garments.Add(Keys[I],Cloth);
 }
 TArray<FVector> Vertices,Normals;TArray<FVector2D> UV;TArray<int32> Triangles;TArray<FLinearColor> VertexTints;TArray<FProcMeshTangent> Tangents;
 // Separate ribbons create ragged openings rather than a solid rectangular cape.
 for(int32 Panel=0;Panel<12;Panel++){
  const int32 Start=Vertices.Num();const float Angle=Panel*2*PI/12;
  for(int32 Row=0;Row<=10;Row++)for(int32 Side=0;Side<2;Side++){
   const float T=Row/10.f,A=Angle+(Side?1:-1)*.20f;
   const float Wind=WindEnabled?FMath::Sin(Clock*1.7f+Panel*.8f+T*5)*T*T:0;
   const float Length=(Panel%3==0?91.f:Panel%3==1?112.f:103.f)*Scale;
   const float Radius=(23+T*30+Wind*7)*Scale;
   const FVector Root=FMath::Lerp(Chest,Hip,FMath::Clamp(T*3,0.f,1.f));
   Vertices.Add(Root+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,-T*Length+Wind*5*Scale));
   Normals.Add(FVector(FMath::Cos(A),FMath::Sin(A),.2f).GetSafeNormal());UV.Add(FVector2D(Side,T));VertexTints.Add(FLinearColor::White);Tangents.Add(FProcMeshTangent(-FMath::Sin(A),FMath::Cos(A),0));
  }
  for(int32 Row=0;Row<10;Row++){int32 A=Start+Row*2,C=A+2;Triangles.Append({A,C,A+1,A+1,C,C+1,A+1,C,A,C+1,C,A+1});}
  // Red seams follow each panel without turning the entire garment into a light source.
  for(int32 Row=4;Row<10;Row++)Beam(Crimson,Vertices[Start+Row*2],Vertices[Start+(Row+1)*2],.55f*Scale);
 }
 if(Cloth->GetNumSections()==0)Cloth->CreateMeshSection_LinearColor(0,Vertices,Triangles,Normals,UV,VertexTints,Tangents,false);
 else Cloth->UpdateMeshSection_LinearColor(0,Vertices,Normals,UV,VertexTints,Tangents);
 // Chest eclipse and shoulder armour.
 FVector Crest=Chest+FVector(-23*Scale,0,0);Ball(Dark,Crest,10*Scale);FaceRing(Crimson,Crest,12*Scale);
 for(int32 Side=-1;Side<=1;Side+=2){
  FVector Shoulder=Bone(Side<0?TEXT("upperarm_l"):TEXT("upperarm_r"));
  Ball(ShadowLimbs,Shoulder,16*Scale);
  for(int32 Spike=0;Spike<3;Spike++)Beam(ShadowLimbs,Shoulder+FVector(0,Side*Spike*5*Scale,0),Shoulder+FVector((Spike-1)*9*Scale,Side*(12+Spike*7)*Scale,(22-Spike*3)*Scale),3*Scale);
 }
 // Crown, dark hair in phase one, white hair already drawn by the phase-two renderer.
 for(int32 Spike=0;Spike<5;Spike++){
  FVector Root=Head+FVector(-3*Scale,(Spike-2)*7*Scale,12*Scale);
  Beam(ShadowLimbs,Root,Root+FVector((Spike-2)*3*Scale,0,(Spike%2?24:34)*Scale),2*Scale);
  Ball(Crimson,Root,1.2f*Scale);
 }
 if(!PhaseTwo)for(int32 Strand=0;Strand<18;Strand++)for(int32 J=0;J<5;J++){
  float Wave=WindEnabled?FMath::Sin(Clock*1.5f+Strand*.6f+J*.5f)*3:0;
  FVector A=Head+FVector((4+J*5)*Scale,((Strand-9)*1.7f+Wave)*Scale,(12-J*9)*Scale);
  FVector End=A+FVector(5*Scale,0,-9*Scale);Beam(ShadowLimbs,A,End,(1.7f-J*.18f)*Scale);
 }
 // Gold chain links drape around the waist and move with the pelvis.
 for(int32 Chain=0;Chain<2;Chain++)for(int32 Link=0;Link<24;Link++){
  float A=Link*2*PI/24;FVector P=Hip+FVector(FMath::Cos(A)*29*Scale,FMath::Sin(A)*29*Scale,(-10-Chain*12-8*FMath::Abs(FMath::Sin(A)))*Scale);
  Beam(Gold,P,P+FVector(-FMath::Sin(A)*3*Scale,FMath::Cos(A)*3*Scale,2*Scale),.65f*Scale);
 }
 const FVector Hand=Bone(TEXT("hand_r"));
 const FVector Axis=Bodies[I]->GetBoneQuaternion(TEXT("hand_r")).RotateVector(FVector(0,1,0)).GetSafeNormal();
 const FVector Edge=FVector::CrossProduct(Axis,FVector(0,0,1)).GetSafeNormal();
 Beam(ShadowLimbs,Hand-Axis*15*Scale,Hand+Axis*70*Scale,3.5f*Scale);
 Beam(Gold,Hand-Axis*3*Scale-Edge*12*Scale,Hand-Axis*3*Scale+Edge*12*Scale,1.5f*Scale);
 for(int32 J=0;J<8;J++){
  FVector A=Hand+Axis*(8+J*7)*Scale,End=Hand+Axis*(15+J*7)*Scale;
  Beam(Crimson,A+Edge*(7-J*.6f)*Scale,End+Edge*(6.4f-J*.6f)*Scale,.7f*Scale);
  Beam(ShadowLimbs,A,End+Edge*(J%2?5:-5)*Scale,(4-J*.35f)*Scale);
 }
}
