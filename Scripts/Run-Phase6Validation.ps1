param([string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8')

# Run two rendered production-flow round trips and require the explicit acceptance marker.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'PinballBattle.uproject'
$logPath = Join-Path $repo 'Saved/Logs/Phase6Acceptance.log'
$arguments = @(
    ('"{0}"' -f $project), '/Game/Tests/Maps/L_AsteroidTest', '-game', '-windowed', '-RenderOffscreen',
    '-ResX=1280', '-ResY=720', '-unattended', '-nop4', '-nosplash', '-AsteroidProbe',
    '"-ExecCmds=t.MaxFPS 60,r.VSync 0"', ('"-abslog={0}"' -f $logPath)
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit(150000)) { throw "Phase 6 exceeded its deadline; see $logPath" }
    $result = Select-String -LiteralPath $logPath -Pattern 'PINBALL_ASTEROID_(PASS|FAIL)' | Select-Object -Last 1
    if (-not $result -or $result.Line -notmatch 'PINBALL_ASTEROID_PASS' -or $process.ExitCode -ne 0) {
        throw "Phase 6 acceptance failed; see $logPath"
    }
    if (-not (Select-String -LiteralPath $logPath -SimpleMatch 'LogExit: Exiting.')) { throw 'Missing clean exit.' }
    Write-Output $result.Line
}
finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id }
}
