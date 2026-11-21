#!/bin/zsh
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/../../.." && pwd)"
BUILD_VERSION_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PCLDIR="${PCLDIR:-/Users/$USER/PCL}"

export PCLDIR
export PCLSRCDIR="$PCLDIR/src"
export PCLINCDIR="$PCLDIR/include"
export PCLLIBDIR64="$PCLDIR/lib/macos/x64"
export PCLBINDIR64="$BUILD_VERSION_DIR/bin"

if [[ "$(uname -m)" != "x86_64" ]]; then
  echo "PCL 2.7.0 macOS makefiles in this snapshot target x86_64." >&2
  echo "Build on an Intel macOS host or obtain an Arm64-compatible PCL SDK." >&2
  exit 2
fi

if [[ ! -d "$PCLSRCDIR" || ! -d "$PCLINCDIR" ]]; then
  echo "PCL 2.7.0 snapshot not found at $PCLDIR" >&2
  echo "Set PCLDIR to the PCL 2.7.0 PixInsight 1.8.9-3 snapshot." >&2
  exit 2
fi

mkdir -p "$PCLLIBDIR64" "$PCLBINDIR64"

THIRDPARTY=(lz4 zstd zlib RFC6234 lcms cminpack)
for library in "${THIRDPARTY[@]}"; do
  make -C "$PCLSRCDIR/3rdparty/$library/macosx/g++" -f makefile-x64
done

make -C "$PCLSRCDIR/pcl/macosx/g++" -f makefile-x64
make -C "$SCRIPT_DIR/g++" -f makefile-x64 \
  PROJECT_DIR="$PROJECT_DIR" \
  PCLDIR="$PCLDIR"

