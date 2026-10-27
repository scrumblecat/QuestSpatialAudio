. "$PSScriptRoot/tools.ps1"
$taskStaging = Join-Path $ProjectRoot 'dist/staging'
New-Item -ItemType Directory -Force -Path $taskStaging | Out-Null
$taskNdk = Get-ProjectNdk
$taskStrip = Join-Path $taskNdk 'toolchains/llvm/prebuilt/windows-x86_64/bin/llvm-strip.exe'
Invoke-Checked $taskStrip @('--strip-debug', 'build/libquest-spatial-audio.so', '-o', "$taskStaging/libquest-spatial-audio.so")
Copy-Item -LiteralPath 'mod.template.json' -Destination "$taskStaging/mod.json"
Copy-Item -LiteralPath 'assets/hrtf/NOTICE.md' -Destination "$taskStaging/KEMAR-NOTICE.md"
$taskVersion = (Get-Content -LiteralPath 'mod.template.json' -Raw | ConvertFrom-Json).version
$taskZip = Join-Path $ProjectRoot "dist/QuestSpatialAudio-$taskVersion.zip"
$taskQmod = Join-Path $ProjectRoot "dist/QuestSpatialAudio-$taskVersion.qmod"
Compress-Archive -LiteralPath @("$taskStaging/mod.json", "$taskStaging/libquest-spatial-audio.so", "$taskStaging/KEMAR-NOTICE.md") -DestinationPath $taskZip -Force
Move-Item -LiteralPath $taskZip -Destination $taskQmod -Force
Write-Output "Created $taskQmod"
