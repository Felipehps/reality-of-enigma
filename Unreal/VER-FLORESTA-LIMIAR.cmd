@echo off
setlocal
set "GAME=%~dp0RealityOfEnigma-Unreal-Floresta\Windows\RealityOfEnigmaUE.exe"
if not exist "%GAME%" set "GAME=%~dp0RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE.exe"
if not exist "%GAME%" (echo A versao da floresta ainda nao foi encontrada nesta pasta. & pause & exit /b 1)
start "Floresta do Limiar - Reality of Enigma" "%GAME%" -EnigmaSceneryPreview -EnigmaForestPreview -windowed -ResX=1600 -ResY=900
