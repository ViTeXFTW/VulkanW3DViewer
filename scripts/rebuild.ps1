# PowerShell script to rebuild a CMake preset
# Usage: .\rebuild-preset.ps1 [optional -d] -Preset debug
# Available presets: debug, release, test

param(
    [Parameter(Mandatory=$true)]
    [ValidateSet("debug", "release", "test")]
    [string]$Preset,
    [switch]$R,
    [switch]$D
)

$buildDir = "build/$Preset"

if ($D) {
    Write-Host "Deleting build directory: $buildDir" -ForegroundColor Yellow
    if (Test-Path $buildDir) {
        Remove-Item -Recurse -Force $buildDir
        Write-Host "Build directory deleted." -ForegroundColor Green
    } else {
        Write-Host "Build directory does not exist." -ForegroundColor Cyan
    }
}

Write-Host ""
Write-Host "Rebuilding preset: $Preset" -ForegroundColor Yellow
cmake --preset $Preset
if ($LASTEXITCODE -eq 0) {
    Write-Host "Preset configuration successful." -ForegroundColor Green
} else {
    Write-Host "Preset configuration failed." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Building preset: $Preset" -ForegroundColor Yellow
cmake --build --preset $Preset
if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful." -ForegroundColor Green
} else {
    Write-Host "Build failed." -ForegroundColor Red
    exit 1
}

if($R) {
    Write-Host ""
    Write-Host "Running application for preset: $Preset" -ForegroundColor Yellow
    $exePath = Join-Path $buildDir "VulkanW3DViewer.exe"
    if (Test-Path $exePath) {
        & $exePath
    } else {
        Write-Host "Executable not found: $exePath" -ForegroundColor Red
        exit 1
    }
}