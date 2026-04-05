"""
PlatformIO extra_script — post-build step.

Produces two files per environment after each successful build:
  firmware_<env>.bin      — merged binary (Web Flasher, flash at 0x0)
  firmware_<env>_ota.bin  — app-only binary (OTA via web panel)
"""

import subprocess
import shutil
import sys
import os
Import("env")

def merge_firmware(source, target, env):
    build_dir   = env.subst("$BUILD_DIR")
    project_dir = env.subst("$PROJECT_DIR")
    env_name    = env["PIOENV"].replace("-", "_")
    piohome = os.path.join(os.path.expanduser("~"), ".platformio")

    app_bin    = os.path.join(build_dir, "firmware.bin")
    bootloader = os.path.join(build_dir, "bootloader.bin")
    partitions = os.path.join(build_dir, "partitions.bin")
    boot_app0  = os.path.join(
        piohome, "packages", "framework-arduinoespressif32",
        "tools", "partitions", "boot_app0.bin"
    )
    esptool = os.path.join(piohome, "packages", "tool-esptoolpy", "esptool.py")

    merged_out = os.path.join(project_dir, f"firmware_{env_name}.bin")
    ota_out    = os.path.join(project_dir, f"firmware_{env_name}_ota.bin")

    # OTA binary = plain app image (no bootloader / partitions)
    shutil.copy(app_bin, ota_out)
    print(f"[merge] OTA binary   : {os.path.basename(ota_out)}")

    # Chip-specific flash parameters
    mcu = env.subst("$BOARD_MCU").lower()
    if "esp32s3" in mcu:
        chip, boot_addr, flash_freq, flash_size = "esp32s3", "0x0000", "80m", "8MB"
    else:
        chip, boot_addr, flash_freq, flash_size = "esp32",   "0x1000", "40m", "4MB"

    cmd = [
        sys.executable, esptool,
        "--chip", chip,
        "merge_bin",
        "-o", merged_out,
        "--flash_mode", "dio",
        "--flash_freq", flash_freq,
        "--flash_size", flash_size,
        boot_addr, bootloader,
        "0x8000",  partitions,
        "0xE000",  boot_app0,
        "0x10000", app_bin,
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stderr)
        raise Exception("esptool merge_bin failed — see output above")
    print(f"[merge] Merged binary : {os.path.basename(merged_out)}  (flash at 0x0)")

env.AddPostAction("$BUILD_DIR/firmware.bin", merge_firmware)
