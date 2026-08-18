param(
    [switch]$Launch = $true
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Set-Location $Root

$winDist = Join-Path $Root "Dist\windows\SBSWarsLauncher.exe"
$built = Join-Path $Root "Binaries\SBSWarsLauncher.exe"
$legacy = Join-Path $Root "Binaries\sbswars-launcher.exe"

if (-not (Test-Path $winDist) -and -not (Test-Path $built) -and -not (Test-Path $legacy)) {
    Write-Host "Windows binaries not found. Building with CMake..."
    $vcpkg = $env:VCPKG_ROOT
    $args = @("-S", ".", "-B", "build-win", "-DCMAKE_BUILD_TYPE=Release")
    if ($vcpkg -and (Test-Path (Join-Path $vcpkg "scripts\buildsystems\vcpkg.cmake"))) {
        $args += "-DCMAKE_TOOLCHAIN_FILE=$vcpkg\scripts\buildsystems\vcpkg.cmake"
        $args += "-DVCPKG_FEATURE_FLAGS=manifests"
    }
    & cmake @args
    if ($LASTEXITCODE -ne 0) { throw "CMake configure failed. Install Visual Studio + vcpkg (sdl2 sdl2-mixer sdl2-ttf). See Docs\BUILD_GUIDE.md" }
    & cmake --build build-win --config Release
    if ($LASTEXITCODE -ne 0) { throw "Build failed." }
}

& (Join-Path $PSScriptRoot "install.ps1")
if ($Launch) {
    $exe = Join-Path $env:LOCALAPPDATA "SBSWars\SBSWarsLauncher.exe"
    if (Test-Path $exe) { Start-Process $exe }
}
