$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $ProjectRoot

function Invoke-Checked {
    param([string]$Command, [string[]]$Arguments)
    & $Command @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Command exited with code $LASTEXITCODE" }
}

function Get-ProjectNdk {
    $taskNdkFile = Join-Path $ProjectRoot 'ndkpath.txt'
    if (Test-Path -LiteralPath $taskNdkFile) { return (Get-Content -LiteralPath $taskNdkFile -Raw).Trim() }
    if ($env:ANDROID_NDK_HOME) { return $env:ANDROID_NDK_HOME }
    throw 'No Android NDK configured. Run qpm ndk resolve -d first.'
}
