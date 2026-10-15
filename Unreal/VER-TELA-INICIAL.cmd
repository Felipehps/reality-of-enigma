@echo off
setlocal
set "GAME=%~dp0RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE.exe"
if not exist "%GAME%" (echo O jogo ainda nao foi empacotado nesta pasta. & pause & exit /b 1)
start "Reality of Enigma" "%GAME%" -windowed -ResX=1600 -ResY=900
