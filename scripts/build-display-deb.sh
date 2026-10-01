#!/bin/sh
# Builds the architecture-independent flight-simulator-display .deb.
# Usage: scripts/build-display-deb.sh [expected-version]
set -eu

VERSION=$(sed -n 's/^[[:space:]]*VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt | head -1)
if [ -n "${1:-}" ] && [ "$1" != "$VERSION" ]; then
	echo "tag version $1 does not match CMakeLists.txt VERSION $VERSION" >&2
	exit 1
fi

OUT=build-display
rm -rf "$OUT"
mkdir -p "$OUT/pkg"
cp -a packaging/display/. "$OUT/pkg/"
sed "s/@VERSION@/$VERSION/" "$OUT/pkg/DEBIAN/control.in" > "$OUT/pkg/DEBIAN/control"
rm "$OUT/pkg/DEBIAN/control.in"
chmod 755 "$OUT/pkg/DEBIAN" "$OUT/pkg/DEBIAN/postinst" "$OUT/pkg/DEBIAN/prerm" \
	"$OUT/pkg/usr/lib/flight-simulator/"*
dpkg-deb --root-owner-group --build "$OUT/pkg" \
	"$OUT/flight-simulator-display_${VERSION}_all.deb"
ls -l "$OUT"/*.deb
