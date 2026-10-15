#include "EnigmaModelPreview.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Engine/SkeletalMesh.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Materials/MaterialInterface.h"
AEnigmaModelPreview::AEnigmaModelPreview(){
 PrimaryActorTick.bCanEverTick=false;
 auto Root=CreateDefaultSubobject<USceneComponent>(TEXT("Studio"));SetRootComponent(Root);
 Model=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Hajun"));Model->SetupAttachment(Root);Model->SetCollisionEnabled(ECollisionEnabled::NoCollision);Model->SetCastShadow(true);
 AnimatedModel=CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("AnimatedHajun"));AnimatedModel->SetupAttachment(Root);AnimatedModel->SetCollisionEnabled(ECollisionEnabled::NoCollision);AnimatedModel->SetVisibility(false);AnimatedModel->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
 Capture=CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("PreviewCamera"));Capture->SetupAttachment(Root);
 Capture->bCaptureEveryFrame=false;Capture->bCaptureOnMovement=false;Capture->FOVAngle=40;
 Capture->PrimitiveRenderMode=ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
 Capture->CaptureSource=ESceneCaptureSource::SCS_FinalColorLDR;
 Capture->ShowFlags.SetAtmosphere(false);Capture->ShowFlags.SetFog(false);Capture->ShowFlags.SetMotionBlur(false);
 Capture->PostProcessSettings.bOverride_AutoExposureMethod=true;Capture->PostProcessSettings.AutoExposureMethod=EAutoExposureMethod::AEM_Manual;
 Capture->PostProcessSettings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Capture->PostProcessSettings.AutoExposureApplyPhysicalCameraExposure=false;
 Capture->PostProcessSettings.bOverride_AutoExposureBias=true;Capture->PostProcessSettings.AutoExposureBias=0;
 for(int32 I=0;I<3;I++){
  auto Light=CreateDefaultSubobject<UPointLightComponent>(*FString::Printf(TEXT("StudioLight%d"),I));Light->SetupAttachment(Root);
  Light->SetRelativeLocation(I==0?FVector(150,160,240):I==1?FVector(-180,80,150):FVector(0,-160,230));
  Light->Intensity=I==0?18000:I==1?9000:16000;Light->AttenuationRadius=650;Light->bUseInverseSquaredFalloff=true;
  Light->SetLightColor(I==2?FLinearColor(1,.35f,.2f):FLinearColor(1,.9f,.82f));
 }
}
void AEnigmaModelPreview::BeginPlay(){
 Super::BeginPlay();SetActorLocation(FVector(0,0,-20000));
 Model->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/CharacterModels/Hajun/SM_HajunPhase1.SM_HajunPhase1")));
 if(auto Material=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/CharacterModels/Hajun/M_HajunPhase1.M_HajunPhase1")))Model->SetMaterial(0,Material);
 AnimatedModel->SetSkeletalMesh(LoadObject<USkeletalMesh>(nullptr,TEXT("/Game/CharacterModels/Hajun/SK_HajunPhase1.SK_HajunPhase1")));AnimatedModel->SetMaterial(0,Model->GetMaterial(0));
 if(auto Mesh=AnimatedModel->GetSkeletalMeshAsset()){const float Height=Mesh->GetBounds().BoxExtent.Z*2;AnimatedModel->SetRelativeScale3D(FVector(Height>1?180.f/Height:1.f));}
 for(const TCHAR* Name:{TEXT("MM_Idle"),TEXT("MM_Attack_01"),TEXT("MM_Attack_02"),TEXT("MM_Attack_03"),TEXT("MM_ChargedAttack")}){FString N=TEXT("Hajun_")+FString(Name);MotionClips.Add(LoadObject<UAnimSequence>(nullptr,*(TEXT("/Game/CharacterModels/Hajun/Animations/")+N+TEXT(".")+N)));}
 Target=NewObject<UTextureRenderTarget2D>(this);Target->ClearColor=FLinearColor(.025f,.025f,.035f,1);Target->InitAutoFormat(768,768);Target->UpdateResourceImmediate(true);Capture->TextureTarget=Target;
 Capture->ShowOnlyComponent(Model);Capture->ShowOnlyComponent(AnimatedModel);Rotate(0);
}
bool AEnigmaModelPreview::Ready() const{return Model&&Model->GetStaticMesh()&&Model->GetMaterial(0)&&Target;}
void AEnigmaModelPreview::SetActive(bool Value){if(Active==Value)return;Active=Value;Capture->bCaptureEveryFrame=Active;AnimatedModel->SetComponentTickEnabled(Active&&Motion);}
void AEnigmaModelPreview::Rotate(float Degrees){
 Angle=FMath::Fmod(Angle+Degrees,360.f);FVector Center(0,0,90);float R=FMath::DegreesToRadians(Angle);
 float Distance=Motion?410.f:300.f;FVector Position(Distance*FMath::Sin(R),-Distance*FMath::Cos(R),105);Capture->SetRelativeLocation(Position);Capture->SetRelativeRotation((Center-Position).Rotation());
}
bool AEnigmaModelPreview::MotionReady() const{if(!AnimatedModel->GetSkeletalMeshAsset()||MotionClips.Num()!=5)return false;for(auto Clip:MotionClips)if(!Clip)return false;return true;}
bool AEnigmaModelPreview::PlayMotion(int32 Index){if(!MotionReady()||!MotionClips.IsValidIndex(Index))return false;Motion=true;Model->SetVisibility(false);AnimatedModel->SetVisibility(true);AnimatedModel->SetComponentTickEnabled(true);AnimatedModel->PlayAnimation(MotionClips[Index],true);Rotate(0);return true;}
void AEnigmaModelPreview::ShowStatic(){Motion=false;Model->SetVisibility(true);AnimatedModel->SetVisibility(false);AnimatedModel->SetComponentTickEnabled(false);Rotate(0);}
FVector AEnigmaModelPreview::HandLocation() const{return AnimatedModel->GetBoneLocation(TEXT("hand_r"));}
