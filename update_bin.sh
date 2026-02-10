#!/bin/bash
set -euo pipefail

copy_env_bins() {
  local env="$1"
  local target_dir="dist/$env"
  local build_dir=".pio/build/$env"

  mkdir -p "$target_dir"

  cp "$build_dir/bootloader.bin" "$target_dir/bootloader_qio_80m.bin"
  cp "$build_dir/partitions.bin" "$target_dir/evDash.ino.partitions.bin"
  cp "$build_dir/firmware.bin" "$target_dir/evDash.ino.bin"
}

copy_env_bins "m5stack-core2-v1_0"
copy_env_bins "m5stack-core2-v1_1"
copy_env_bins "m5stack-cores3"
