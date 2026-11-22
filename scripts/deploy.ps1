# SPDX-License-Identifier: GPL-2.0-or-later
#
# "d": close any serial monitor on the port, build and upload the app,
# then reopen the monitor in its own window.
#
#   powershell -File scripts/deploy.ps1 [-Port COM11] [-Env esp32dev]
#
# Exit status: 0 on success, 1 if the build or upload failed (the monitor is
# reopened either way, so the old firmware's output is still visible).
param(
    [string]$Port = "COM11",
    [string]$Env = "esp32dev"
)

. "$PSScriptRoot\serial_port.ps1"
Stop-SerialMonitor -Port $Port

Set-Location (Split-Path $PSScriptRoot -Parent)
pio run -e $Env -t upload --upload-port $Port
$ok = $LASTEXITCODE -eq 0
if (-not $ok) {
    Write-Host "deploy: build or upload failed. If it couldn't connect, hold BOOT and tap EN." -ForegroundColor Red
}

Start-SerialMonitor -Port $Port
if ($ok) { exit 0 } else { exit 1 }
