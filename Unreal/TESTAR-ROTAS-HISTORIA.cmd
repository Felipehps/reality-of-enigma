@echo off
setlocal
set "GAME=%~dp0RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE.exe"
if exist "%~dp0RealityOfEnigma-Unreal-Atualizado\Windows\RealityOfEnigmaUE.exe" set "GAME=%~dp0RealityOfEnigma-Unreal-Atualizado\Windows\RealityOfEnigmaUE.exe"
if exist "%~dp0RealityOfEnigma-Unreal-Cenarios\Windows\RealityOfEnigmaUE.exe" set "GAME=%~dp0RealityOfEnigma-Unreal-Cenarios\Windows\RealityOfEnigmaUE.exe"
if not exist "%GAME%" (echo Jogo nao encontrado nesta pasta. & pause & exit /b 1)
start "Reality of Enigma - Previa isolada das rotas" "%GAME%" -EnigmaChroniclePreview -windowed -ResX=1600 -ResY=900
