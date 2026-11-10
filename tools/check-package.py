#!/usr/bin/env python3
"""Checks that an installed app holds what a package needs.

    python tools/check-package.py <install folder>

The folder is the prefix given to `cmake --install --component App`. The
script finds the layout from what is there: IconEmailCheck.app on macOS,
bin/IconEmailCheck.exe on Windows, or opt/icon-email-check on Linux. It fails,
naming each missing file, if the app lacks its executable, the Qt libraries it
links, the platform plugin, the SVG plugins for its icons, or a TLS plugin,
without which no mail server or Google can be reached.
"""

from __future__ import annotations

import sys
from pathlib import Path


def windows(root: Path) -> list[str]:
    return [
        "bin/IconEmailCheck.exe",
        "bin/Qt6Core.dll",
        "bin/Qt6Gui.dll",
        "bin/Qt6Widgets.dll",
        "bin/Qt6Network.dll",
        "bin/Qt6Svg.dll",
        "bin/vcruntime140.dll",
        "plugins/platforms/qwindows.dll",
        "plugins/iconengines/qsvgicon.dll",
        "plugins/imageformats/qsvg.dll",
        "plugins/tls/qschannelbackend.dll",
        "LICENSE",
    ]


def macos(root: Path) -> list[str]:
    contents = "IconEmailCheck.app/Contents"
    return [
        f"{contents}/MacOS/IconEmailCheck",
        f"{contents}/Info.plist",
        f"{contents}/Resources/app.icns",
        *[f"{contents}/Frameworks/Qt{m}.framework" for m in ("Core", "Gui", "Widgets", "Network", "Svg")],
        f"{contents}/PlugIns/platforms/libqcocoa.dylib",
        f"{contents}/PlugIns/iconengines/libqsvgicon.dylib",
        f"{contents}/PlugIns/imageformats/libqsvg.dylib",
        f"{contents}/PlugIns/tls/libqsecuretransportbackend.dylib",
        "LICENSE",
    ]


def linux(root: Path) -> list[str]:
    app = "opt/icon-email-check"
    return [
        f"{app}/bin/IconEmailCheck",
        *[f"{app}/lib/libQt6{m}.so.6" for m in ("Core", "Gui", "Widgets", "Network", "Svg", "DBus", "XcbQpa")],
        f"{app}/plugins/platforms/libqxcb.so",
        f"{app}/plugins/iconengines/libqsvgicon.so",
        f"{app}/plugins/imageformats/libqsvg.so",
        f"{app}/plugins/tls/libqopensslbackend.so",
        f"{app}/LICENSE",
        "usr/bin/icon-email-check",
        "usr/share/applications/icon-email-check.desktop",
        "usr/share/icons/hicolor/scalable/apps/icon-email-check.svg",
    ]


def main() -> int:
    if len(sys.argv) != 2:
        print(__doc__, file=sys.stderr)
        return 2
    root = Path(sys.argv[1])
    if (root / "IconEmailCheck.app").is_dir():
        expected = macos(root)
    elif (root / "bin/IconEmailCheck.exe").is_file():
        expected = windows(root)
    elif (root / "opt/icon-email-check").is_dir():
        expected = linux(root)
    else:
        print(f"{root} holds no installed app.", file=sys.stderr)
        return 1
    # lexists: usr/bin/icon-email-check is a link into /opt, which does not
    # resolve before the package is installed.
    missing = [path for path in expected if not (root / path).exists() and not (root / path).is_symlink()]
    for path in missing:
        print(f"missing: {path}", file=sys.stderr)
    if missing:
        return 1
    print(f"{root}: all {len(expected)} expected files are there.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
