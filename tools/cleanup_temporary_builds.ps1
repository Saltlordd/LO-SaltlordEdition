#requires -Version 7.0
<##
Preview the reviewed temporary build list by default. Use -Execute to delete.
The current main build, FSR inputs, runtime evidence and nested repositories
are not cleanup targets. Run while no builds or game sessions are active.
##>
[CmdletBinding()]
param([switch]$Execute)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$outRoot = (Resolve-Path -LiteralPath (Join-Path $repoRoot 'out')).Path
$archive = Join-Path $outRoot 'cleanup-builds-20260927-175059'
$names = @(
    'asm-profiler/build', 'battle-motion-replay-build', 'build/tools',
    'capture-background/build', 'cheats-trigger-ui', 'd3d12-ngx-check/build',
    'fg-adapter-check/build', 'fg-checked-validation-20260927/cpu-contract',
    'flatpak-install-check/build-off', 'focused-sr-scene-recovery',
    'fsr-adapter-gpu-build', 'fsr-dx12-fixture-build', 'fsr-dx12-local-compile',
    'fsr-dx12-sdk-build', 'fsr-p2-test', 'fsr-sdk-build', 'fsr-sdk-disabled',
    'import-menu-build', 'issue40/controller-atlas-fixture',
    'issue40/ui-export/build', 'issue70-af-measurement',
    'merge-AF/settings-menu-build', 'native-dlss-cpu', 'native-dlss-d3d12',
    'pr-triage-20260926/settings-menu', 'release-v0.4.2/integration-tests/build',
    'streamline-fg-p0/fsr-p2-battle-cpu-01/build',
    'streamline-fg-p0/fsr-p2-cross-submit-01/build',
    'streamline-fg-p0/fsr-p2-raster-edge-01/build', 'v0.4.0-planning/version-config'
)

function Assert-LocalPath([string]$Path) {
    $full = [IO.Path]::GetFullPath($Path)
    if (!$full.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar,
            [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside out: $full"
    }
    # Check existing ancestors as well as the leaf before traversing/deleting.
    for ($p = $full; $p -and $p.Length -ge $outRoot.Length; $p = Split-Path -Parent $p) {
        if (Test-Path -LiteralPath $p) {
            if ((Get-Item -LiteralPath $p -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) {
                throw "Link or junction is protected: $p"
            }
        }
    }
}

function Get-BuildFiles([string]$Path) {
    foreach ($item in Get-ChildItem -LiteralPath $Path -Force) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) {
            throw "Link or junction is protected: $($item.FullName)"
        }
        if ($item.Name -in @('.git', 'save', 'saves', 'profile', 'game', 'captures', 'settings.ini')) {
            throw "Protected content in build: $($item.FullName)"
        }
        if ($item.PSIsContainer) { Get-BuildFiles $item.FullName }
        else { $item }
    }
}

function Assert-Idle {
    $active = @(Get-CimInstance Win32_Process | Where-Object {
        $_.Name -match '^(cmake|ninja|clang|clang-cl|cl|link|lld-link|LostOdysseyRecomp|Lo.*Test)\.exe$'
    })
    if ($active.Count) {
        throw "Close build/game processes first: $($active.Name -join ', ')"
    }
}

$targets = @(foreach ($name in $names) {
    $path = [IO.Path]::GetFullPath((Join-Path $outRoot $name))
    Assert-LocalPath $path
    if (!(Test-Path -LiteralPath $path)) { continue }
    if (!(Test-Path -LiteralPath (Join-Path $path 'CMakeCache.txt') -PathType Leaf)) {
        throw "Not a recognized CMake build directory: $path"
    }
    $files = @(Get-BuildFiles $path)
    [pscustomobject]@{ Name = $name; Path = $path; Bytes = ($files | Measure-Object Length -Sum).Sum }
})
$targets | Select-Object Name, @{n='MiB'; e={[math]::Round($_.Bytes / 1MB, 2)}} | Format-Table -AutoSize
Write-Host ("{0} directories, {1:N2} MiB before evidence retention." -f $targets.Count, (($targets | Measure-Object Bytes -Sum).Sum / 1MB))
if (!$Execute) {
    Write-Host 'Preview only. Run again with -Execute to delete these directories.'
    return
}

Assert-Idle
Assert-LocalPath $archive
New-Item -ItemType Directory -Path $archive -Force | Out-Null
$deleted = @()
foreach ($target in $targets) {
    Assert-Idle
    Assert-LocalPath $target.Path
    $files = @(Get-BuildFiles $target.Path)
    foreach ($file in $files) {
        $keep = $file.Extension -in @('.log', '.json', '.md', '.patch', '.cmd', '.bat', '.png', '.ppm', '.csv', '.xml') -or
            $file.Name -eq 'CMakeCache.txt' -or
            ($file.DirectoryName -eq $target.Path -and $file.Extension -in @('.txt', '.cpp', '.hlsl', '.ps1', '.py'))
        if (!$keep) { continue }
        $relative = $file.FullName.Substring($target.Path.Length + 1)
        $dest = Join-Path (Join-Path (Join-Path $archive 'evidence') $target.Name) $relative
        Assert-LocalPath $dest
        New-Item -ItemType Directory -Path (Split-Path -Parent $dest) -Force | Out-Null
        Copy-Item -LiteralPath $file.FullName -Destination $dest -Force
        if ((Get-Item -LiteralPath $dest).Length -ne $file.Length) { throw "Evidence copy failed: $dest" }
    }
    Remove-Item -LiteralPath $target.Path -Recurse -Force
    if (Test-Path -LiteralPath $target.Path) { throw "Deletion incomplete: $($target.Path)" }
    $deleted += $target
    $deleted | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $archive 'deleted.json') -Encoding utf8
    Write-Host "Deleted: $($target.Name)"
}
Write-Host "Finished. Deleted $($deleted.Count) directories. Evidence: $archive"
