#!/usr/bin/env python3
# CTF|认证
from __future__ import annotations

import gzip
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEVICE = ROOT / "artifacts" / "device"
required = ["config.gz", "kallsyms.txt", "version.txt", "boot_a.img", "boot_kernel_Image", "sample_vendor.ko"]
missing = [name for name in required if not (DEVICE / name).is_file()]
if missing:
    raise SystemExit(f"missing: {missing}")

config = gzip.decompress((DEVICE / "config.gz").read_bytes()).decode("utf-8", "replace")
checks = {
    "CONFIG_MODULES=y": "CONFIG_MODULES=y" in config,
    "CONFIG_MODULE_UNLOAD=y": "CONFIG_MODULE_UNLOAD=y" in config,
    "CONFIG_MODVERSIONS=y": "CONFIG_MODVERSIONS=y" in config,
    "CONFIG_MODULE_SIG=y": "CONFIG_MODULE_SIG=y" in config,
}
hashes = {}
for path in sorted(DEVICE.iterdir()):
    if path.is_file():
        h = hashlib.sha256()
        with path.open("rb") as f:
            for chunk in iter(lambda: f.read(1024 * 1024), b""):
                h.update(chunk)
        hashes[path.name] = {"bytes": path.stat().st_size, "sha256": h.hexdigest().upper()}

result = {"tag": "CTF|认证", "missing": missing, "config_checks": checks, "artifacts": hashes}
(DEVICE / "artifact_manifest.json").write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding="utf-8")
print(json.dumps(result, ensure_ascii=False, indent=2))

