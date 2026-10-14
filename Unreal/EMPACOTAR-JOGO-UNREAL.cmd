@echo off
setlocal
set "ENIGMA_ENGINE=C:\Program Files\Epic Games\UE_5.8"
set "ENIGMA_PROJECT=%~dp0RealityOfEnigmaUE\RealityOfEnigmaUE.uproject"
dotnet publish "%~dp0RealityOfEnigmaUE\NativeCore\Enigma.Native.csproj" -c Release -r win-x64 -o "%~dp0RealityOfEnigmaUE\Content\Runtime\Win64"
if errorlevel 1 (echo Falha ao compilar as regras. & pause & exit /b 1)
call "%ENIGMA_ENGINE%\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun -project="%ENIGMA_PROJECT%" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage -pak -archive -archivedirectory="%~dp0RealityOfEnigma-Unreal" -ubtargs=-WaitMutex
if errorlevel 1 (echo Falha ao gerar o jogo Windows. & pause & exit /b 1)
echo Versao Windows gerada em RealityOfEnigma-Unreal\Windows.
pause
