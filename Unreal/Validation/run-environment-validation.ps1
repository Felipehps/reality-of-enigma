param([string]$GameExecutable='')
$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
$gameExe=Join-Path $taskRoot 'outputs/RealityOfEnigma-Unreal-Cenarios/Windows/RealityOfEnigmaUE/Binaries/Win64/RealityOfEnigmaUE.exe'
if($GameExecutable){$gameExe=$GameExecutable}
New-Item -ItemType Directory -Force -Path (Join-Path $taskRoot 'outputs'),(Join-Path $taskRoot 'work') | Out-Null
$results=@()
foreach($entry in @(@{Name='1280x720';Width=1280;Height=720},@{Name='2560x1080';Width=2560;Height=1080},@{Name='3440x1440';Width=3440;Height=1440},@{Name='1024x768';Width=1024;Height=768})){
 $folder=Join-Path $taskRoot ('outputs/Mouse-Windows-'+$entry.Name)
 New-Item -ItemType Directory -Force -Path $folder | Out-Null
 $gameLog=Join-Path $taskRoot ('work/mouse-windows-'+$entry.Name+'.log')
 $gameArgs=@(('-EnigmaMouseTour="'+$folder+'"'),'-RenderOffscreen','-windowed','-ForceRes','-unattended','-NoVSync',('-ResX='+$entry.Width),('-ResY='+$entry.Height),('-abslog="'+$gameLog+'"'))
 $job=Start-Process -FilePath $gameExe -ArgumentList $gameArgs -WindowStyle Hidden -PassThru
 $job.WaitForExit()
 $match=[regex]::Match((Get-Content -Raw -LiteralPath $gameLog),'ENIGMA_MOUSE_QA_RESULT checks=(\d+) failures=(\d+)')
 if($job.ExitCode -ne 0 -or !$match.Success -or $match.Groups[2].Value -ne '0'){throw ('Falha no mouse: '+$entry.Name)}
 $results+=@{resolution=$entry.Name;checks=[int]$match.Groups[1].Value;failures=0}
 Write-Output ('MOUSE_WINDOWS_OK '+$entry.Name+' checks='+$match.Groups[1].Value)
}
$scenery=@()
foreach($entry in @(@{Name='1280x720';Width=1280;Height=720},@{Name='2560x1080';Width=2560;Height=1080})){
 $folder=Join-Path $taskRoot ('outputs/Scenery-Windows-'+$entry.Name)
 New-Item -ItemType Directory -Force -Path $folder | Out-Null
 $gameLog=Join-Path $taskRoot ('work/scenery-windows-'+$entry.Name+'.log')
 $gameArgs=@(('-EnigmaSceneryTour="'+$folder+'"'),'-RenderOffscreen','-windowed','-ForceRes','-unattended','-NoVSync',('-ResX='+$entry.Width),('-ResY='+$entry.Height),('-abslog="'+$gameLog+'"'))
 $job=Start-Process -FilePath $gameExe -ArgumentList $gameArgs -WindowStyle Hidden -PassThru
 $job.WaitForExit()
 $match=[regex]::Match((Get-Content -Raw -LiteralPath $gameLog),'ENIGMA_SCENERY_QA_RESULT checks=(\d+) failures=(\d+)')
 if($job.ExitCode -ne 0 -or !$match.Success -or $match.Groups[2].Value -ne '0'){throw ('Falha nos cenarios: '+$entry.Name)}
 $captures=@(Get-ChildItem -LiteralPath $folder -Filter '*.png')
 if($captures.Count -ne 21){throw 'Capturas de cenarios incompletas.'}
 $scenery+=@{resolution=$entry.Name;checks=[int]$match.Groups[1].Value;failures=0;captures=21}
 Write-Output ('SCENERY_WINDOWS_OK '+$entry.Name+' checks='+$match.Groups[1].Value)
}
$coreLog=Join-Path $taskRoot 'work/scenery-windows-core.log'
$job=Start-Process -FilePath $gameExe -ArgumentList @('-EnigmaMigrationQA','-RenderOffscreen','-unattended',('-abslog="'+$coreLog+'"')) -WindowStyle Hidden -PassThru
$job.WaitForExit()
$match=[regex]::Match((Get-Content -Raw -LiteralPath $coreLog),'ENIGMA_MIGRATION_QA_RESULT checks=(\d+) failures=(\d+)')
if($job.ExitCode -ne 0 -or !$match.Success -or $match.Groups[2].Value -ne '0'){throw 'Falha na integracao do jogo.'}
@{date='2026-10-17';scenery=$scenery;mouse=$results;core_checks=[int]$match.Groups[1].Value;core_failures=0;visual_review_pending=$true}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $taskRoot 'outputs/Cenarios-validacao.json') -Encoding utf8
Write-Output ('SCENERY_CORE_OK checks='+$match.Groups[1].Value)
