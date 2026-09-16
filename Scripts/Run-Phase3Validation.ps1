param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [ValidateRange(2, 100)][int]$CyclesPerRate = 8,
    [int[]]$Rates = @(30, 60, 120),
    [switch]$FixturesOnly,
    [ValidatePattern('^[A-Za-z0-9_-]+$')][string]$RunLabel = 'Interactions'
)

# Run independent rendered gameplay processes and require their explicit evidence markers.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'PinballBattle.uproject'
$runs = @()
foreach ($rate in $Rates) {
    $logPath = Join-Path $repo "Saved/Logs/Phase3$RunLabel$rate.log"
    $arguments = @(
        ('"{0}"' -f $project), '/Game/Tests/Maps/L_TableInteractions',
        '-game', '-windowed', '-RenderOffscreen', '-ResX=1280', '-ResY=720',
        '-unattended', '-nop4', '-nosplash', '-PinballInteractionProbe',
        "-PinballInteractionCycles=$CyclesPerRate", "-PinballProbeName=$RunLabel$rate",
        ('"-ExecCmds=t.MaxFPS {0},r.VSync 0"' -f $rate), ('"-abslog={0}"' -f $logPath)
    )
    if ($FixturesOnly) { $arguments += '-PinballFixturesOnly' }
    $process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
    $runs += [pscustomobject]@{ Rate = $rate; Process = $process; Log = $logPath }
}
try {
    foreach ($run in $runs) {
        if (-not $run.Process.WaitForExit(($CyclesPerRate * 80 + 180) * 1000)) {
            throw "Interaction validation at $($run.Rate) FPS timed out."
        }
        $result = Select-String -LiteralPath $run.Log -Pattern 'PINBALL_INTERACTIONS_(PASS|FAIL)' | Select-Object -Last 1
        if (-not $result -or $result.Line -notmatch 'PINBALL_INTERACTIONS_PASS') {
            throw "Interaction validation at $($run.Rate) FPS failed. See $($run.Log)"
        }
        Write-Output $result.Line
    }
}
finally {
    foreach ($run in $runs) {
        if (-not $run.Process.HasExited) { Stop-Process -Id $run.Process.Id }
    }
}
