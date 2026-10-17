@echo off
setlocal
set "GAME=%~dp0RealityOfEnigma-Unreal-Floresta\Windows\RealityOfEnigmaUE.exe"
if not exist "%GAME%" set "GAME=%~dp0RealityOfEnigma-Unreal-Arte\Windows\RealityOfEnigmaUE.exe"
if not exist "%GAME%" set "GAME=%~dp0RealityOfEnigma-Unreal-Cenarios\Windows\RealityOfEnigmaUE.exe"
if not exist "%GAME%" set "GAME=%~dp0RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE.exe"
if not exist "%GAME%" (echo O jogo ainda nao foi empacotado nesta pasta. & pause & exit /b 1)
start "Cenarios da historia" "%GAME%" -EnigmaSceneryPreview -windowed -ResX=1600 -ResY=900
