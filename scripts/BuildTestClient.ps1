$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:BUILD_ID -notmatch '^[a-f0-9]{24}$' -or $env:SOURCE_SHA -notmatch '^[a-f0-9]{40}$') { throw 'Invalid build identity' }
$options = @($env:MODULE_OPTIONS | ConvertFrom-Json)
$modules = @($env:SELECTED_MODULES | ConvertFrom-Json)
foreach ($option in $options) {
    if ($option -notmatch '^-DWOW_MODULE_[A-Z][A-Z0-9_]{0,63}=(ON|OFF)$' -or $option -match 'WOW_MODULE_TESTS=') { throw 'Invalid module option' }
}
$source = Join-Path $env:GITHUB_WORKSPACE 'source'
$actual = (& git -C $source rev-parse HEAD).Trim()
if ($LASTEXITCODE -ne 0 -or $actual -ne $env:SOURCE_SHA) { throw 'Source revision mismatch' }
$build = Join-Path $env:RUNNER_TEMP 'client-build'
$package = Join-Path $env:RUNNER_TEMP 'client-package'
New-Item -ItemType Directory -Path $package -Force | Out-Null
& cmake -S $source -B $build -G 'Visual Studio 17 2022' -A x64 '-DWOWCLIENT_OUTPUT_OVERRIDE=WowClient' '-DWOWCLIENT_CRASH_LOG=ON' '-DWOWDECOMP_DISABLE_SIGNATURE_CHECKS=OFF' @options
if ($LASTEXITCODE -ne 0) { throw 'Client configure failed' }
& cmake --build $build --config RelWithDebInfo --target WowClient XvidDecoder --parallel 4
if ($LASTEXITCODE -ne 0) { throw 'Client compilation failed' }
$binaryRoot = Join-Path $source 'bin\Win'
Copy-Item (Join-Path $binaryRoot 'WowClient.exe') $package
Get-ChildItem $binaryRoot -Filter '*.dll' | Copy-Item -Destination $package
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vs = (& $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationVersion).Trim()
$metadata = [ordered]@{
    id = $env:BUILD_ID
    sha = $env:SOURCE_SHA
    modules = $modules
    repository = 'moostigre/wowdecomp'
    compiler = "MSVC / Visual Studio $vs"
    architecture = 'x64'
    configuration = 'RelWithDebInfo'
    cmake_options = $options
    cmake = (& cmake --version | Select-Object -First 1)
    created_at = (Get-Date).ToUniversalTime().ToString('o')
    workflow_run = "$env:GITHUB_SERVER_URL/$env:GITHUB_REPOSITORY/actions/runs/$env:GITHUB_RUN_ID"
}
$metadata | ConvertTo-Json -Depth 6 | Set-Content -Encoding utf8NoBOM (Join-Path $package 'build.json')
@'
WoW 3.3.5a build 12340 - native client test build (Windows x64)

Game assets are not included. Copy a compatible Data folder into this build's
folder, or create a directory junction to your existing 3.3.5a Data folder.
Keep this build in its own folder; preserve your existing game executable.
Run LaunchClient.cmd for the OpenGL test configuration.
This package is a compilation artifact, not a gameplay certification.
Source revision and selected native modules are recorded in build.json.
'@ | Set-Content -Encoding utf8NoBOM (Join-Path $package 'README.txt')
'@echo off', 'cd /d "%~dp0"', 'WowClient.exe -opengl' | Set-Content -Encoding ascii (Join-Path $package 'LaunchClient.cmd')
Compress-Archive -Path (Join-Path $package '*') -DestinationPath (Join-Path $env:RUNNER_TEMP 'client.zip') -Force
