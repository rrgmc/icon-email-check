#!/bin/sh
# Makes resources/app/app.icns (macOS bundle icon) and resources/app/app.ico
# (Windows executable icon) from resources/icons/app.svg. Run it after changing
# the SVG, and commit the results. Requires rsvg-convert (brew install librsvg),
# python3, and on macOS iconutil for the .icns.
#
# The SVG's rounded square, 240 of its 256 units, fills 824 of 1024 pixels on
# the macOS icon grid, so the Dock shows it the size of other apps' icons.

set -eu

REPO=$(cd "$(dirname "$0")/.." && pwd)
SVG=$REPO/resources/icons/app.svg
OUT=$REPO/resources/app

command -v rsvg-convert >/dev/null 2>&1 || { echo "rsvg-convert is required: brew install librsvg" >&2; exit 1; }

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT

# macOS: the drawing placed on the icon grid.
GRID=$WORK/grid.svg
{
    echo '<svg xmlns="http://www.w3.org/2000/svg" width="1024" height="1024" viewBox="0 0 1024 1024">'
    echo '<g transform="translate(72.5333 72.5333) scale(3.43333)">'
    sed -e '1d' -e '$d' "$SVG"
    echo '</g>'
    echo '</svg>'
} > "$GRID"

if command -v iconutil >/dev/null 2>&1; then
    ICONSET=$WORK/app.iconset
    mkdir "$ICONSET"
    for size in 16 32 128 256 512; do
        rsvg-convert -w "$size" -h "$size" "$GRID" -o "$ICONSET/icon_${size}x${size}.png"
        double=$((size * 2))
        rsvg-convert -w "$double" -h "$double" "$GRID" -o "$ICONSET/icon_${size}x${size}@2x.png"
    done
    iconutil -c icns "$ICONSET" -o "$OUT/app.icns"
    echo "Wrote $OUT/app.icns"
fi

# Windows: an .ico of PNG images, the full square without the macOS margin.
for size in 16 20 24 32 40 48 64 128 256; do
    rsvg-convert -w "$size" -h "$size" "$SVG" -o "$WORK/ico_$size.png"
done
python3 -I - "$WORK" "$OUT/app.ico" <<'PY'
import struct, sys, os
work, out = sys.argv[1], sys.argv[2]
sizes = [16, 20, 24, 32, 40, 48, 64, 128, 256]
images = [open(os.path.join(work, f"ico_{s}.png"), "rb").read() for s in sizes]
header = struct.pack("<HHH", 0, 1, len(images))
offset = 6 + 16 * len(images)
entries = b""
for s, data in zip(sizes, images):
    entries += struct.pack("<BBBBHHII", s % 256, s % 256, 0, 0, 1, 32, len(data), offset)
    offset += len(data)
with open(out, "wb") as f:
    f.write(header + entries + b"".join(images))
PY
echo "Wrote $OUT/app.ico"
