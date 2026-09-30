#!/bin/sh
# Cross-builds the Raspberry Pi .deb for one architecture.
# Intended to run inside a debian:bookworm container (as CI does) so the
# binary only needs glibc <= 2.36 and runs on Raspberry Pi OS Bookworm+.
#
#   docker run --rm -v "$PWD":/src -w /src debian:bookworm scripts/build-deb.sh armhf
#
# Usage: scripts/build-deb.sh armhf|arm64 [expected-version]
# With expected-version (CI passes the git tag), the build fails unless it
# matches project(VERSION) in CMakeLists.txt, so tag, .deb and --version agree.
set -eu

ARCH="${1:?usage: $0 armhf|arm64 [version]}"
VERSION="${2:-}"
MAX_GLIBC=2.36

case "$ARCH" in
	armhf) TRIPLE=arm-linux-gnueabihf ;;
	arm64) TRIPLE=aarch64-linux-gnu ;;
	*) echo "unsupported arch: $ARCH" >&2; exit 2 ;;
esac

if ! command -v "$TRIPLE-g++" >/dev/null 2>&1; then
	apt-get update -q
	DEBIAN_FRONTEND=noninteractive apt-get install -y -q --no-install-recommends \
		cmake make dpkg-dev file binutils-multiarch "g++-$TRIPLE"
fi

BUILD="build-$ARCH"
rm -rf "$BUILD"
set -- -DCMAKE_SYSTEM_NAME=Linux -DCMAKE_SYSTEM_PROCESSOR="$ARCH" \
	-DCMAKE_CXX_COMPILER="$TRIPLE-g++" -DCMAKE_BUILD_TYPE=Release \
	-DFLIGHTSIM_BUILD_TESTS=OFF -DFLIGHTSIM_WERROR=ON \
	-DCPACK_DEBIAN_PACKAGE_ARCHITECTURE="$ARCH" -DFLIGHTSIM_MIN_GLIBC="$MAX_GLIBC"
if [ -n "$VERSION" ]; then
	PROJECT_VERSION=$(sed -n 's/^[[:space:]]*VERSION \([0-9.]*\).*/\1/p' CMakeLists.txt | head -1)
	if [ "$VERSION" != "$PROJECT_VERSION" ]; then
		echo "tag version $VERSION does not match CMakeLists.txt VERSION $PROJECT_VERSION" >&2
		exit 1
	fi
fi
cmake -S . -B "$BUILD" "$@"
cmake --build "$BUILD" -j"$(nproc)"

# The .deb promises libc6 >= $MAX_GLIBC; fail if the binary needs newer.
NEED=$("$TRIPLE-objdump" -T "$BUILD/flight-simulator-keys" \
	| grep -o 'GLIBC_[0-9.]*' | sed 's/GLIBC_//' | sort -uV | tail -1)
if [ "$(printf '%s\n%s\n' "$NEED" "$MAX_GLIBC" | sort -V | tail -1)" != "$MAX_GLIBC" ]; then
	echo "binary needs glibc $NEED but the package promises >= $MAX_GLIBC" >&2
	exit 1
fi
echo "glibc requirement: $NEED (<= $MAX_GLIBC OK)"

(cd "$BUILD" && cpack)
ls -l "$BUILD"/*.deb
