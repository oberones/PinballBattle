param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [ValidateRange(2, 100)][int]$CyclesPerRate = 7
)

$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'PinballBattle.uproject'
$runs = @()
foreach ($rate in @(30, 60, 120)) {
    $logPath = Join-Path $repo "Saved/Logs/Phase2Practice$rate.log"
    $arguments = @(
        ('"{0}"' -f $project), '/Game/Tests/Maps/L_PhysicsPrototype',
        '-game', '-windowed', '-RenderOffscreen', '-ResX=1280', '-ResY=720',
        '-unattended', '-nop4', '-nosplash', '-PinballPracticeProbe',
        "-PinballProbeCycles=$CyclesPerRate", "-PinballProbeName=Practice$rate",
        ('"-ExecCmds=t.MaxFPS {0},r.VSync 0"' -f $rate),
        ('"-abslog={0}"' -f $logPath)
    )
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $runs += [pscustomobject]@{ Rate = $rate; Process = $process; Log = $logPath }
}
try {
    foreach ($run in $runs) {
        if (-not $run.Process.WaitForExit(($CyclesPerRate * 55 + 60) * 1000)) {
            throw "Physics run at $($run.Rate) FPS timed out."
        }
        # Engine shutdown can return zero even for a failed probe; require its actual pass marker.
        $result = Select-String -LiteralPath $run.Log -Pattern 'PINBALL_PRACTICE_(PASS|FAIL)' | Select-Object -Last 1
        if (-not $result -or $result.Line -notmatch 'PINBALL_PRACTICE_PASS') {
            throw "Physics run at $($run.Rate) FPS failed. See $($run.Log)"
        }
        Write-Output $result.Line
    }
}
finally {
    foreach ($run in $runs) {
        if (-not $run.Process.HasExited) { Stop-Process -Id $run.Process.Id }
    }
}
