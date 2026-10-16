#include "EnigmaCampaignMode.h"
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"

// Shared menu composition. Content and commands remain owned by the campaign HUD.
void AEnigmaCampaignHUD::DrawMenuFrame(const FString& Page){
 const FLinearColor Gold(.83f,.62f,.32f),Muted(.56f,.66f,.68f);
 DrawRect(FLinearColor(.006f,.014f,.022f),0,0,Canvas->ClipX,Canvas->ClipY);
 const bool Region=Page==TEXT("campaign")||Page==TEXT("ending");
 auto Back=Texture(Region?FString::Printf(TEXT("Region_%d"),Mode->Region+1):TEXT("TitleLimiar"));
 if(Back){
  const float Fit=FMath::Max(Canvas->ClipX/Back->GetSizeX(),Canvas->ClipY/Back->GetSizeY());
  const float Drift=Mode->ReducedMotion?0:2*FMath::Sin(Mode->Clock*.1f);
  const float W=Back->GetSizeX()*Fit*1.02f,H=Back->GetSizeY()*Fit*1.02f;
  DrawTexture(Back,(Canvas->ClipX-W)*.5f+Drift,(Canvas->ClipY-H)*.5f,W,H,0,0,1,1,FLinearColor(.58f,.66f,.67f));
 }
 DrawRect(FLinearColor(.006f,.015f,.023f,.74f),0,0,Canvas->ClipX,Canvas->ClipY);
 // An inset frame anchors the interface even on ultrawide displays.
 Box(24,15,1232,685,FLinearColor(.008f,.018f,.026f,.5f));
 Box(40,59,1200,1,FLinearColor(.83f,.62f,.32f,.28f));
 Box(40,59,55,2,Gold);
 Box(40,642,1200,1,FLinearColor(.42f,.54f,.56f,.23f));
 const TCHAR* Section=Page==TEXT("campaign")?TEXT("ATLAS DOS MUNDOS"):Page==TEXT("ending")?TEXT("CRÔNICAS DA JORNADA"):Page==TEXT("team")?TEXT("OS QUE ATRAVESSAM O LIMIAR"):Page==TEXT("cards")?TEXT("ARQUIVO DAS REALIDADES"):Page==TEXT("formation")?TEXT("PREPARAÇÃO DE COMBATE"):Page==TEXT("summon")?TEXT("NOVOS DESTINOS"):Page==TEXT("training")?TEXT("CAMINHO DO APERFEIÇOAMENTO"):Page==TEXT("arenas")?TEXT("DESAFIOS DO LIMIAR"):Page==TEXT("settings")?TEXT("SUA EXPERIÊNCIA"):Page==TEXT("reference")?TEXT("GALERIA DO ELENCO"):TEXT("O INÍCIO DA RUPTURA");
 Text(Page==TEXT("routes")||Page==TEXT("routestory")||Page==TEXT("routeoutcome")?TEXT("CRÔNICAS DA RUPTURA"):Section,45,75,Muted,.72f);
 Text(TEXT("PROGRESSO SALVO AUTOMATICAMENTE"),45,685,FLinearColor(.47f,.59f,.61f),.65f);
 Text(TEXT("ESC  Início     TAB / DIRECIONAIS  Navegar     ENTER  Selecionar"),755,685,FLinearColor(.47f,.59f,.61f),.65f);
 auto Panel=[&](float X,float Y,float W,float H){Box(X,Y,W,H,FLinearColor(.014f,.031f,.041f,.88f));Box(X,Y,W,1,FLinearColor(.83f,.62f,.32f,.2f));};
 if(Page==TEXT("team")||Page==TEXT("cards")){
  Panel(32,151,336,452);Panel(381,151,854,452);
  if(Page==TEXT("team"))Text(TEXT("EVOLUÇÃO  /  EQUIPE  /  FORMAÇÃO"),395,610,Muted,.72f);
 }
 else if(Page==TEXT("formation")){
  Panel(32,164,346,365);Panel(408,164,685,343);
  Text(TEXT("VANGUARDA"),1103,212,Gold,.68f);Text(TEXT("CENTRO"),1103,317,Muted,.68f);Text(TEXT("RETAGUARDA"),1103,422,Muted,.68f);
 }
 else if(Page==TEXT("training")){
  Panel(32,164,790,320);Panel(854,164,367,320);
  Text(TEXT("DOMINE O ENCONTRO"),880,193,Gold,1.2f);
  Wrap(TEXT("Enfrente três ondas consecutivas. Preserve sua equipe, experimente formações e conquiste fragmentos para recrutar novos aliados."),880,240,36,7);
  Text(TEXT("Libere desafios avançando na campanha."),880,435,Muted,.72f);
 }
 else if(Page==TEXT("summon")){
  Panel(32,181,460,286);Panel(518,181,706,286);
  Text(TEXT("ATRAVESSE. ENCONTRE. EVOLUA."),540,405,Gold,1.1f);
  Text(TEXT("Cada encontro abre uma nova possibilidade."),45,503,Muted,.95f);
 }
 else if(Page==TEXT("settings")){
  Panel(32,174,530,329);Panel(590,174,634,329);
  Text(TEXT("IMAGEM E CONFORTO"),618,206,Gold,1.35f);
  Wrap(TEXT("Escolha como explorar as realidades. O movimento reduzido suaviza a experiência visual, desativando os movimentos decorativos dos menus."),618,255,61,5);
  Text(TEXT("As preferências são salvas automaticamente."),618,441,Muted,.85f);
  Box(45,237,425,3,FLinearColor(.18f,.27f,.29f));Box(45,237,425*Mode->Volume,3,Gold);
 }
 else if(Page==TEXT("arenas")){
  Text(TEXT("Arenas do limiar"),45,110,Gold,1.6f);
  Text(TEXT("Escolha um encontro. Prepare sua equipe. Enfrente o impossível."),45,605,Muted,.95f);
 }
 else if(Page==TEXT("campaign")){
  Text(TEXT("ESCOLHA SUA PRÓXIMA TRAVESSIA"),45,229,Muted,.75f);
  // Route lines join the stage cards, with a visible break between the two rows.
  for(int32 I=0;I<4;I++)for(int32 Row=0;Row<2;Row++)Box(265+I*235,292+Row*95,15,1,FLinearColor(.83f,.62f,.32f,.35f));
 }
 if(!Mode->ReducedMotion)for(int32 I=0;I<12;I++){
  const float X=40+FMath::Fmod(I*107.f+FMath::Sin(Mode->Clock*.13f+I)*4,1190.f);
  const float Y=630-FMath::Fmod(I*51.f+Mode->Clock*3,465.f);
  Box(X,Y,1,1,FLinearColor(.93f,.7f,.37f,.13f));
 }
}
