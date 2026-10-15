@echo off
cd /d "%~dp0"
start "" "RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE.exe" -EnigmaAkurothTest -windowed -ResX=1280 -ResY=720 -ExecCmds="t.MaxFPS 30"
