#include "EnigmaStoryEnvironment.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/PostProcessComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"

bool AEnigmaStoryEnvironment::ConfigureBlenderForest(){
 const FString Folder=TEXT("/Game/StoryEnvironments/Forest/");
 auto Load=[&](const TCHAR* Name){return LoadObject<UStaticMesh>(nullptr,*(Folder+Name+TEXT(".")+Name));};
 auto Clearing=Load(TEXT("LimiarClearing")),Tree=Load(TEXT("LimiarTree")),Fern=Load(TEXT("LimiarFern"));
 FString Data;TSharedPtr<FJsonObject> Layout;
 if(!Clearing||!Tree||!Fern||!FFileHelper::LoadFileToString(Data,*(FPaths::ProjectContentDir()/TEXT("GameData/forest-limiar-layout.json")))||!FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Data),Layout)||!Layout.IsValid())return false;
 // Blender FBX imports with Y reflected. A positive 180-degree scene rotation keeps winding correct; placements use the matching frame.
 for(auto Mesh:{Clearing,Tree,Fern})for(int I=0;I<Mesh->GetStaticMaterials().Num();I++){auto Material=Mesh->GetMaterial(I);if(!Material||!Material->GetPathName().StartsWith(Folder))return false;}
 ForestSet->SetRelativeRotation(FRotator(0,180,0));ForestSet->SetStaticMesh(Clearing);ForestTrees->SetStaticMesh(Tree);ForestFerns->SetStaticMesh(Fern);
 auto Populate=[&](const TCHAR* Key,UHierarchicalInstancedStaticMeshComponent* Component){
  const TArray<TSharedPtr<FJsonValue>>* Rows=nullptr;if(!Layout->TryGetArrayField(Key,Rows))return 0;
  for(const auto& Row:*Rows){const auto Entry=Row->AsObject();if(!Entry.IsValid())continue;const auto& P=Entry->GetArrayField(TEXT("p"));const auto& S=Entry->GetArrayField(TEXT("s"));if(P.Num()!=3||S.Num()!=3)continue;
   Component->AddInstance(FTransform(FRotator(0,180-Entry->GetNumberField(TEXT("yaw")),0),FVector(-P[0]->AsNumber(),P[1]->AsNumber(),P[2]->AsNumber()),FVector(S[0]->AsNumber(),S[1]->AsNumber(),S[2]->AsNumber())));
  }return Component->GetInstanceCount();
 };
 ForestTreeCount=Populate(TEXT("trees"),ForestTrees);ForestFernCount=Populate(TEXT("ferns"),ForestFerns);
 if(ForestTreeCount<40||ForestFernCount<200)return false;
 // Shared mesh assets and hierarchical instances keep the repeated canopy out of the procedural mesh batches.
 for(UStaticMeshComponent* Component:{ForestSet.Get(),static_cast<UStaticMeshComponent*>(ForestTrees.Get()),static_cast<UStaticMeshComponent*>(ForestFerns.Get())}){
  Component->SetVisibility(true);Component->SetCastShadow(true);
  for(int Index=0;Index<Component->GetNumMaterials();Index++){auto Base=Component->GetStaticMesh()->GetMaterial(Index);if(!Base)continue;auto Dynamic=UMaterialInstanceDynamic::Create(Base,this);Dynamic->SetScalarParameterValue(TEXT("SceneTime"),0);Component->SetMaterial(Index,Dynamic);Materials.Add(Dynamic);}
 }
 TriangleCount=Clearing->GetNumTriangles(0)+Tree->GetNumTriangles(0)+Fern->GetNumTriangles(0);
 AmbientLight->SetIntensity(.65f);Atmosphere->SetFogInscatteringColor(FLinearColor(.045f,.075f,.08f));Atmosphere->SetFogDensity(.029f);Atmosphere->SetFogHeightFalloff(.055f);Atmosphere->SetStartDistance(350);Atmosphere->SetVolumetricFog(true);Atmosphere->SetVolumetricFogDistance(9000);
 Lens->Settings.bOverride_ColorContrast=true;Lens->Settings.ColorContrast=FVector4(1.05f,1.04f,1.03f,1);Lens->Settings.ColorSaturation=FVector4(.90f,.92f,.91f,1);
 Materials[4]->SetVectorParameterValue(TEXT("Color"),FLinearColor(.04f,.45f,.25f));
 for(int Side:{-1,1}){auto Lamp=NewObject<UPointLightComponent>(this);AddInstanceComponent(Lamp);Lamp->SetupAttachment(RootComponent);Lamp->SetRelativeLocation(FVector(Side*260,-2120,480));Lamp->SetLightColor(FLinearColor(.07f,.8f,.42f));Lamp->SetIntensity(16000);Lamp->SetAttenuationRadius(900);Lamp->SetCastShadows(false);Lamp->RegisterComponent();Lamps.Add(Lamp);}
 UE_LOG(LogTemp,Display,TEXT("BLENDER_FOREST_READY unique_triangles=%d trees=%d ferns=%d"),TriangleCount,ForestTreeCount,ForestFernCount);
 return true;
}
