# SPDX-License-Identifier: GPL-2.0-or-later
#
# The test gate of "cpdt" (CLAUDE.md): build, Tier 1, then Tier 2 for
# test_device_board plus each changed area. Stops at the first failure.
# Commit, push and deploy follow only if this exits 0.
#
#   powershell -File scripts/gate.ps1 [-Areas ir,rf] [-Port COM11]
#
# Leaves Tier 2 TEST firmware on the board; scripts/deploy.ps1 restores the app.
# Tier 2 tests IGNORED for absent hardware don't fail the gate; they are
# listed at the end so the unverified areas can be reported.
param(
    [string[]]$Areas = @(),
    [string]$Port = "COM11"
)

. "$PSScriptRoot\serial_port.ps1"
Set-Location (Split-Path $PSScriptRoot -Parent)

function Invoke-Step([string]$Name, [scriptblock]$Cmd) {
    Write-Host "`n=== $Name" -ForegroundColor Cyan
    & $Cmd | Tee-Object -Variable lines | Out-Host
    if ($LASTEXITCODE -ne 0) {
        Write-Host "`ngate: FAILED at '$Name'. No commit, push or deploy." -ForegroundColor Red
        exit 1
    }
    return $lines
}

Invoke-Step "build" { pio run } | Out-Null
Invoke-Step "t1 (native tests)" { pio test -e native } | Out-Null

Stop-SerialMonitor -Port $Port
$filters = @("-f", "test_device_board")
foreach ($a in $Areas) { $filters += @("-f", "test_device_$a") }
$t2 = Invoke-Step "t2 ($($filters -join ' '))" {
    pio test -e esp32dev_test --upload-port $Port @filters
}

$ignored = $t2 | Where-Object { "$_" -match 'IGNORE' }
Write-Host "`ngate: PASSED." -ForegroundColor Green
if ($ignored) {
    Write-Host "Not verified on hardware (IGNORED):" -ForegroundColor Yellow
    $ignored | ForEach-Object { Write-Host "  $_" }
}
Write-Host "Test firmware is on the board; deploy restores the app."
exit 0
