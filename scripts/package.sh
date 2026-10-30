#!/usr/bin/env bash
# Zips manifest + .so + config into a .levipack (same layout Camera Overhaul documents).
# Usage: scripts/package.sh path/to/libchukl_levi.so
set -euo pipefail
SO="${1:?path to libchukl_levi.so}"
OUT=dist/arm64-v8a
rm -rf "$OUT" && mkdir -p "$OUT/chukl_levi/config"
cp manifest.json "$OUT/chukl_levi/"
cp "$SO" "$OUT/chukl_levi/libchukl_levi.so"
cp config/config.json "$OUT/chukl_levi/config/"
(cd "$OUT/chukl_levi" && zip -r ../chukl_levi.levipack .)
echo "Built $OUT/chukl_levi.levipack"
