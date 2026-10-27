param([string]$Serial)
. "$PSScriptRoot/tools.ps1"
$taskNdk = Get-ProjectNdk
$taskToolchain = Join-Path $taskNdk 'build/cmake/android.toolchain.cmake'
Invoke-Checked cmake @('-S', '.', '-B', 'build-tests', '-G', 'Ninja', '-DQSA_TEST_ONLY=ON', "-DCMAKE_TOOLCHAIN_FILE=$taskToolchain", '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=29', '-DANDROID_STL=c++_static', '-DCMAKE_BUILD_TYPE=Release')
Invoke-Checked cmake @('--build', 'build-tests', '--parallel', '4')
$taskAdbArgs = @()
if ($Serial) { $taskAdbArgs = @('-s', $Serial) }
Invoke-Checked adb ($taskAdbArgs + @('push', 'build-tests/qsa-dsp-tests', '/data/local/tmp/qsa-dsp-tests'))
Invoke-Checked adb ($taskAdbArgs + @('shell', 'chmod', '755', '/data/local/tmp/qsa-dsp-tests'))
Invoke-Checked adb ($taskAdbArgs + @('shell', '/data/local/tmp/qsa-dsp-tests'))
