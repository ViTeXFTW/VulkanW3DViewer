# PowerShell script to rebuild a CMake configuration
# Usage: .\rebuild.ps1 <config> [-Compiler <compiler>] [-D] [-R]
#
# Arguments:
#   config          Build configuration: debug, release, or test
#
# Options:
#   -Compiler       Compiler to use: msvc, clang, or gcc (default: auto-detect)
#   -D              Delete build directory before rebuilding
#   -R              Run the application after building
#
# Examples:
#   .\rebuild.ps1 debug                    # Debug build (auto-detect compiler)
#   .\rebuild.ps1 release -Compiler msvc   # Release build with MSVC
#   .\rebuild.ps1 debug -D -R              # Clean debug build and run

param(
    [Parameter(Mandatory=$false, Position=0)]
    [ValidateSet("debug", "release", "test")]
    [string]$Config,

    [Parameter(Mandatory=$false)]
    [ValidateSet("msvc", "clang", "gcc")]
    [string]$Compiler,

    [switch]$D,
    [switch]$R,
    [switch]$Help
)

# Show help if requested or if no config provided
if ($Help -or -not $Config) {
    Write-Host ""
    Write-Host "rebuild.ps1 - Build script for VulkanW3DViewer" -ForegroundColor Cyan
    Write-Host ""
    Write-Host "USAGE:" -ForegroundColor Yellow
    Write-Host "  .\rebuild.ps1 <config> [-Compiler <compiler>] [-D] [-R]"
    Write-Host "  .\rebuild.ps1 -Help"
    Write-Host ""
    Write-Host "ARGUMENTS:" -ForegroundColor Yellow
    Write-Host "  config          Build configuration: debug, release, or test"
    Write-Host ""
    Write-Host "OPTIONS:" -ForegroundColor Yellow
    Write-Host "  -Compiler       Compiler to use: msvc, clang, or gcc (default: auto-detect)"
    Write-Host "  -D              Delete build directory before rebuilding"
    Write-Host "  -R              Run the application after building"
    Write-Host "  -Help           Show this help message"
    Write-Host ""
    Write-Host "EXAMPLES:" -ForegroundColor Yellow
    Write-Host "  .\rebuild.ps1 debug                    # Debug build (auto-detect compiler)"
    Write-Host "  .\rebuild.ps1 release -Compiler msvc   # Release build with MSVC"
    Write-Host "  .\rebuild.ps1 debug -D -R              # Clean debug build and run"
    Write-Host "  .\rebuild.ps1 test                     # Build and run tests"
    Write-Host "  .\rebuild.ps1 release -Compiler clang  # Release build with Clang"
    Write-Host ""
    exit 0
}

# Function to find Visual Studio installation
function Find-VSInstallation {
    $vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswherePath) {
        $vsPath = & $vswherePath -latest -property installationPath
        if ($vsPath) {
            return $vsPath
        }
    }

    # Fallback: check common installation paths
    $commonPaths = @(
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional",
        "${env:ProgramFiles}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2019\Professional"
    )

    foreach ($path in $commonPaths) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

# Function to set up MSVC environment
function Initialize-VSEnvironment {
    $vsPath = Find-VSInstallation
    if (-not $vsPath) {
        Write-Host "Visual Studio installation not found!" -ForegroundColor Red
        Write-Host "Please install Visual Studio 2019 or later with C++ tools." -ForegroundColor Yellow
        exit 1
    }

    $vcvarsPath = Join-Path $vsPath "VC\Auxiliary\Build\vcvars64.bat"
    if (-not (Test-Path $vcvarsPath)) {
        Write-Host "vcvars64.bat not found at: $vcvarsPath" -ForegroundColor Red
        exit 1
    }

    Write-Host "Setting up MSVC environment from: $vsPath" -ForegroundColor Cyan

    # Call vcvarsall and capture environment variables
    $tempFile = [System.IO.Path]::GetTempFileName()
    cmd /c "`"$vcvarsPath`" && set > `"$tempFile`""

    # Parse and set environment variables
    Get-Content $tempFile | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            $name = $matches[1]
            $value = $matches[2]
            Set-Item -Path "env:$name" -Value $value -Force
        }
    }

    Remove-Item $tempFile
    Write-Host "MSVC environment configured successfully." -ForegroundColor Green
}

# Construct preset name from config and compiler
if ($Compiler) {
    $preset = "$Compiler-$Config"
} else {
    $preset = $Config
}

# Check if MSVC is being used and we're not already in VS Developer environment
if ($Compiler -eq "msvc" -and -not $env:VCINSTALLDIR) {
    Initialize-VSEnvironment
}

$buildDir = "build/$preset"

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
if ($Compiler) {
    Write-Host "Building $Config with $Compiler (preset: $preset)" -ForegroundColor Yellow
} else {
    Write-Host "Building $Config with auto-detected compiler (preset: $preset)" -ForegroundColor Yellow
}
cmake --preset $preset
if ($LASTEXITCODE -eq 0) {
    Write-Host "Preset configuration successful." -ForegroundColor Green
} else {
    Write-Host "Preset configuration failed." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Building..." -ForegroundColor Yellow
cmake --build --preset $preset
if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful." -ForegroundColor Green
} else {
    Write-Host "Build failed." -ForegroundColor Red
    exit 1
}

if ($R) {
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