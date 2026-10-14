@echo off
setlocal
set "ENIGMA_ENGINE=C:\Program Files\Epic Games\UE_5.8"
set "ENIGMA_PROJECT=%~dp0RealityOfEnigmaUE\RealityOfEnigmaUE.uproject"
dotnet publish "%~dp0RealityOfEnigmaUE\NativeCore\Enigma.Native.csproj" -c Release -r win-x64 -o "%~dp0RealityOfEnigmaUE\Content\Runtime\Win64"
if errorlevel 1 (echo Falha ao compilar as regras. & pause & exit /b 1)
call "%ENIGMA_ENGINE%\Engine\Build\BatchFiles\Build.bat" RealityOfEnigmaUEEditor Win64 Development -Project="%ENIGMA_PROJECT%" -WaitMutex -NoHotReloadFromIDE
if errorlevel 1 (echo Falha ao compilar o projeto Unreal. & pause & exit /b 1)
call "%~dp0ABRIR-JOGO-UNREAL.cmd" --editor
