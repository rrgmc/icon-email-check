#!/bin/sh
# Makes the macOS disk image from an installed, already signed app, with the
# same layout as CPack's: the install folder's contents and a link to
# /Applications. The release workflow's signing job uses this, because cpack
# would run the build's install scripts there.
#
#   sh tools/package-macos.sh <install folder> <disk image>

set -eu

[ $# -eq 2 ] || { echo "usage: sh tools/package-macos.sh <install folder> <disk image>" >&2; exit 2; }
INSTALL=$1
DMG=$2

STAGE=$(mktemp -d)
MOUNT=$(mktemp -d)
trap 'rm -rf "$STAGE"; rmdir "$MOUNT" 2>/dev/null || true' EXIT
ditto "$INSTALL" "$STAGE"
ln -s /Applications "$STAGE/Applications"

# hdiutil sometimes fails with "Resource busy" on a CI runner, and works when it
# runs again.
attempt=1
until hdiutil create -ov -srcfolder "$STAGE" -volname "Icon Email Check" -fs HFS+ -format UDZO "$DMG"; do
    if [ "$attempt" -ge 5 ]; then
        echo "hdiutil failed $attempt times." >&2
        exit 1
    fi
    attempt=$((attempt + 1))
    echo "hdiutil failed; trying again in 15 seconds." >&2
    sleep 15
done

# The layout that the README's install steps describe.
hdiutil attach -readonly -nobrowse -mountpoint "$MOUNT" "$DMG" > /dev/null
contents=$(ls "$MOUNT" | tr '\n' ' ')
hdiutil detach "$MOUNT" > /dev/null
if [ "$contents" != "Applications IconEmailCheck.app LICENSE " ]; then
    echo "The disk image holds: $contents" >&2
    exit 1
fi
echo "Disk image: $DMG"
