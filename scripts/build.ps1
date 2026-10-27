param([switch]$Package)
. "$PSScriptRoot/tools.ps1"
if (-not (Test-Path -LiteralPath 'extern.cmake')) { Invoke-Checked qpm @('restore') }
$taskNdk = Get-ProjectNdk
Invoke-Checked cmake @('-S', '.', '-B', 'build', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=RelWithDebInfo', "-DCMAKE_ANDROID_NDK=$taskNdk")
Invoke-Checked cmake @('--build', 'build', '--parallel', '4')
if ($Package) { & "$PSScriptRoot/package.ps1" }
