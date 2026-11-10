# Downloads the prebuilt Qt that the app builds against, with aqtinstall.
# Requires uv (https://docs.astral.sh/uv/). CI uses this; locally any Qt 6.8
# or newer for MSVC 2022 works.
#
# Qt goes into the folder named by the environment variable IEC_QT_ROOT, or
# into .qt/ in the repository if it is not set.
#
# Prints the Qt folder to pass to CMake as CMAKE_PREFIX_PATH.

$ErrorActionPreference = 'Stop'

$QtVersion = '6.8.3'
$QtArch = 'win64_msvc2022_64'
$QtArchDir = 'msvc2022_64'

$Repo = Split-Path -Parent $PSScriptRoot
$QtRoot = if ($env:IEC_QT_ROOT) { $env:IEC_QT_ROOT } else { Join-Path $Repo '.qt' }
$QtDir = Join-Path $QtRoot "$QtVersion\$QtArchDir"

# qtbase holds Core, Gui, Widgets, Network and Test; qtsvg loads the SVG icons.
$Markers = @('lib\cmake\Qt6Widgets', 'lib\cmake\Qt6Svg', 'plugins\iconengines\qsvgicon.dll')
$Missing = @($Markers | Where-Object { -not (Test-Path (Join-Path $QtDir $_)) })
if ($Missing) {
    if (-not (Get-Command uvx -ErrorAction SilentlyContinue)) {
        throw 'uv is required to download Qt. Install it from https://docs.astral.sh/uv/ and run this script again.'
    }
    Write-Host "Installing Qt $QtVersion into $QtRoot"
    # uvx and aqt print progress on stderr, which PowerShell turns into errors.
    # Pass it on as text and check the exit code instead.
    $ErrorActionPreference = 'Continue'
    & uvx --from aqtinstall aqt install-qt windows desktop $QtVersion $QtArch -O $QtRoot --archives qtbase qtsvg 2>&1 |
        ForEach-Object { "$_" } | Out-Host
    $ErrorActionPreference = 'Stop'
    if ($LASTEXITCODE -ne 0) { throw "aqtinstall failed with exit code $LASTEXITCODE" }
}

$QtDir
