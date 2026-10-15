$ErrorActionPreference = 'Stop'
$taskRoot = Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path (Join-Path $taskRoot 'Validation') | Out-Null
$gameExe = Join-Path $taskRoot 'RealityOfEnigma-Unreal/Windows/RealityOfEnigmaUE/Binaries/Win64/RealityOfEnigmaUE.exe'
$fontFile = Join-Path $taskRoot 'RealityOfEnigma-Unreal/Windows/RealityOfEnigmaUE/Content/UI/Fonts/Cinzel.ttf'
if (!(Test-Path -LiteralPath $gameExe) -or !(Test-Path -LiteralPath $fontFile)) { throw 'Executavel ou fonte nao foram empacotados.' }
$checks = @(
    @{ Name='1280x720'; Width=1280; Height=720 },
    @{ Name='2560x1080'; Width=2560; Height=1080 }
)
$results = @()
foreach ($entry in $checks) {
    $captureFolder = Join-Path $taskRoot ('Menus-Windows-' + $entry.Name)
    New-Item -ItemType Directory -Force -Path $captureFolder | Out-Null
    $gameLog = Join-Path $taskRoot ('Validation/menus-windows-' + $entry.Name + '.log')
    $args = @(
        ('-EnigmaHomeTour="' + $captureFolder + '"'),
        '-RenderOffscreen', '-windowed', '-ForceRes', '-unattended', '-NoVSync',
        ('-ResX=' + $entry.Width), ('-ResY=' + $entry.Height),
        ('-abslog="' + $gameLog + '"')
    )
    $job = Start-Process -FilePath $gameExe -ArgumentList $args -WindowStyle Hidden -PassThru
    $job.WaitForExit()
    $text = Get-Content -Raw -LiteralPath $gameLog
    $match = [regex]::Match($text, 'ENIGMA_HOME_QA_RESULT checks=(\d+) failures=(\d+)')
    if ($job.ExitCode -ne 0 -or !$match.Success -or $match.Groups[2].Value -ne '0') { throw ('Falha na validacao da tela inicial: ' + $entry.Name) }
    $captures = @(Get-ChildItem -LiteralPath $captureFolder -Filter '*.png')
    if ($captures.Count -ne 22) { throw ('Capturas incompletas: ' + $entry.Name) }
    $results += @{ resolution=$entry.Name; checks=[int]$match.Groups[1].Value; failures=0; captures=$captures.Count; exit_code=$job.ExitCode }
    Write-Output ('TITLE_WINDOWS_OK ' + $entry.Name + ' checks=' + $match.Groups[1].Value)
}
$coreLog = Join-Path $taskRoot 'Validation/menus-windows-core.log'
$job = Start-Process -FilePath $gameExe -ArgumentList @('-EnigmaMigrationQA','-RenderOffscreen','-unattended',('-abslog="'+$coreLog+'"')) -WindowStyle Hidden -PassThru
$job.WaitForExit()
$text = Get-Content -Raw -LiteralPath $coreLog
$match = [regex]::Match($text,'ENIGMA_MIGRATION_QA_RESULT checks=(\d+) failures=(\d+)')
if ($job.ExitCode -ne 0 -or !$match.Success -or $match.Groups[2].Value -ne '0') { throw 'Falha nos testes de integracao do jogo.' }
$record = @{ date='2026-10-15'; home=$results; core_checks=[int]$match.Groups[1].Value; core_failures=0; fonts_staged=$true; visual_review_pending=$true }
$record | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $taskRoot 'Menus-validacao.json') -Encoding utf8
Write-Output ('TITLE_CORE_OK checks=' + $match.Groups[1].Value)
