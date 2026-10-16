$ErrorActionPreference='Stop'
$taskRoot=Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path (Join-Path $taskRoot 'Validation') | Out-Null
$gameExe=Join-Path $taskRoot 'RealityOfEnigma-Unreal/Windows/RealityOfEnigmaUE/Binaries/Win64/RealityOfEnigmaUE.exe'
$updatedExe=Join-Path $taskRoot 'RealityOfEnigma-Unreal-Atualizado/Windows/RealityOfEnigmaUE/Binaries/Win64/RealityOfEnigmaUE.exe'
if(Test-Path -LiteralPath $updatedExe){$gameExe=$updatedExe}
$results=@()
foreach($entry in @(@{Name='1280x720';Width=1280;Height=720},@{Name='2560x1080';Width=2560;Height=1080})){
 $folder=Join-Path $taskRoot ('StoryMap-Windows-'+$entry.Name)
 New-Item -ItemType Directory -Force -Path $folder | Out-Null
 $gameLog=Join-Path $taskRoot ('Validation/story-windows-'+$entry.Name+'.log')
 $gameArgs=@(('-EnigmaChronicleTour="'+$folder+'"'),'-RenderOffscreen','-windowed','-ForceRes','-unattended','-NoVSync',('-ResX='+$entry.Width),('-ResY='+$entry.Height),('-abslog="'+$gameLog+'"'))
 $job=Start-Process -FilePath $gameExe -ArgumentList $gameArgs -WindowStyle Hidden -PassThru
 $job.WaitForExit()
 $text=Get-Content -Raw -LiteralPath $gameLog
 $match=[regex]::Match($text,'ENIGMA_CHRONICLE_QA_RESULT checks=(\d+) failures=(\d+)')
 if($job.ExitCode -ne 0 -or !$match.Success -or $match.Groups[2].Value -ne '0'){throw ('Falha nas rotas: '+$entry.Name)}
 $captures=@(Get-ChildItem -LiteralPath $folder -Filter '*.png')
 if($captures.Count -ne 9){throw 'Capturas das rotas incompletas.'}
 $results+=@{resolution=$entry.Name;checks=[int]$match.Groups[1].Value;failures=0;captures=9}
 Write-Output ('STORY_WINDOWS_OK '+$entry.Name+' checks='+$match.Groups[1].Value)
}
$coreLog=Join-Path $taskRoot 'Validation/story-windows-core.log'
$job=Start-Process -FilePath $gameExe -ArgumentList @('-EnigmaMigrationQA','-RenderOffscreen','-unattended',('-abslog="'+$coreLog+'"')) -WindowStyle Hidden -PassThru
$job.WaitForExit()
$match=[regex]::Match((Get-Content -Raw -LiteralPath $coreLog),'ENIGMA_MIGRATION_QA_RESULT checks=(\d+) failures=(\d+)')
if($job.ExitCode -ne 0 -or !$match.Success -or $match.Groups[2].Value -ne '0'){throw 'Falha na integracao do jogo.'}
@{date='2026-10-16';maps=$results;core_checks=[int]$match.Groups[1].Value;core_failures=0;visual_review_pending=$true}|ConvertTo-Json -Depth 6|Set-Content -LiteralPath (Join-Path $taskRoot 'Mapas-validacao.json') -Encoding utf8
Write-Output ('STORY_CORE_OK checks='+$match.Groups[1].Value)
