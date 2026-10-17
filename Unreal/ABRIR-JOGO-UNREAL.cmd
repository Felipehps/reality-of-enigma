@echo off
setlocal
if /I "%~1"=="--editor" goto editor
if exist "%~dp0RealityOfEnigma-Unreal-Arte\Windows\RealityOfEnigmaUE.exe" (call "%~dp0VER-TELA-INICIAL.cmd" & exit /b)
if exist "%~dp0RealityOfEnigma-Unreal-Cenarios\Windows\RealityOfEnigmaUE.exe" (call "%~dp0VER-TELA-INICIAL.cmd" & exit /b)
if exist "%~dp0RealityOfEnigma-Unreal-Atualizado\Windows\RealityOfEnigmaUE.exe" (call "%~dp0VER-TELA-INICIAL.cmd" & exit /b)
set "GAME=%~dp0RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE.exe"
if exist "%GAME%" (
 start "Reality of Enigma" "%GAME%" -windowed -ResX=1280 -ResY=720
 exit /b 0
)
:editor
set "ENGINE=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%~dp0RealityOfEnigmaUE\RealityOfEnigmaUE.uproject"
if not exist "%ENGINE%" (echo Unreal 5.8 nao encontrada. & pause & exit /b 1)
if not exist "%~dp0RealityOfEnigmaUE\Binaries\Win64\UnrealEditor-RealityOfEnigmaUE.dll" (echo Compile o projeto antes de abrir. & pause & exit /b 1)
start "Reality of Enigma - Unreal" "%ENGINE%" "%PROJECT%" -game -windowed -ResX=1280 -ResY=720 -nosplash
