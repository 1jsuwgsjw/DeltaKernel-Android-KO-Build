#!/usr/bin/env bash
# CTF|认证
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
WORKSPACE="${WORKSPACE:-$ROOT/workspace/oneplus_ace_6}"
MANIFEST_BRANCH="${MANIFEST_BRANCH:-oneplus/sm8750}"
MANIFEST_FILE="${MANIFEST_FILE:-oneplus_ace_6.xml}"

mkdir -p "$WORKSPACE"
cd "$WORKSPACE"

if [[ ! -d .repo ]]; then
  repo init -u https://github.com/Numbersf/kernel_manifest.git \
    -b "$MANIFEST_BRANCH" -m "$MANIFEST_FILE" \
    --depth=1 --no-clone-bundle --no-tags
fi
repo sync -c -j"$(nproc)" --force-sync --no-clone-bundle --no-tags

LTO=thin SYSTEM_DLKM_RE_SIGN=0 BUILD_SYSTEM_DLKM=0 \
  KMI_SYMBOL_LIST_STRICT_MODE=0 \
  ./kernel_platform/oplus/build/oplus_build_kernel.sh sun perf

COMMON="$WORKSPACE/kernel_platform/common"
SYMVERS=$(find "$WORKSPACE/kernel_platform" -type f -name Module.symvers -path '*/out/*' | head -n 1)
test -n "$SYMVERS"
OUT=$(dirname "$SYMVERS")
CLANG=$(find "$WORKSPACE/kernel_platform/prebuilts/clang/host/linux-x86" \
  -type f -path '*/bin/clang' | sort -V | tail -n 1)
export PATH="$(dirname "$CLANG"):$PATH"

make -C "$COMMON" O="$OUT" ARCH=arm64 LLVM=1 LLVM_IAS=1 \
  M="$ROOT/kernel_module" modules

mkdir -p "$ROOT/dist"
cp -f "$ROOT/kernel_module/delta_kernel.ko" "$ROOT/dist/"
cp -f "$OUT/.config" "$ROOT/dist/kernel.config"
cp -f "$SYMVERS" "$ROOT/dist/Module.symvers"
modinfo "$ROOT/dist/delta_kernel.ko" | tee "$ROOT/dist/modinfo.txt"
sha256sum "$ROOT/dist"/* | tee "$ROOT/dist/SHA256SUMS"

