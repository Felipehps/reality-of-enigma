@echo off
cd /d "%~dp0"
start "" "RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE.exe" "-EnigmaCombatReplay=%~dp0RealityOfEnigma-Unreal\Windows\RealityOfEnigmaUE\Content\GameData\TripoTest" -EnigmaKeepReplayOpen -windowed -ResX=1280 -ResY=720 -ExecCmds="t.MaxFPS 30"
