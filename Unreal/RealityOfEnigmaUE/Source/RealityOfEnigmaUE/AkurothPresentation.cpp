#include "AkurothPresentation.h"
#include "AkurothArenaMode.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundWave.h"
#include "ProceduralMeshComponent.h"

AAkurothPresentation::AAkurothPresentation(){PrimaryActorTick.bCanEverTick=true;SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Scene")));}
UMaterialInstanceDynamic* AAkurothPresentation::Material(FLinearColor Color,float Glow){
    auto* M=UMaterialInstanceDynamic::Create(BaseMaterial,this);M->SetVectorParameterValue(TEXT("Color"),Color);M->SetScalarParameterValue(TEXT("Glow"),Glow);Materials.Add(M);return M;
}
UInstancedStaticMeshComponent* AAkurothPresentation::Batch(UStaticMesh* Mesh,UMaterialInterface* Mat){
    auto* C=NewObject<UInstancedStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(GetRootComponent());C->SetStaticMesh(Mesh);C->SetMaterial(0,Mat);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->SetCastShadow(false);C->RegisterComponent();return C;
}
void AAkurothPresentation::Instance(UInstancedStaticMeshComponent* C,FVector P,FVector S){C->AddInstance(FTransform(FQuat::Identity,P,S));}
void AAkurothPresentation::Beam(UInstancedStaticMeshComponent* C,FVector A,FVector B,float Width){
    FVector Delta=B-A;C->AddInstance(FTransform(FQuat::FindBetweenNormals(FVector::UpVector,Delta.GetSafeNormal()),(A+B)*.5,FVector(Width/100,Width/100,Delta.Size()/100)));
}
void AAkurothPresentation::Ring(UInstancedStaticMeshComponent* C,FVector P,float Radius,float Tilt,int32 N){
    for(int32 I=0;I<N;++I){float A=I*2*PI/N,B=(I+1)*2*PI/N;
        FVector V(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,FMath::Sin(A+Clock)*Tilt);
        FVector W(FMath::Cos(B)*Radius,FMath::Sin(B)*Radius,FMath::Sin(B+Clock)*Tilt);Beam(C,P+V,P+W,7);}
}
void AAkurothPresentation::StaticShape(UStaticMesh* Mesh,FVector P,FVector S,UMaterialInterface* Mat,FRotator R){
    auto* C=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(GetRootComponent());C->SetStaticMesh(Mesh);C->SetMaterial(0,Mat);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();C->SetWorldLocation(P);C->SetWorldRotation(R);C->SetWorldScale3D(S);
}
void AAkurothPresentation::Initialize(AAkurothArenaMode* Combat,AActor* Hero,AActor* Boss,ACameraActor* Camera){
    Mode=Combat;HeroRoot=Hero;BossRoot=Boss;View=Camera;
    Ball=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere"));Cylinder=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));Block=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
    BaseMaterial=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Akuroth/Materials/M_Combat.M_Combat"));
    USkeletalMesh* Manny=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
    Idle=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle.MM_Idle"));
    Attack=LoadObject<UAnimSequence>(nullptr,TEXT("/Game/Characters/Mannequins/Anims/Unarmed/Attack/MM_Attack_01.MM_Attack_01"));
    AssetsReady=BaseMaterial&&Manny&&Idle&&Attack;
    UE_LOG(LogTemp,Display,TEXT("AKUROTH_PRESENTATION assets_ready=%d"),AssetsReady);
    if(!AssetsReady)return;
    for(AActor* Root:{Hero,Boss}){if(auto* Mesh=Cast<UStaticMeshComponent>(Root->GetRootComponent()))Mesh->SetVisibility(false);Root->SetActorScale3D(FVector(1));}
    auto* Stone=Material(FLinearColor(.025f,.018f,.03f));auto* Armor=Material(FLinearColor(.012f,.008f,.015f));
    auto* Crimson=Material(FLinearColor(.32f,.009f,.018f));auto* Lava=Material(FLinearColor(1,.045f,.004f),1.5f);
    Red=Batch(Cylinder,Material(FLinearColor(1,.008f,.025f),6));Purple=Batch(Cylinder,Material(FLinearColor(.65f,.015f,1),5));
    Shadow=Batch(Ball,Armor);Void=Batch(Cylinder,Armor);Rubble=Batch(Block,Armor);Hair=Batch(Cylinder,Material(FLinearColor(.015f,.009f,.012f)));Sparks=Batch(Ball,Lava);Blood=Batch(Ball,Material(FLinearColor(.23f,.001f,.006f)));
    StaticShape(Cylinder,FVector(0,0,-70),FVector(27,23,1.2),Stone);
    StaticShape(Cylinder,FVector(0,0,-10),FVector(23,20,.06),Lava);
    // A basalt pavement made of actual fractured slab geometry, with lava visible in the joints.
    auto* Pavement=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Pavement);Pavement->SetupAttachment(GetRootComponent());Pavement->RegisterComponent();Pavement->SetMaterial(0,Stone);
    TArray<FVector> PV,PN;TArray<int32> PT;TArray<FVector2D> PU;FRandomStream Random(7819);
    for(int32 X=-13;X<13;++X)for(int32 Y=-11;Y<11;++Y){float PX=X*80+40,PY=Y*80+40;if(FMath::Square(PX/1060)+FMath::Square(PY/880)>1)continue;
        int32 Start=PV.Num();float Z=Random.FRandRange(0,11);float Corners[4][2]={{-35,-35},{35,-35},{35,35},{-35,35}};
        for(int32 Side=0;Side<2;++Side)for(int32 J=0;J<4;++J){PV.Add(FVector(PX+Corners[J][0]+Random.FRandRange(-3,3),PY+Corners[J][1]+Random.FRandRange(-3,3),Side?Z:-8));PN.Add(FVector::UpVector);PU.Add(FVector2D(J%2,J/2));}
        const int32 Faces[]={4,5,6,4,6,7,0,4,1,1,4,5,1,5,2,2,5,6,2,6,3,3,6,7,3,7,0,0,7,4};for(int32 I:Faces)PT.Add(Start+I);
    }
    Pavement->CreateMeshSection_LinearColor(0,PV,PT,PN,PU,{}, {},false);
    if(auto* Backdrop=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Akuroth/Environment/M_InfernoBackdrop.M_InfernoBackdrop"))){
        BackdropMaterial=UMaterialInstanceDynamic::Create(Backdrop,this);BackdropMaterial->SetScalarParameterValue(TEXT("Brightness"),.25f);
        auto* Sky=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Sky);Sky->SetupAttachment(GetRootComponent());Sky->SetCastShadow(false);Sky->RegisterComponent();Sky->SetMaterial(0,BackdropMaterial);
        TArray<FVector> V={FVector(-3000,-2200,-350),FVector(3000,-2200,-350),FVector(3000,-2200,1100),FVector(-3000,-2200,1100)};
        Sky->CreateMeshSection_LinearColor(0,V,{0,1,2,0,2,3},{FVector(0,1,0),FVector(0,1,0),FVector(0,1,0),FVector(0,1,0)},{FVector2D(0,1),FVector2D(1,1),FVector2D(1,0),FVector2D(0,0)},{},{},false);
    }
    for(int32 N=0;N<14;++N){float A=N*2*PI/14;FVector P(FMath::Cos(A)*1500,FMath::Sin(A)*1300,0);if(P.Y>-100)continue;
        StaticShape(Block,P+FVector(0,0,420),FVector(1.1,1.1,8.4),Stone,FRotator(0,N*17,0));
        StaticShape(Block,P+FVector(0,0,860),FVector(1.8,1.8,.5),Armor);
        for(int32 K=0;K<3;++K)StaticShape(Block,P+FVector((K-1)*60,0,940),FVector(.22,.45,1.1),Stone);
        if(N%3==0){StaticShape(Block,P*.88+FVector(0,0,60),FVector(.9,.9,1.2),Armor);}
    }
    for(int32 N=0;N<32;++N){float A=N*2*PI/32;StaticShape(Block,FVector(FMath::Cos(A)*1000,FMath::Sin(A)*850,-1),FVector(2.2,.12,.06),Lava,FRotator(0,A*180/PI,0));}
    FLinearColor Colors[]={FLinearColor(.03f,.2f,.12f),FLinearColor(.08f,.3f,.75f),FLinearColor(.6f,.28f,.045f),FLinearColor(.33f,.04f,.6f),FLinearColor(.5f,.035f,.025f)};
    USkeletalMesh* Quinn=LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/Characters/Mannequins/Meshes/SKM_Quinn_Simple.SKM_Quinn_Simple"));
    for(int32 N=0;N<5;++N){auto* C=NewObject<USkeletalMeshComponent>(this);AddInstanceComponent(C);C->SetupAttachment(GetRootComponent());C->SetSkeletalMesh(Quinn&&(N==1||N==3)?Quinn:Manny);C->SetCollisionEnabled(ECollisionEnabled::NoCollision);C->RegisterComponent();C->SetWorldScale3D(FVector(1.35));C->SetWorldRotation(FRotator(0,-90,0));
        auto* HeroMat=Material(Colors[N]);for(int32 J=0;J<C->GetNumMaterials();++J)C->SetMaterial(J,HeroMat);C->PlayAnimation(Idle,true);Heroes.Add(C);HeroHomes.Add(FVector(-750+N*130,150+FMath::Abs(N-2)*20,0));
        auto* Sword=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Sword);Sword->SetupAttachment(C,TEXT("hand_r"));Sword->SetStaticMesh(Block);Sword->SetMaterial(0,Material(Colors[N],5));Sword->SetCollisionEnabled(ECollisionEnabled::NoCollision);Sword->RegisterComponent();Sword->SetRelativeLocation(FVector(0,0,40));Sword->SetRelativeScale3D(FVector(.06,.09,.95));
    }
    BossBody=NewObject<USkeletalMeshComponent>(this);AddInstanceComponent(BossBody);BossBody->SetupAttachment(GetRootComponent());BossBody->SetSkeletalMesh(Manny);BossBody->SetCollisionEnabled(ECollisionEnabled::NoCollision);BossBody->RegisterComponent();BossBody->SetWorldScale3D(FVector(2.25));BossBody->SetWorldRotation(FRotator(0,90,0));for(int32 J=0;J<BossBody->GetNumMaterials();++J)BossBody->SetMaterial(J,Armor);BossBody->PlayAnimation(Idle,true);
    Cape=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Cape);Cape->SetupAttachment(GetRootComponent());Cape->SetCollisionEnabled(ECollisionEnabled::NoCollision);Cape->RegisterComponent();Cape->SetMaterial(0,Crimson);
    TArray<int32> Triangles;TArray<FVector> Normals;TArray<FVector2D> UV;TArray<FLinearColor> VertexColors;TArray<FProcMeshTangent> Tangents;
    for(int32 Y=0;Y<9;++Y)for(int32 X=0;X<13;++X){CapeVertices.Add(FVector(0));Normals.Add(FVector(-1,0,0));UV.Add(FVector2D(X/12.f,Y/8.f));VertexColors.Add(FLinearColor::White);Tangents.Add(FProcMeshTangent(0,1,0));}
    for(int32 Y=0;Y<8;++Y)for(int32 X=0;X<12;++X){int32 A=Y*13+X;Triangles.Append({A,A+13,A+1,A+1,A+13,A+14});}
    Cape->CreateMeshSection_LinearColor(0,CapeVertices,Triangles,Normals,UV,VertexColors,Tangents,false);
    for(int32 N=0;N<5;++N){auto* Cloth=NewObject<UProceduralMeshComponent>(this);AddInstanceComponent(Cloth);Cloth->SetupAttachment(GetRootComponent());Cloth->SetCollisionEnabled(ECollisionEnabled::NoCollision);Cloth->RegisterComponent();Cloth->SetMaterial(0,Material(Colors[N]*.5f));Cloth->CreateMeshSection_LinearColor(0,CapeVertices,Triangles,Normals,UV,VertexColors,Tangents,false);HeroCapes.Add(Cloth);}
    auto* Blade=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Blade);Blade->SetupAttachment(BossBody,TEXT("hand_r"));Blade->SetStaticMesh(Block);Blade->SetMaterial(0,Armor);Blade->SetCollisionEnabled(ECollisionEnabled::NoCollision);Blade->RegisterComponent();Blade->SetRelativeLocation(FVector(0,0,50));Blade->SetRelativeScale3D(FVector(.06f,.14f,1.35f));
    auto* Edge=NewObject<UStaticMeshComponent>(this);AddInstanceComponent(Edge);Edge->SetupAttachment(BossBody,TEXT("hand_r"));Edge->SetStaticMesh(Block);Edge->SetMaterial(0,Material(FLinearColor(1,.005f,.02f),4));Edge->SetCollisionEnabled(ECollisionEnabled::NoCollision);Edge->RegisterComponent();Edge->SetRelativeLocation(FVector(0,8,50));Edge->SetRelativeScale3D(FVector(.015f,.015f,1.35f));
    KeyLight=NewObject<UPointLightComponent>(this);AddInstanceComponent(KeyLight);KeyLight->SetupAttachment(GetRootComponent());KeyLight->SetIntensity(3000000);KeyLight->SetAttenuationRadius(3500);KeyLight->SetLightColor(FLinearColor(.32f,.24f,.55f));KeyLight->RegisterComponent();KeyLight->SetWorldLocation(FVector(-200,700,850));
    AssetsReady=AssetsReady&&BackdropMaterial&&HeroCapes.Num()==5;CameraHome=FVector(-150,1900,850);View->SetActorLocation(CameraHome);View->GetCameraComponent()->SetFieldOfView(58);View->SetActorRotation((FVector(80,0,260)-CameraHome).Rotation());
}
void AAkurothPresentation::Sound(int32 Index){
    const TCHAR* Names[]={TEXT("akuroth_cut"),TEXT("akuroth_waves"),TEXT("akuroth_prison"),TEXT("akuroth_meteor"),TEXT("akuroth_rift"),TEXT("akuroth_chaos"),TEXT("akuroth_collapse"),TEXT("akuroth_rain"),TEXT("akuroth_rewrite"),TEXT("akuroth_annihilation")};
    FString Path=FString::Printf(TEXT("/Game/Akuroth/Audio/%s.%s"),Names[Index],Names[Index]);if(auto* Wave=LoadObject<USoundWave>(nullptr,*Path))UGameplayStatics::PlaySound2D(this,Wave,.55f);
}
void AAkurothPresentation::PlaySkill(int32 Index){if(!AssetsReady)return;SkillIndex=Index;LastSkill=Index;SkillAge=0;BossBody->PlayAnimation(Attack,false);Sound(Index);}
void AAkurothPresentation::PlayHeroes(){if(!AssetsReady)return;HeroAttackAge=0;for(auto& Hero:Heroes)Hero->PlayAnimation(Attack,false);}
void AAkurothPresentation::Tick(float Dt){
    Super::Tick(Dt);if(!AssetsReady)return;Clock+=Dt;Red->ClearInstances();Purple->ClearInstances();Shadow->ClearInstances();Void->ClearInstances();Rubble->ClearInstances();Hair->ClearInstances();Sparks->ClearInstances();Blood->ClearInstances();
    const bool Phase=Mode->PhaseTwo;if(Phase!=WasPhaseTwo){WasPhaseTwo=Phase;KeyLight->SetLightColor(Phase?FLinearColor(.35f,.05f,.6f):FLinearColor(.32f,.24f,.55f));if(BackdropMaterial)BackdropMaterial->SetScalarParameterValue(TEXT("Brightness"),Phase?.55f:.25f);if(auto* HairMat=Cast<UMaterialInstanceDynamic>(Hair->GetMaterial(0)))HairMat->SetVectorParameterValue(TEXT("Color"),Phase?FLinearColor(.82f,.75f,.86f):FLinearColor(.015f,.009f,.012f));}
    FVector B=BossRoot->GetActorLocation()-FVector(0,0,190);float Casting=SkillAge>=0?FMath::Sin(FMath::Clamp(SkillAge/1.8f,0.f,1.f)*PI):0;
    if(SkillIndex==0&&SkillAge>=0)B.X-=Casting*650;BossBody->SetWorldLocation(B);
    float Pull=FMath::Clamp(float((HeroRoot->GetActorLocation().X+550)/490),0.f,1.f);
    if(Mode->HeroHP<PreviousHP){HitAge=0;}PreviousHP=Mode->HeroHP;
    for(int32 N=0;N<5;++N){FVector P=FMath::Lerp(HeroHomes[N],FVector(-60,(N-2)*30,100),Pull);
        if(HeroAttackAge>=0)P=FMath::Lerp(P,FVector(B.X-140,(N-2)*60,B.Z),FMath::Sin(FMath::Clamp(HeroAttackAge/1.4f,0.f,1.f)*PI));
        if(HitAge>=0){P.X-=FMath::Sin(HitAge*12)*35*FMath::Max(0.f,1-HitAge);P.Z+=FMath::Sin(HitAge*6)*15;}
        Heroes[N]->SetWorldLocation(P);Heroes[N]->SetWorldRotation(FRotator(0,(B-P).Rotation().Yaw-90,0));if(HeroAttackAge>.45f&&HeroAttackAge<.9f)Beam(N%2?Purple:Red,Heroes[N]->GetSocketLocation(TEXT("hand_r")),BossBody->GetSocketLocation(TEXT("spine_03")),5);Ring(N%2?Purple:Red,P+FVector(0,0,3),38,0,12);
        TArray<FVector> ClothVertices;
        for(int32 Y=0;Y<9;++Y)for(int32 X=0;X<13;++X){float V=Y/8.f,U=(X-6)/6.f;ClothVertices.Add(P+FVector(-18-V*45+FMath::Sin(Clock*3+U+V*5)*V*18,U*(20+V*32),180-V*150));}
        HeroCapes[N]->UpdateMeshSection_LinearColor(0,ClothVertices,{}, {}, {}, {});
    }
    if(HeroAttackAge>=0){HeroAttackAge+=Dt;if(HeroAttackAge>1.4f){HeroAttackAge=-1;for(auto& Hero:Heroes)Hero->PlayAnimation(Idle,true);}}
    if(HitAge>=0){HitAge+=Dt;for(int32 N=0;N<45;++N){float A=N*2.399f;FVector P=Heroes[N%5]->GetComponentLocation()+FVector(FMath::Cos(A)*HitAge*160,FMath::Sin(A)*HitAge*110,100+HitAge*100-HitAge*HitAge*200);Instance(Blood,P,FVector(.03));}if(HitAge>1)HitAge=-1;}
    // The cloak is real mesh geometry with a continuous wind deformation.
    for(int32 Y=0;Y<9;++Y)for(int32 X=0;X<13;++X){float V=Y/8.f,U=(X-6)/6.f;CapeVertices[Y*13+X]=B+FVector(30+V*140+FMath::Sin(Clock*2+U*3+V*4)*V*35,U*(50+V*170),310-V*(280+FMath::Sin(X*1.7f)*25));}
    Cape->UpdateMeshSection_LinearColor(0,CapeVertices,{}, {}, {}, {});
    FVector Head=BossBody->GetSocketLocation(TEXT("head"))+FVector(0,0,15),Heart=BossBody->GetSocketLocation(TEXT("spine_03"))+FVector(-42,-10,0);
    Instance(Shadow,Heart,FVector(.36));Ring(Red,Heart,26,26,24);
    for(int32 N=0;N<18;++N){float A=N*2*PI/18;FVector Prev=Head+FVector(FMath::Cos(A)*20,FMath::Sin(A)*30,0);
        for(int32 J=1;J<7;++J){float V=J/6.f;FVector Next=Head+FVector(20+V*55+FMath::Sin(Clock*2+A+V*3)*V*25,FMath::Sin(A)*(30+V*70),-V*(Phase?220:75));Beam(Hair,Prev,Next,Phase?4:2);Prev=Next;}}
    for(int32 Side=-1;Side<=1;Side+=2){FVector P=Head+FVector(0,Side*25,5);for(int32 J=0;J<6;++J){FVector Q=P+FVector(J<3?-8:12,Side*12,20);Beam(Void,P,Q,20-J*2);Beam(Red,P+FVector(0,Side*8,0),Q+FVector(0,Side*8,0),2);P=Q;}}
    if(Phase){for(int32 N=0;N<6;++N){float A=(N-2.5f)*.48f;FVector S=B+FVector(30,(N%2?1:-1)*45,290);FVector E=B+FVector(FMath::Cos(A+Clock*.4f)*180,FMath::Sin(A+Clock*.4f)*240,260+FMath::Sin(N+Clock)*95);
            FVector Previous=S;for(int32 J=0;J<10;++J){float V=J/9.f;FVector Point=FMath::Lerp(S,E,V)+FVector(0,0,FMath::Sin(V*PI)*80);Instance(Shadow,Point,FVector(.24f*(1-V*.55f)));if(J>0)Beam(Void,Previous,Point,24*(1-V*.55f));Previous=Point;}for(int32 Finger=0;Finger<5;++Finger)Beam(Void,E+FVector(0,(Finger-2)*5,0),E+FVector(-10,(Finger-2)*12,-35),6);Ring(Purple,E,22,8,12);}}
    // Burning fissures, rising embers and a distant eclipse remain alive between turns.
    for(int32 N=0;N<100;++N){float A=N*2.399f,R=650+(N%11)*60,Z=FMath::Fmod(Clock*(50+N%5*15)+N*23,360);Instance(Sparks,FVector(FMath::Cos(A)*R,FMath::Sin(A)*R,Z),FVector(.015f+(N%3)*.01f));}
    for(int32 N=0;N<14;++N){float A=N*2*PI/14;FVector P(FMath::Cos(A)*1300,FMath::Sin(A)*1130,65);for(int32 J=0;J<4;++J){float H=FMath::Fmod(Clock*90+J*40+N*17,160);Instance(Sparks,P+FVector(FMath::Sin(Clock*5+N+J)*12,0,H),FVector(.22f,.22f,.8f-H*.003f));}}
    if(Phase&&FMath::Fmod(Clock,5.f)<.22f){FVector P(900,700,1400);for(int32 J=0;J<9;++J){FVector Q=P+FVector(FMath::Sin(J*19.f)*70,FMath::Cos(J*23.f)*60,-140);Beam(Purple,P,Q,9);P=Q;}}
    FVector Eclipse(650,-1050,400);Instance(Shadow,Eclipse,FVector(6));
    for(int32 N=0;N<80;++N){float A=N*2*PI/80;FVector P=Eclipse+FVector(FMath::Cos(A)*310,0,FMath::Sin(A)*310);FVector Q=Eclipse+FVector(FMath::Cos(A+2*PI/80)*310,0,FMath::Sin(A+2*PI/80)*310);Beam(Red,P,Q,12);}
    for(int32 N=0;N<18;++N){float A=N*2.399f;Rubble->AddInstance(FTransform(FRotator(Clock*20+N*13,Clock*10+N*19,N*11),FVector(FMath::Cos(A+Clock*.07f)*1100,FMath::Sin(A+Clock*.07f)*1000,550+FMath::Sin(Clock+N)*100),FVector(.6f,.32f,.2f)));}
    if(Mode->Charging)Ring(Purple,B+FVector(0,0,250),150+FMath::Sin(Clock*3)*20,100);
    if(SkillAge>=0){SkillAge+=Dt;float T=SkillAge,V=FMath::Clamp(T/1.1f,0.f,1.f);FVector H(-490,175,120);
        if(SkillIndex==0||SkillIndex==5){for(int32 N=0;N<(SkillIndex==5?7:3);++N){float A=T*7+N*.5f;FVector P=FMath::Lerp(B+FVector(0,0,230),H,FMath::Clamp(T/.55f,0.f,1.f));Beam(Red,P+FVector(0,FMath::Cos(A)*140,FMath::Sin(A)*140),P+FVector(-70,-FMath::Cos(A)*140,-FMath::Sin(A)*140),12);}}
        if(SkillIndex==1){for(int32 N=0;N<3;++N)Ring(Red,FVector(550-V*1040,V*175,40+N*50),80+V*220,30);}
        if(SkillIndex==2){for(int32 N=0;N<5;++N){FVector P=HeroHomes[N];for(int32 J=0;J<18;++J){float A=J*.65f+T;Instance(Shadow,P+FVector(FMath::Cos(A)*50,FMath::Sin(A)*50,J*14),FVector(.1));}Ring(Purple,P+FVector(0,0,10),60);}}
        if(SkillIndex==3||SkillIndex==7){for(int32 N=0;N<(SkillIndex==7?9:3);++N){float Local=FMath::Clamp(T-N*.07f,0.f,1.5f);FVector P=HeroHomes[N%5]+FVector((N/5)*35,0,900-Local*680);Instance(Shadow,P,FVector(.9));Ring(Red,P,55,20,24);Beam(Red,P,P+FVector(170,0,250),18);if(P.Z<100)Ring(Red,FVector(P.X,P.Y,10),(T-1.1f)*600);}}
        if(SkillIndex==4||SkillIndex==8){for(int32 N=0;N<5;++N){FVector P=HeroHomes[N];Beam(Purple,P+FVector(-40,0,5),P+FVector(40,0,350*FMath::Sin(FMath::Min(T/2,1.f)*PI)),12);Ring(Purple,P+FVector(0,0,150),80,70);}}
        if(SkillIndex==6){Ring(Purple,FVector(-490,175,20),100+T*350,60);for(int32 N=0;N<28;++N){float A=N*2.399f;Instance(Shadow,FVector(-490+FMath::Cos(A)*T*130,175+FMath::Sin(A)*T*150,100+FMath::Sin(T*3+N)*130),FVector(.3));}}
        if(SkillIndex==9){FVector P(0,0,230);float R=T<3?30+T*65:225+(T-3)*260;Instance(Shadow,P,FVector(R/50));Ring(Red,P,R+20,100);Ring(Purple,P,R+45,160);
            for(int32 N=0;N<60;++N){float A=N*2.399f+T*3,Radius=400-FMath::Fmod(T*180+N*23,350);Instance(Sparks,P+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,FMath::Sin(A*.7f)*100),FVector(.025));}}
        if(T>(SkillIndex==9?4.6f:2.4f)){SkillAge=-1;BossBody->PlayAnimation(Idle,true);}
    }
    float Shake=(HitAge>=0?.9f:0)+(SkillIndex==9&&SkillAge>2.9f&&SkillAge<3.8f?4.f:0);
    float Transformation=Mode->TransformationProgress(),Focus=FMath::Sin(Transformation*PI);
    FVector Camera=FMath::Lerp(CameraHome,FVector(B.X-250,1050,650),Focus*.55f);View->SetActorLocation(Camera+FVector(FMath::Sin(Clock*63)*Shake,0,FMath::Cos(Clock*71)*Shake));
    View->SetActorRotation((FMath::Lerp(FVector(80,0,260),B+FVector(0,0,260),Focus*.6f)-Camera).Rotation());
    if(Transformation<1){Ring(Red,B+FVector(0,0,30),100+Transformation*450,100);Ring(Purple,B+FVector(0,0,250),150+Focus*100,150);}
}
