param(
    [string]$EngineRoot = 'C:/Program Files/Epic Games/UE_5.8',
    [ValidateRange(3, 100)][int]$Sessions = 3,
    [ValidateRange(1, 3600)][int]$PerBallSeconds = 65,
    [ValidateRange(60, 3600)][int]$SetupAndMenuSeconds = 60,
    [ValidateRange(1, 3600)][int]$StartupGraceSeconds = 90,
    [ValidateRange(1, 3600)][int]$ShutdownGraceSeconds = 30,
    [switch]$Remediation,
    [switch]$BudgetOnly
)

# Launch one rendered acceptance process and require both its scenario marker and clean Quit.
$ErrorActionPreference = 'Stop'
$repo = Split-Path -Parent $PSScriptRoot
$editor = Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'PinballBattle.uproject'
$ballsPerSession = 3
if (-not $PSBoundParameters.ContainsKey('SetupAndMenuSeconds')) { $SetupAndMenuSeconds = 20 * $Sessions }
if ($SetupAndMenuSeconds -lt 20 * $Sessions) { throw 'Allow at least 20 seconds of launch/menu overhead per session.' }
$scenarioSeconds = $Sessions * $ballsPerSession * $PerBallSeconds + $SetupAndMenuSeconds
$probeSeconds = if ($Remediation) { 30 } else { $scenarioSeconds }
$processSeconds = $probeSeconds + $StartupGraceSeconds + $ShutdownGraceSeconds
Write-Output "Phase 4 budget: $Sessions sessions x $ballsPerSession balls x ${PerBallSeconds}s + ${SetupAndMenuSeconds}s overhead; probe=${probeSeconds}s process=${processSeconds}s. Ball deadlines include pauses."
if ($BudgetOnly) { return }
$runName = if ($Remediation) { 'Remediation' } else { 'Session' }
$marker = if ($Remediation) { 'PINBALL_REMEDIATION' } else { 'PINBALL_SESSION' }
$logPath = Join-Path $repo "Saved/Logs/Phase4$runName.log"
$arguments = @(
    ('"{0}"' -f $project), '/Game/Cabinets/AlienInvasion/Maps/L_AlienCabinet',
    '-game', '-windowed', '-RenderOffscreen', '-ResX=1280', '-ResY=720',
    '-unattended', '-nop4', '-nosplash',
    "-PinballProbeSessions=$Sessions", "-PinballProbeBalls=$ballsPerSession",
    "-PinballProbeBallSeconds=$PerBallSeconds", "-PinballProbeOverheadSeconds=$SetupAndMenuSeconds",
    "-PinballProbeTotalSeconds=$scenarioSeconds",
    '"-ExecCmds=t.MaxFPS 60,r.VSync 0"', ('"-abslog={0}"' -f $logPath)
)
$arguments += if ($Remediation) { '-PinballSessionRemediationProbe' } else { '-PinballSessionProbe' }
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
try {
    if (-not $process.WaitForExit($processSeconds * 1000)) { throw "Phase 4 validation timed out after ${processSeconds}s." }
    $result = Select-String -LiteralPath $logPath -Pattern ($marker + '_(PASS|FAIL)') | Select-Object -Last 1
    if (-not $result -or $result.Line -notmatch ($marker + '_PASS') -or $process.ExitCode -ne 0) {
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
