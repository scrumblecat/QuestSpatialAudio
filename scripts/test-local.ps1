param([string]$Zig)
. "$PSScriptRoot/tools.ps1"
if (-not $Zig) {
    if (Get-Command zig -ErrorAction SilentlyContinue) { $Zig = 'zig' }
    else { throw 'Install a portable Zig compiler or pass -Zig with its executable path.' }
}
New-Item -ItemType Directory -Force -Path build-tests/windows | Out-Null
Invoke-Checked $Zig @('c++', '-std=c++20', '-O3', '-Wall', '-Wextra', '-Werror', '-Iinclude', 'tests/renderer_test.cpp', 'src/renderer.cpp', 'src/upmixer.cpp', 'src/hrtf_data.cpp', '-o', 'build-tests/windows/qsa-dsp-tests.exe')
Invoke-Checked './build-tests/windows/qsa-dsp-tests.exe' @()
