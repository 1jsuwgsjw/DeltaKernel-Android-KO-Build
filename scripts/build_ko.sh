#!/usr/bin/env bash
# CTF|认证
set -euo pipefail

: "${KERNEL_SRC:?Set KERNEL_SRC to the matching Android 15 GKI 6.6 kernel source}"
KERNEL_OUT="${KERNEL_OUT:-$KERNEL_SRC/out/android15-6.6}"
MODULE_DIR="$(cd "$(dirname "$0")/../kernel_module" && pwd)"
DIST_DIR="$(cd "$(dirname "$0")/.." && pwd)/dist"

mkdir -p "$DIST_DIR"

make -C "$KERNEL_SRC" O="$KERNEL_OUT" \
  ARCH=arm64 LLVM=1 LLVM_IAS=1 \
  M="$MODULE_DIR" modules

cp -f "$MODULE_DIR/delta_kernel.ko" "$DIST_DIR/delta_kernel.ko"

echo "Built: $DIST_DIR/delta_kernel.ko"
modinfo "$DIST_DIR/delta_kernel.ko" 2>/dev/null || true

