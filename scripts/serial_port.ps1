# SPDX-License-Identifier: GPL-2.0-or-later
#
# Helpers shared by deploy.ps1 and gate.ps1 (dot-source this file).

# Stop every "pio device monitor" process on the port, so upload and Tier 2
# tests can open it (John, 2026-11-19: close it without asking).
function Stop-SerialMonitor([string]$Port) {
    $procs = Get-CimInstance Win32_Process |
        Where-Object { $_.CommandLine -match 'device monitor' -and $_.CommandLine -match "\b$Port\b" }
    foreach ($p in $procs) {
        try { Stop-Process -Id $p.ProcessId -Force -ErrorAction Stop } catch {}
    }
    if ($procs) {
        Write-Host "Closed the serial monitor on $Port."
        Start-Sleep -Milliseconds 500   # let Windows release the port
    }
}

# Reopen the monitor in its own window.
function Start-SerialMonitor([string]$Port) {
    Start-Process pio -ArgumentList "device monitor --port $Port --baud 115200 --filter time"
}
