@echo off
setlocal
set "ENIGMA_ENGINE=C:\Program Files\Epic Games\UE_5.8"
set "ENIGMA_PROJECT=%~dp0RealityOfEnigmaUE\RealityOfEnigmaUE.uproject"
dotnet run --project "%~dp0RealityOfEnigmaUE\NativeCore\Tests\Enigma.Native.Tests.csproj" -c Release
if errorlevel 1 (pause & exit /b 1)
"%ENIGMA_ENGINE%\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "%ENIGMA_PROJECT%" -game -NullRHI -unattended -nosound -nosplash -EnigmaMigrationQA
if errorlevel 1 (pause & exit /b 1)
echo Testes concluidos.
pause
