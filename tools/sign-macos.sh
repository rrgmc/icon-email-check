#!/bin/sh
# Signs an installed IconEmailCheck.app. The install runs it after macdeployqt,
# which leaves the bundle's binaries unsigned, and the release workflow runs it
# again with the Developer ID.
#
#   sh tools/sign-macos.sh <app bundle> <identity>
#
# With the identity "-", it signs ad hoc, which is enough to start the app on
# the Mac that made it. With a certificate, it signs every piece from the
# inside out, with the hardened runtime and a secure timestamp, as
# notarisation requires. The app needs no entitlements.

set -eu

[ $# -eq 2 ] || { echo "usage: sh tools/sign-macos.sh <app bundle> <identity>" >&2; exit 2; }
APP=$1
IDENTITY=$2

if [ "$IDENTITY" = "-" ]; then
    codesign --force --deep --sign - "$APP"
    codesign --verify --deep --strict "$APP"
    exit 0
fi

sign() {
    codesign --force --options runtime --timestamp --sign "$IDENTITY" "$@"
}

# Loose libraries and plugins first.
find "$APP/Contents" -type f \( -name '*.dylib' -o -name '*.so' \) -print0 | xargs -0 -n 1 sh -c '
    codesign --force --options runtime --timestamp --sign "$0" "$1"' "$IDENTITY"

# Each framework, after what it holds.
for framework in "$APP/Contents/Frameworks"/*.framework; do
    [ -d "$framework" ] && sign "$framework"
done

# The app last.
sign "$APP"

codesign --verify --deep --strict --verbose=2 "$APP"
