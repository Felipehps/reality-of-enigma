@echo off
setlocal
set "GAME=%~dp0RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE.exe"
set "OLD_SAVE=%~dp0RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE\Saved"
set "NEW_SAVE=%~dp0RealityOfEnigma-Unreal-Atualizado\Windows\RealityOfEnigmaUE\Saved"
if exist "%~dp0RealityOfEnigma-Unreal-Atualizado\Windows\RealityOfEnigmaUE.exe" (
 set "GAME=%~dp0RealityOfEnigma-Unreal-Atualizado\Windows\RealityOfEnigmaUE.exe"
 if not exist "%NEW_SAVE%" mkdir "%NEW_SAVE%"
 if not exist "%NEW_SAVE%\Campaign\save-v1.json" if exist "%OLD_SAVE%\Campaign\save-v1.json" (
  if not exist "%NEW_SAVE%\Campaign" mkdir "%NEW_SAVE%\Campaign"
  copy /Y "%OLD_SAVE%\Campaign\save-v1.json" "%NEW_SAVE%\Campaign\save-v1.json" >nul
  if errorlevel 1 (echo Nao foi possivel copiar sua campanha para a atualizacao. & pause & exit /b 1)
 )
 if not exist "%NEW_SAVE%\EnigmaPreferences.ini" if exist "%OLD_SAVE%\EnigmaPreferences.ini" copy /Y "%OLD_SAVE%\EnigmaPreferences.ini" "%NEW_SAVE%\EnigmaPreferences.ini" >nul
)
if not exist "%GAME%" (echo O jogo ainda nao foi empacotado nesta pasta. & pause & exit /b 1)
start "Reality of Enigma" "%GAME%" -windowed -ResX=1600 -ResY=900
