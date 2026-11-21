#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_VERSION_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PCLDIR="${PCLDIR:-/Users/$USER/PCL}"

export PCLDIR
export PCLSRCDIR="$PCLDIR/src"
export PCLINCDIR="$PCLDIR/include"
export PCLLIBDIR64="$PCLDIR/lib/macos/arm64"
export PCLBINDIR64="$BUILD_VERSION_DIR/bin"

if [[ "$(uname -m)" != "arm64" ]]; then
  echo "This PCL 2.7.0 arm64 build must run on an Apple Silicon macOS host." >&2
  exit 2
fi

if [[ ! -d "$PCLSRCDIR" || ! -d "$PCLINCDIR" ]]; then
  echo "PCL 2.7.0 snapshot not found at $PCLDIR" >&2
  echo "Set PCLDIR to the PCL 2.7.0 PixInsight 1.8.9-3 snapshot." >&2
  exit 2
fi

SDKROOT="$(xcrun --sdk macosx --show-sdk-path)"
mkdir -p "$PCLLIBDIR64" "$PCLBINDIR64"

generate_arm64_makefile() {
  local source_makefile="$1"
  local directory="${source_makefile:h}"
  local target_makefile="$directory/makefile-arm64"

  sed \
    -e 's/-arch x86_64/-arch arm64/g' \
    -e 's/-msse4\.2 *//g' \
    -e 's/-minline-all-stringops *//g' \
    -e 's#-isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/[^[:space:]]*sdk#-isysroot '"$SDKROOT"'#g' \
    -e 's#-Wl,-syslibroot,/Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/[^[:space:]]*sdk#-Wl,-syslibroot,'"$SDKROOT"'#g' \
    "$source_makefile" > "$target_makefile"
  echo "$target_makefile"
}

THIRDPARTY=(lz4 zstd zlib RFC6234 lcms cminpack)
for library in "${THIRDPARTY[@]}"; do
  directory="$PCLSRCDIR/3rdparty/$library/macosx/g++"
  arm64_makefile="$(generate_arm64_makefile "$directory/makefile-x64")"
  make -C "$directory" -f "${arm64_makefile:t}"
done

pcl_directory="$PCLSRCDIR/pcl/macosx/g++"
arm64_makefile="$(generate_arm64_makefile "$pcl_directory/makefile-x64")"
make -C "$pcl_directory" -f "${arm64_makefile:t}"

make -C "$SCRIPT_DIR/g++" -f makefile-arm64 \
  PROJECT_DIR="$PROJECT_DIR" \
  PCLDIR="$PCLDIR"
