param([Parameter(Mandatory=$true)][string]$SaveOriginal)
$ErrorActionPreference='Stop'
$sourceSave=(Resolve-Path -LiteralPath $SaveOriginal).Path
$gameRoot=Join-Path $PSScriptRoot 'RealityOfEnigmaUE'
$dest=Join-Path $gameRoot 'Saved/Campaign/save-v1.json'
if(Test-Path -LiteralPath $dest){throw 'Ja existe uma jornada Unreal. Preserve-a antes de importar outro save.'}
$saveText=Get-Content -LiteralPath $sourceSave -Raw
$saveData=$saveText|ConvertFrom-Json
if($saveData.Version -ne 1 -or $saveData.Inventory.Gold -lt 0){throw 'Save incompatível.'}
New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
Copy-Item -LiteralPath $sourceSave -Destination $dest
Write-Output 'Save copiado. A Unreal validará níveis e estrelas ao iniciar; o original foi preservado.'
