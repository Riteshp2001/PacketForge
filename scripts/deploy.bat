<# :
@echo off
setlocal
echo Calling Interactive Deployment Script...
powershell -NoProfile -ExecutionPolicy Bypass -Command "iex ((Get-Content -LiteralPath '%~f0') | Out-String)"
endlocal
goto :eof
#>

$ErrorActionPreference = "Stop"

function Get-QtKitInfo {
    param($Path)
    # Path is like C:\Qt\6.6.0\msvc2019_64\bin\windeployqt.exe
    # We want "6.6.0" and "msvc2019_64"
    $parent = Split-Path $Path -Parent # bin
    $compiler = Split-Path $parent -Parent # msvc2019_64
    $compilerName = Split-Path $compiler -Leaf
    $version = Split-Path (Split-Path $compiler -Parent) -Leaf
    return [PSCustomObject]@{
        Version = $version
        Compiler = $compilerName
        Path = $Path
    }
}

Write-Host "Starting Interactive Deployment..." -ForegroundColor Cyan

# 1. Locate windeployqt.exe candidates
Write-Host "Searching for installed Qt versions..." -ForegroundColor Cyan

$qtDrive = "C:\Qt"
$candidates = @()

if (Test-Path $qtDrive) {
    # Find all windeployqt.exe
    $files = Get-ChildItem -Path $qtDrive -Filter "windeployqt.exe" -Recurse -ErrorAction SilentlyContinue
    foreach ($f in $files) {
        $candidates += Get-QtKitInfo -Path $f.FullName
    }
}

$selectedTool = $null

if ($candidates.Count -eq 0) {
    # Fallback to PATH
    $fromPath = Get-Command "windeployqt.exe" -ErrorAction SilentlyContinue
    if ($fromPath) {
        Write-Host "No Qt installation found in C:\Qt, but found one in system PATH." -ForegroundColor Yellow
        $selectedTool = $fromPath.Source
    } else {
        Write-Error "No Qt versions found in C:\Qt or System PATH. Please install Qt."
    }
} elseif ($candidates.Count -eq 1) {
    $c = $candidates[0]
    Write-Host "Found single Qt Kit: $($c.Version) - $($c.Compiler)" -ForegroundColor Green
    $selectedTool = $c.Path
} else {
    Write-Host "`nMultiple Qt Kits found. Please select which one to us (match your build kit):" -ForegroundColor White
    $i = 1
    foreach ($c in $candidates) {
        Write-Host "  [$i] Qt $($c.Version) ($($c.Compiler))" -ForegroundColor Yellow
        $i++
    }
    
    while ($true) {
        $selection = Read-Host "`nEnter selection [1-$($candidates.Count)]"
        if ($selection -match "^\d+$" -and [int]$selection -ge 1 -and [int]$selection -le $candidates.Count) {
             $selectedTool = $candidates[[int]$selection - 1].Path
             break
        }
        Write-Host "Invalid selection. Please try again." -ForegroundColor Red
    }
}

Write-Host "`nUsing Tool: $selectedTool" -ForegroundColor Magenta

# 2. Locate Target Executable
$binDir = Get-Location
$allExes = Get-ChildItem -Path $binDir -Filter "*.exe" | Where-Object { $_.Name -ne "windeployqt.exe" }

if ($allExes.Count -eq 0) {
    throw "No executable files found in $binDir. Please build the project (Release) first or run from the build directory."
} elseif ($allExes.Count -eq 1) {
    $exePath = $allExes[0].FullName
    Write-Host "Found target executable: $($allExes[0].Name)" -ForegroundColor Green
} else {
    Write-Host "`nMultiple executables found. Please select target:" -ForegroundColor White
    $i = 1
    foreach ($exe in $allExes) {
        Write-Host "  [$i] $($exe.Name)" -ForegroundColor Yellow
        $i++
    }
    
    while ($true) {
        $selection = Read-Host "`nEnter selection [1-$($allExes.Count)]"
        if ($selection -match "^\d+$" -and [int]$selection -ge 1 -and [int]$selection -le $allExes.Count) {
             $exePath = $allExes[[int]$selection - 1].FullName
             break
        }
        Write-Host "Invalid selection. Please try again." -ForegroundColor Red
    }
}

Write-Host "Target: $exePath" -ForegroundColor Magenta

# 3. Create Files Directory Structure
$projectRoot = Split-Path -Path $binDir -Parent
$filesDir = Join-Path $projectRoot "Files"
$logsDir = Join-Path $filesDir "Logs"

if (-not (Test-Path $filesDir)) { New-Item -ItemType Directory -Path $filesDir | Out-Null }
if (-not (Test-Path $logsDir)) { New-Item -ItemType Directory -Path $logsDir -Force | Out-Null }

# 4. Run Deployment
Write-Host "Running windeployqt..." -ForegroundColor Yellow
$args = @("--dir", $binDir, "--no-translations", "--compiler-runtime", $exePath)

Start-Process -FilePath $selectedTool -ArgumentList $args -NoNewWindow -Wait

Write-Host "Deployment Completed Successfully!" -ForegroundColor Green
