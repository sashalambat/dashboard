param(
    [string]$Prefix = $(Join-Path $env:LOCALAPPDATA "SBSWars"),
    [string]$Payload = "",
    [switch]$NoDesktop,
    [switch]$NoStartMenu
)

$ErrorActionPreference = "Stop"
$Here = $PSScriptRoot
$Root = $Here
if ((Split-Path -Leaf $Here) -eq "windows" -and (Split-Path -Leaf (Split-Path -Parent $Here)) -eq "Packaging") {
    $Root = Split-Path -Parent (Split-Path -Parent $Here)
}
if (-not $Payload) {
    if (Test-Path (Join-Path $Here "SBSWarsLauncher.exe")) {
        $Payload = $Here
        $Root = $Here
        $repoMaps = Join-Path $Here "..\..\Maps"
        if (-not (Test-Path (Join-Path $Here "Maps")) -and (Test-Path $repoMaps)) {
            $Root = (Resolve-Path (Join-Path $Here "..\..")).Path
        }
    } elseif (Test-Path (Join-Path $Root "Dist\windows\SBSWarsLauncher.exe")) {
        $Payload = Join-Path $Root "Dist\windows"
    } elseif (Test-Path (Join-Path $Root "Binaries\windows\SBSWarsLauncher.exe")) {
        $Payload = Join-Path $Root "Binaries\windows"
    } else {
        $Payload = $Root
    }
}

Write-Host "SBS Wars Windows Setup"
Write-Host "Install to: $Prefix"
Write-Host "Payload:    $Payload"

New-Item -ItemType Directory -Force -Path $Prefix | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $Prefix "Maps") | Out-Null

function Copy-IfExists($src, $dst) {
    if (Test-Path $src) {
        Copy-Item -Recurse -Force $src $dst
    }
}

# Binaries + SDL DLLs
$binSources = @(
    (Join-Path $Payload "SBSWarsLauncher.exe"),
    (Join-Path $Payload "SBSWars.exe"),
    (Join-Path $Payload "SBSWarsServer.exe"),
    (Join-Path $Payload "SBSWarsInstaller.exe"),
    (Join-Path $Payload "sbswars-launcher.exe"),
    (Join-Path $Payload "sbswars.exe"),
    (Join-Path $Payload "sbswars-server.exe")
)
Get-ChildItem -Path $Payload -Filter *.exe -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item -Force $_.FullName $Prefix
}
Get-ChildItem -Path $Payload -Filter *.dll -ErrorAction SilentlyContinue | ForEach-Object {
    Copy-Item -Force $_.FullName $Prefix
}

$launcher = Join-Path $Prefix "SBSWarsLauncher.exe"
if (-not (Test-Path $launcher)) {
    $alt = Join-Path $Prefix "sbswars-launcher.exe"
    if (Test-Path $alt) {
        Copy-Item -Force $alt $launcher
    }
}
$game = Join-Path $Prefix "SBSWars.exe"
if (-not (Test-Path $game)) {
    $alt = Join-Path $Prefix "sbswars.exe"
    if (Test-Path $alt) { Copy-Item -Force $alt $game }
}

# Content
foreach ($dir in @("Maps", "Content", "Audio", "UI", "Docs")) {
    $src = Join-Path $Root $dir
    if (-not (Test-Path $src) -and (Test-Path (Join-Path $Payload $dir))) {
        $src = Join-Path $Payload $dir
    }
    if (Test-Path $src) {
        Copy-Item -Recurse -Force $src (Join-Path $Prefix $dir)
    }
}

$icon = Join-Path $Root "Packaging\windows\sbswars.ico"
if (Test-Path $icon) {
    Copy-Item -Force $icon (Join-Path $Prefix "sbswars.ico")
}

function New-Shortcut($path, $target, $workDir, $iconPath) {
    $ws = New-Object -ComObject WScript.Shell
    $sc = $ws.CreateShortcut($path)
    $sc.TargetPath = $target
    $sc.WorkingDirectory = $workDir
    if (Test-Path $iconPath) { $sc.IconLocation = $iconPath }
    $sc.Description = "SBS Wars"
    $sc.Save()
}

$target = $launcher
if (-not (Test-Path $target)) { $target = $game }
$ico = Join-Path $Prefix "sbswars.ico"
if (-not (Test-Path $target)) {
    throw "SBS Wars launcher was not found. Build the Windows binaries first (see Docs\BUILD_GUIDE.md)."
}

if (-not $NoDesktop) {
    $desktop = [Environment]::GetFolderPath("Desktop")
    $link = Join-Path $desktop "SBS Wars.lnk"
    New-Shortcut $link $target $Prefix $ico
    Write-Host "Desktop shortcut: $link"
}

if (-not $NoStartMenu) {
    $programs = Join-Path ([Environment]::GetFolderPath("StartMenu")) "Programs\SBS Wars"
    New-Item -ItemType Directory -Force -Path $programs | Out-Null
    New-Shortcut (Join-Path $programs "SBS Wars.lnk") $target $Prefix $ico
    Write-Host "Start Menu shortcut: $programs\SBS Wars.lnk"
}

$uninstall = @"
Remove-Item -Force -ErrorAction SilentlyContinue "`$env:USERPROFILE\Desktop\SBS Wars.lnk"
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue "`$env:APPDATA\Microsoft\Windows\Start Menu\Programs\SBS Wars"
Remove-Item -Recurse -Force -ErrorAction SilentlyContinue "$Prefix"
Write-Host "SBS Wars uninstalled."
"@
Set-Content -Path (Join-Path $Prefix "Uninstall.ps1") -Value $uninstall

Write-Host "Installed SBS Wars to $Prefix"
Write-Host "Launch: $target"
Write-Host "Double-click the Desktop shortcut 'SBS Wars' to play."
