[CmdletBinding()]
param(
    [string]$PbrtExe = "D:\coderepo\cpp-base\pbrt-v4-fork\build\cmake-RelWithDebInfo-visualstudio\pbrt.exe",
    [string]$Scene = "D:\coderepo\cpp-base\pbrt-v4-fork\scenes\killeroos\killeroo-simple-debug.pbrt",
    [string]$OutputDir = (Join-Path $PSScriptRoot "out"),
    [string]$TraceName = "pbrt-killeroo",
    [string]$ProcessName = "pbrt.exe",
    [int]$NumThreads = 8,
    [string]$PerlExe,
    [string]$SymbolPath,
    [switch]$Symbols,
    [switch]$NoOpen
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Command {
    param([string]$Name, [string]$InstallHint)

    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        throw "Cannot find '$Name'. $InstallHint"
    }
}

function Resolve-Perl {
    param([string]$RequestedPath)

    $candidates = @()
    if ($RequestedPath) {
        $candidates += $RequestedPath
    }

    $pathCommand = Get-Command perl -ErrorAction SilentlyContinue
    if ($pathCommand) {
        $candidates += $pathCommand.Source
    }

    $candidates += @(
        "C:\Program Files\Git\usr\bin\perl.exe",
        "C:\Strawberry\perl\bin\perl.exe",
        "C:\msys64\usr\bin\perl.exe",
        "D:\msys64\usr\bin\perl.exe"
    )

    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return (Resolve-Path -LiteralPath $candidate).Path
        }
    }

    throw "Cannot find perl.exe. Install Strawberry Perl/Git for Windows Perl, add perl to PATH, or pass -PerlExe."
}

function Assert-Admin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
        throw "xperf tracing requires an elevated PowerShell session. Restart PowerShell as Administrator and run this script again."
    }
}

Assert-Admin
Assert-Command xperf "Install Windows Performance Toolkit."
Assert-Command wpaexporter "Install Windows Performance Toolkit."
Assert-Command python "Install Python 3.11 or newer."
$resolvedPerl = Resolve-Perl $PerlExe

if (-not (Test-Path -LiteralPath $PbrtExe -PathType Leaf)) {
    throw "Cannot find pbrt executable: $PbrtExe"
}

if (-not (Test-Path -LiteralPath $Scene -PathType Leaf)) {
    throw "Cannot find scene file: $Scene"
}

$converter = Join-Path $PSScriptRoot "xperf_to_collapsedstacks.py"
$profile = Join-Path $PSScriptRoot "ExportCPUUsageSampled.wpaProfile"
if (-not (Test-Path -LiteralPath $converter -PathType Leaf)) {
    throw "Cannot find converter script: $converter"
}
if (-not (Test-Path -LiteralPath $profile -PathType Leaf)) {
    throw "Cannot find WPA export profile: $profile"
}

New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
$etlPath = Join-Path $OutputDir "$TraceName.etl"
$sceneDir = Split-Path -Parent $Scene

Write-Host "Starting xperf CPU sampling..."
& xperf -on PROC_THREAD+LOADER+PROFILE -stackwalk Profile -BufferSize 1024 -MinBuffers 256 -MaxBuffers 1024

try {
    Write-Host "Running pbrt..."
    $process = Start-Process -FilePath $PbrtExe -ArgumentList @($Scene) -WorkingDirectory $sceneDir -NoNewWindow -Wait -PassThru
    if ($process.ExitCode -ne 0) {
        throw "pbrt exited with code $($process.ExitCode)."
    }
}
finally {
    Write-Host "Stopping xperf and writing ETL..."
    & xperf -d $etlPath
}

Write-Host "Converting ETL to collapsed stacks and SVG..."
$convertArgs = @($converter, $etlPath, "-p", $ProcessName, "-n", $NumThreads, "-o", $OutputDir, "--perl", $resolvedPerl)
if ($NoOpen) {
    $convertArgs += "-d"
}
if ($Symbols) {
    if (-not $SymbolPath) {
        $SymbolPath = Split-Path -Parent $PbrtExe
    }
    elseif (Test-Path -LiteralPath $SymbolPath -PathType Leaf) {
        $SymbolPath = Split-Path -Parent (Resolve-Path -LiteralPath $SymbolPath).Path
    }
    if (-not (Test-Path -LiteralPath $SymbolPath -PathType Container)) {
        throw "Cannot find symbol directory: $SymbolPath"
    }
    Write-Host "Using local symbol path: $SymbolPath"
    $convertArgs += "-s"
    $convertArgs += @("--symbol-path", $SymbolPath)
}
& python @convertArgs
if ($LASTEXITCODE -ne 0) {
    throw "xperf_to_collapsedstacks.py failed with exit code $LASTEXITCODE."
}

Write-Host "Done. Outputs are under: $OutputDir"
