param(
    [Parameter(Mandatory = $true)]
    [string] $Executable,

    [Parameter(Mandatory = $true)]
    [string] $OutputDir,

    [Parameter(Mandatory = $true)]
    [string] $QtBinDir,

    [string] $Platform = "windows",
    [string[]] $ExpectedPages = @("overview", "input", "buttons", "pickers", "data", "containers", "windows", "motion", "settings", "overview"),
    [int] $ExpectedCount = 0
)

$ErrorActionPreference = "Stop"

$exe = (Resolve-Path -LiteralPath $Executable).Path
$exeDir = Split-Path -Parent $exe

if (-not (Test-Path -LiteralPath $OutputDir)) {
    New-Item -ItemType Directory -Force -Path $OutputDir | Out-Null
}
$resolvedOutput = (Resolve-Path -LiteralPath $OutputDir).Path
Get-ChildItem -LiteralPath $resolvedOutput -Filter "*.png" -File -ErrorAction SilentlyContinue | Remove-Item -Force
Remove-Item -LiteralPath (Join-Path $resolvedOutput "_render.log") -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath (Join-Path $resolvedOutput "stdout.txt") -Force -ErrorAction SilentlyContinue
Remove-Item -LiteralPath (Join-Path $resolvedOutput "stderr.txt") -Force -ErrorAction SilentlyContinue

function ConvertTo-SafeBaselineName {
    param([Parameter(Mandatory = $true)][string] $Text)

    $safe = $Text -replace "[^A-Za-z0-9_.-]+", "_"
    if ($safe.Length -gt 120) {
        return $safe.Substring(0, 120)
    }
    return $safe
}

function Test-RenderedPngHealth {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Directory,

        [Parameter(Mandatory = $true)]
        [string] $Filter,

        [Parameter(Mandatory = $true)]
        [int] $MinimumCount
    )

    $files = @(Get-ChildItem -LiteralPath $Directory -Filter $Filter -File -ErrorAction SilentlyContinue)
    if ($files.Count -lt $MinimumCount) {
        Write-Error "Expected at least $MinimumCount rendered images, captured $($files.Count)."
        return $false
    }

    try {
        Add-Type -AssemblyName System.Drawing -ErrorAction Stop
    } catch {
        Write-Warning "System.Drawing is unavailable; skipped PNG health validation."
        return $true
    }

    foreach ($file in $files) {
        if ($file.Length -lt 1024) {
            Write-Error "Rendered image $($file.Name) is unexpectedly small ($($file.Length) bytes)."
            return $false
        }

        $image = [System.Drawing.Bitmap]::FromFile($file.FullName)
        try {
            if ($image.Width -lt 320 -or $image.Height -lt 240) {
                Write-Error "Rendered image $($file.Name) has unexpected dimensions $($image.Width)x$($image.Height)."
                return $false
            }

            $sampleColors = @{}
            $stepX = [Math]::Max(1, [Math]::Floor($image.Width / 18))
            $stepY = [Math]::Max(1, [Math]::Floor($image.Height / 18))
            for ($y = [Math]::Floor($stepY / 2); $y -lt $image.Height; $y += $stepY) {
                for ($x = [Math]::Floor($stepX / 2); $x -lt $image.Width; $x += $stepX) {
                    $sampleColors[$image.GetPixel($x, $y).ToArgb()] = $true
                }
            }

            if ($sampleColors.Count -lt 3) {
                Write-Error "Rendered image $($file.Name) appears blank or nearly blank; sampled $($sampleColors.Count) distinct colors."
                return $false
            }
        } finally {
            $image.Dispose()
        }
    }

    return $true
}

function Test-ExpectedInteractionArtifacts {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Directory,

        [Parameter(Mandatory = $true)]
        [string[]] $Pages
    )

    for ($i = 0; $i -lt $Pages.Count; ++$i) {
        $safePage = ConvertTo-SafeBaselineName -Text $Pages[$i]
        $expectedName = "interaction-{0:D2}-{1}.png" -f $i, $safePage
        if (-not (Test-Path -LiteralPath (Join-Path $Directory $expectedName))) {
            Write-Error "Missing expected interaction smoke artifact: $expectedName"
            return $false
        }
    }

    return $true
}

if ($ExpectedCount -le 0) {
    $ExpectedCount = $ExpectedPages.Count
}
if ($ExpectedPages.Count -ne $ExpectedCount) {
    Write-Error "ExpectedPages contains $($ExpectedPages.Count) entries but ExpectedCount is $ExpectedCount."
    exit 1
}

$originalPath = $env:PATH
$originalPlatform = $env:QT_QPA_PLATFORM
$originalSuppressWarnings = $env:QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS

try {
    $env:PATH = "$exeDir;$QtBinDir;$env:PATH"
    $env:QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS = "1"
    if ($Platform) {
        $env:QT_QPA_PLATFORM = $Platform
    }

    Write-Host "Running interaction smoke with QT_QPA_PLATFORM=$env:QT_QPA_PLATFORM"
    $process = Start-Process `
        -FilePath $exe `
        -ArgumentList @("--interaction-smoke", $resolvedOutput) `
        -Wait `
        -PassThru `
        -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $resolvedOutput "stdout.txt") `
        -RedirectStandardError (Join-Path $resolvedOutput "stderr.txt")

    if ($process.ExitCode -ne 0) {
        exit $process.ExitCode
    }
} finally {
    $env:PATH = $originalPath
    if ($null -eq $originalPlatform) {
        Remove-Item Env:QT_QPA_PLATFORM -ErrorAction SilentlyContinue
    } else {
        $env:QT_QPA_PLATFORM = $originalPlatform
    }
    if ($null -eq $originalSuppressWarnings) {
        Remove-Item Env:QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS -ErrorAction SilentlyContinue
    } else {
        $env:QTFLUENT_SUPPRESS_KNOWN_PAINT_WARNINGS = $originalSuppressWarnings
    }
}

if (-not (Test-RenderedPngHealth -Directory $resolvedOutput -Filter "interaction-*.png" -MinimumCount $ExpectedCount)) {
    exit 1
}

$actual = (Get-ChildItem -LiteralPath $resolvedOutput -Filter "interaction-*.png" -File -ErrorAction SilentlyContinue | Measure-Object).Count
if ($actual -ne $ExpectedCount) {
    Write-Error "Expected exactly $ExpectedCount interaction smoke images, captured $actual."
    exit 1
}
if (-not (Test-ExpectedInteractionArtifacts -Directory $resolvedOutput -Pages $ExpectedPages)) {
    exit 1
}

Write-Host "Rendered $actual interaction smoke images in $resolvedOutput"
exit 0
