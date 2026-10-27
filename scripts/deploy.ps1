param([string]$Serial, [switch]$Launch)
. "$PSScriptRoot/tools.ps1"
$taskAdbArgs = @()
if ($Serial) { $taskAdbArgs = @('-s', $Serial) }
$taskPackage = 'com.beatgames.beatsaber'
$taskVersion = (& adb @taskAdbArgs shell dumpsys package $taskPackage | Select-String 'versionName=').ToString().Trim()
if ($LASTEXITCODE -ne 0 -or $taskVersion -ne 'versionName=1.40.8_7379') { throw "Expected Beat Saber 1.40.8_7379. Found: $taskVersion" }
$taskRoot = '/sdcard/ModData/com.beatgames.beatsaber'
foreach ($taskDependency in @('beatsaber-hook_v6.4.2', 'custom-types_v0.18.4', 'paper2_scotland2_v4.8.0')) {
    Invoke-Checked adb ($taskAdbArgs + @('shell', 'test', '-f', "$taskRoot/Packages/1.40.8_7379/$taskDependency/mod.json"))
}
$taskLib = "$taskRoot/Modloader/mods/libquest-spatial-audio.so"
& "$PSScriptRoot/package.ps1"
& adb @taskAdbArgs shell test -f $taskLib
if ($LASTEXITCODE -eq 0) {
    $taskBackup = Join-Path $ProjectRoot ('dist/backups/' + (Get-Date -Format 'yyyyMMdd-HHmmss'))
    New-Item -ItemType Directory -Force -Path $taskBackup | Out-Null
    Invoke-Checked adb ($taskAdbArgs + @('pull', $taskLib, "$taskBackup/libquest-spatial-audio.so"))
}
Invoke-Checked adb ($taskAdbArgs + @('shell', 'am', 'force-stop', $taskPackage))
Invoke-Checked adb ($taskAdbArgs + @('push', 'dist/staging/libquest-spatial-audio.so', $taskLib))
Write-Output "Deployed only libquest-spatial-audio.so. Existing dependency libraries were not replaced."
if ($Launch) {
    Invoke-Checked adb ($taskAdbArgs + @('shell', 'am', 'start', '-n', "$taskPackage/com.unity3d.player.UnityPlayerActivity"))
}
