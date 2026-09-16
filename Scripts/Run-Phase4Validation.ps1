param([string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8')

# Launch one rendered acceptance process and require both its scenario marker and clean Quit.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'PinballBattle.uproject'
$logPath = Join-Path $repo 'Saved/Logs/Phase4Session.log'
$arguments = @(
    ('"{0}"' -f $project), '/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet',
    '-game', '-windowed', '-RenderOffscreen', '-ResX=1280', '-ResY=720',
    '-unattended', '-nop4', '-nosplash', '-PinballSessionProbe',
    '"-ExecCmds=t.MaxFPS 60,r.VSync 0"', ('"-abslog={0}"' -f $logPath)
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit(480000)) { throw 'Phase 4 validation timed out.' }
    $result = Select-String -LiteralPath $logPath -Pattern 'PINBALL_SESSION_(PASS|FAIL)' | Select-Object -Last 1
    if (-not $result -or $result.Line -notmatch 'PINBALL_SESSION_PASS' -or $process.ExitCode -ne 0) {
        throw "Phase 4 acceptance failed; see $logPath"
    }
    if (-not (Select-String -LiteralPath $logPath -SimpleMatch 'LogExit: Exiting.')) {
        throw "No clean Quit recorded; see $logPath"
    }
    Write-Output $result.Line
}
finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id }
}
