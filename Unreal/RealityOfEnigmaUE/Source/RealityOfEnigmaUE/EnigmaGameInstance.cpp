#include "EnigmaGameInstance.h"
#include "HAL/PlatformProcess.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
void UEnigmaGameInstance::Init(){
 Super::Init();
 const FString Dll=FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("Runtime/Win64/Enigma.Native.dll"));
 Library=FPlatformProcess::GetDllHandle(*Dll);
 if(!Library){Error=TEXT("Biblioteca das regras não encontrada: ")+Dll;return;}
 auto Open=reinterpret_cast<FOpen>(FPlatformProcess::GetDllExport(Library,TEXT("EnigmaOpen")));
 Close=reinterpret_cast<FClose>(FPlatformProcess::GetDllExport(Library,TEXT("EnigmaClose")));
 Run=reinterpret_cast<FExecute>(FPlatformProcess::GetDllExport(Library,TEXT("EnigmaExecute")));
 Free=reinterpret_cast<FFree>(FPlatformProcess::GetDllExport(Library,TEXT("EnigmaFree")));
 if(!Open||!Close||!Run||!Free){Error=TEXT("Biblioteca incompatível");return;}
 Session=Open();
 auto Request=MakeShared<FJsonObject>();Request->SetStringField(TEXT("command"),TEXT("init"));
 Request->SetStringField(TEXT("data"),FPaths::ConvertRelativePathToFull(FPaths::ProjectContentDir()/TEXT("GameData")));
 FString Save=FPaths::ProjectSavedDir()/TEXT("Campaign/save-v1.json");
 if(FParse::Param(FCommandLine::Get(),TEXT("EnigmaMigrationQA"))||FCString::Strstr(FCommandLine::Get(),TEXT("EnigmaUITour=")))Save=FPaths::ProjectSavedDir()/TEXT("MigrationQA")/(FGuid::NewGuid().ToString()+TEXT(".json"));
 Request->SetStringField(TEXT("save"),FPaths::ConvertRelativePathToFull(Save));Execute(Request);
}
void UEnigmaGameInstance::Shutdown(){
 if(Session&&Close){Close(Session);Session=nullptr;}
 // NativeAOT owns runtime threads; keep the library loaded for the process lifetime.
 Library=nullptr;
 Super::Shutdown();
}
bool UEnigmaGameInstance::Execute(const TSharedPtr<FJsonObject>& Request){
 if(!Run||!Session)return false;
 FString Input;auto Writer=TJsonWriterFactory<>::Create(&Input);FJsonSerializer::Serialize(Request.ToSharedRef(),Writer);
 const FTCHARToUTF8 Bytes(*Input);char* Output=Run(Session,Bytes.Get());
 if(!Output){Error=TEXT("Sem resposta das regras");return false;}
 const FString Text=UTF8_TO_TCHAR(Output);Free(Output);
 TSharedPtr<FJsonObject> Response;auto Reader=TJsonReaderFactory<>::Create(Text);
 if(!FJsonSerializer::Deserialize(Reader,Response)||!Response.IsValid()){Error=TEXT("Resposta inválida");return false;}
 bool Ok=false;Response->TryGetBoolField(TEXT("ok"),Ok);
 if(!Ok){Response->TryGetStringField(TEXT("error"),Error);return false;}
 Error.Reset();State=Response;return true;
}
bool UEnigmaGameInstance::Command(const FString& Name,const FString& Id,int32 Number){
 auto Request=MakeShared<FJsonObject>();Request->SetStringField(TEXT("command"),Name);Request->SetStringField(TEXT("id"),Id);Request->SetNumberField(TEXT("number"),Number);return Execute(Request);
}
