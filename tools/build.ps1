# Configures, builds and tests one CMake preset on Windows, from any shell.
#
#   tools/build.ps1                  the dev preset
#   tools/build.ps1 -Preset release  another preset
#   tools/build.ps1 -Version 1.2.0   build that version, as the release does
#   tools/build.ps1 -Package         after the tests, install into
#                                    build/<preset>/install, check the install
#                                    and make the setup with CPack (needs NSIS)
#
# Downloads Qt with tools/setup-qt.ps1 if it is missing, and enters the
# Visual Studio 2022 developer environment if the shell is not in one.

param(
    [string]$Preset = 'dev',
    [string]$Version,
    [switch]$Package
)

$ErrorActionPreference = 'Stop'
$Repo = Split-Path -Parent $PSScriptRoot

# Windows locks a running program's file, so the build would fail near its end.
$BuildDir = Join-Path $Repo "build\$Preset\"
$running = Get-Process IconEmailCheck -ErrorAction SilentlyContinue | Where-Object { $_.Path -and $_.Path.StartsWith($BuildDir, [StringComparison]::OrdinalIgnoreCase) }
if ($running) {
    throw "Icon Email Check is still running from $($running[0].Path), so the build cannot replace it. Quit it from its tray icon, then build again."
}

$QtDir = & (Join-Path $PSScriptRoot 'setup-qt.ps1') | Select-Object -Last 1

if (-not $env:VSCMD_VER) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
    if (-not (Test-Path $vswhere)) { throw 'Visual Studio 2022 is required, and vswhere.exe was not found.' }
    $vs = & $vswhere -latest -version '[17.0,18.0)' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vs) { throw 'Visual Studio 2022 with the C++ tools is required.' }
    # The developer shell looks for vswhere.exe on PATH.
    $env:PATH = "$(Split-Path -Parent $vswhere);$env:PATH"
    Import-Module (Join-Path $vs 'Common7\Tools\Microsoft.VisualStudio.DevShell.dll')
    Enter-VsDevShell -VsInstallPath $vs -SkipAutomaticLocation -Arch amd64 -HostArch amd64 | Out-Null
}

# Runs a tool and fails on a non-zero exit code. PowerShell turns a native
# program's stderr output into error records, and CMake prints warnings there,
# so the output is passed on as plain text instead.
function Invoke-Tool {
    $ErrorActionPreference = 'Continue'
    & $args[0] $args[1..($args.Count - 1)] 2>&1 | ForEach-Object { "$_" }
    if ($LASTEXITCODE -ne 0) { throw "$($args[0]) failed with exit code $LASTEXITCODE" }
}

Push-Location $Repo
try {
    $configure = @('--preset', $Preset, "-DCMAKE_PREFIX_PATH=$QtDir")
    if ($Version) { $configure += "-DIEC_VERSION=$Version" }
    Invoke-Tool cmake @configure
    Invoke-Tool cmake --build --preset $Preset
    # The tests load Qt's DLLs from its bin folder.
    $env:PATH = "$(Join-Path $QtDir 'bin');$env:PATH"
    Invoke-Tool ctest --preset $Preset
    if ($Package) {
        $install = Join-Path $Repo "build\$Preset\install"
        if (Test-Path $install) { Remove-Item -Recurse -Force $install }
        Invoke-Tool cmake --install "build\$Preset" --component App --prefix $install
        Invoke-Tool python tools\check-package.py $install
        Push-Location "build\$Preset"
        try { Invoke-Tool cpack } finally { Pop-Location }
    }
} finally {
    Pop-Location
}
