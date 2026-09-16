param([string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8', [switch]$RecoveryOnly)

# Run the rendered shipping-flow fixture with a bounded deadline and require its explicit pass marker.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'PinballBattle.uproject'
$logPath = Join-Path $repo 'Saved/Logs/Phase5Acceptance.log'
$arguments = @(
    ('"{0}"' -f $project), '/Game/Tests/Maps/L_TransitionTest', '-game', '-windowed', '-RenderOffscreen',
    '-ResX=1280', '-ResY=720', '-unattended', '-nop4', '-nosplash', '-PinballTransitionProbe',
    '"-ExecCmds=t.MaxFPS 60,r.VSync 0"', ('"-abslog={0}"' -f $logPath)
)
if ($RecoveryOnly) { $arguments += '-PinballTransitionRecoveryOnly' }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit(300000)) { throw "Phase 5 validation exceeded 300 seconds; see $logPath" }
    $result = Select-String -LiteralPath $logPath -Pattern 'PINBALL_TRANSITION_(PASS|FAIL)' | Select-Object -Last 1
    if (-not $result -or $result.Line -notmatch 'PINBALL_TRANSITION_PASS' -or $process.ExitCode -ne 0) {
        throw "Phase 5 acceptance failed; see $logPath"
    }
    if (-not (Select-String -LiteralPath $logPath -SimpleMatch 'LogExit: Exiting.')) { throw 'Missing clean exit.' }
    Write-Output $result.Line
}
finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id }
}
