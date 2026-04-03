# Tesla FSD Controller — ESP32 Web Edition

<p align="right">English | <a href="README.md">中文</a></p>

> ## 🚫 Notice: This project no longer works on China-region vehicles
>
> Since approximately **March 31, 2026 at 9 PM**, Tesla remotely pushed a low-level configuration to China-region vehicles that disables FSD-related functionality at the chip level. **CAN bus modifications can no longer bypass this restriction — this project no longer works on China-region vehicles.**
>
> Note: Your **FSD account authorization is not affected**. Any purchased or subscribed FSD entitlement remains valid; only this mod is ineffective.
>
> Vehicles outside China are currently unaffected. The codebase is preserved for educational, research, and international use.

---

ESP32 + WiFi web panel build based on [tesla-open-can-mod](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod).

After flashing, the ESP32 creates a WiFi hotspot. Connect with your phone and control all parameters in real time from a browser — **no reprogramming required**.

---

## ⚡ Quick Flash (Recommended — no software install needed)

> Uses a pre-built binary. Done entirely in the browser in under 5 minutes.

1. [**Download firmware.bin**](https://github.com/wjsall/tesla-fsd-controller/releases/latest/download/firmware.bin)
2. Open [ESP Web Flasher](https://esp.huhn.me/) in **Chrome or Edge**
3. Connect ESP32 via USB → click **Connect** and select the serial port
4. Click **Add File**, select `firmware.bin`, set address to `0x0`
5. Click **Program** and wait for completion
6. Connect to the `FSD-Controller` WiFi hotspot (password: `12345678`), then open `192.168.4.1` in your browser

> ⚠️ Safari does not support WebUSB. Use Chrome or Edge. Some ESP32 boards require holding the **BOOT** button before clicking Connect.

<p align="center">
  <img src="images/screenshot_zh.jpg" width="45%" alt="Web UI - Chinese">
  &nbsp;&nbsp;
  <img src="images/screenshot_en.jpg" width="45%" alt="Web UI - English">
</p>

---

## ⚠️ Important Disclaimer (Read before use)

1. **FSD purchase required** — This device only unlocks a CAN-level restriction. A valid FSD authorization on your Tesla account is required; without it the device has no effect.
2. **Use at your own risk** — The CAN bus connects directly to critical vehicle systems. Incorrect frames could cause unexpected behavior or damage to electronic components. This project is for educational and testing purposes only. The author accepts no liability for vehicle damage, personal injury, or any other loss resulting from use.
3. **Warranty void** — Modifying your vehicle's CAN bus may void the Tesla factory warranty. The user bears all associated risks.
4. **Comply with local laws** — Use of driver-assistance features on public roads must comply with local traffic regulations. Some features may be legally restricted in certain regions.
5. **Accident liability** — Any traffic accidents occurring while using this device — including collisions, property damage, or personal injury — are the sole responsibility of the driver. The author and contributors accept no liability. Always remain attentive, keep hands on the wheel, and be ready to take control at any time.
6. **Test privately first** — On first use, test in a private location to confirm correct operation before driving on public roads.

---

## Hardware Required

| Component | Notes | Approx. Cost |
|-----------|-------|--------------|
| ESP32 Dev Board | Search "ESP32-DevKitC" or "ESP32 38-pin dev board" | $3–6 |
| SN65HVD230 Module | CAN transceiver — **must be the 3.3V version** | $1–3 |
| Dupont wires (F-F) | Female-to-female, 20 cm, get a pack | $1 |
| USB cable | Micro-USB or Type-C depending on your ESP32 model, for flashing | Usually on hand |

> **Why ESP32?** Built-in WiFi and native CAN (TWAI) controller — no extra chips needed, lowest cost.

---

## Wiring

**Model 3 / Model Y 2021 and later (X179):**
![Wiring diagram X179](images/wiring.svg)

**Model 3 2020 and earlier (X652):**
![Wiring diagram X652](images/wiring_x652.svg)

### ESP32 ↔ SN65HVD230

```
ESP32 GPIO 5  →  SN65HVD230 TX  (may be labeled CTX or D)
ESP32 GPIO 4  →  SN65HVD230 RX  (may be labeled CRX or R)
ESP32 3.3V    →  SN65HVD230 VCC
ESP32 GND     →  SN65HVD230 GND
```

### SN65HVD230 ↔ Vehicle CAN Bus

```
SN65HVD230 CANH  →  Vehicle CAN-H  (typically white/brown wire)
SN65HVD230 CANL  →  Vehicle CAN-L  (typically blue/green wire)
```

> 🚫 **Do NOT use the OBD2 port.** The OBD2 diagnostic port connects to a separate diagnostic CAN bus that is isolated by the vehicle's gateway ECU. Modified frames will **never reach the Autopilot computer**. You must connect directly to the internal CAN bus via the X179 or X652 connector.
>
> ⚠️ **The connector locations below are verified for Model 3 / Model Y only.** Model S, Model X, Model 3 Highland, Model Y Juniper, Cybertruck, and other variants use different connectors — do not assume they are the same. Always consult the official Tesla service manual for your vehicle.
>
> - **Model 3 / Model Y (2021 and later):** <a href="https://service.tesla.com/docs/Model3/ElectricalReference/prog-233/connector/x179/" target="_blank" rel="noopener">X179 connector</a> — Pin 13 (CAN-H) / Pin 14 (CAN-L)
> - **Model 3 (2020 and earlier):** <a href="https://service.tesla.com/docs/Model3/ElectricalReference/prog-187/connector/x652/" target="_blank" rel="noopener">X652 connector</a> — Pin 1 (CAN-H) / Pin 2 (CAN-L)
>
> When in doubt, consult the Tesla service manual before touching any wiring.

### Wiring Overview

```
[Vehicle CAN-H] ───────────┐
                            │  SN65HVD230
[Vehicle CAN-L] ───────────┤   module
                         CANH/CANL
                            │
                         TX → GPIO5
                         RX → GPIO4   [ESP32]
                        VCC → 3.3V
                        GND → GND
                                 │
                               USB
                                 │
                          [PC / power bank]
```

---

## Step 1: Install Software

![Compile and flash steps](images/compile-steps.svg)

### 1.1 Install VS Code

1. Go to [https://code.visualstudio.com](https://code.visualstudio.com), download and install.
2. Open VS Code after installation.

### 1.2 Install the PlatformIO Extension

1. Click the **Extensions** icon in the left sidebar (four squares).
2. Search for `PlatformIO IDE` and click **Install**.
3. After installation a house icon appears in the bottom status bar (you may need to restart VS Code).

> **Slow install?** PlatformIO downloads the ESP32 toolchain on first launch (~200–500 MB). Keep your network connection stable and wait.

---

## Step 2: Download and Open the Project

1. Click the green **Code** button at the top of this page and select **Download ZIP**.
2. Extract to any folder, e.g. `C:\tesla-fsd`.
3. In VS Code, go to **File → Open Folder** and select the extracted folder.
4. If prompted "Do you trust the authors?", click **Yes, I trust the authors**.

---

## Step 3: Configuration

> **No code changes required before flashing.** WiFi name/password and hardware version can both be changed after flashing via the web panel. Settings are saved automatically.

### 3.1 Change WiFi Hotspot Name and Password

**Recommended (change via web panel after flashing):**

1. After flashing, connect to the default hotspot `FSD-Controller` (password `12345678`).
2. Open `192.168.4.1` in your browser. Go to the **WiFi Settings** card, enter a new name and password.
3. Click **Save & Restart**. Reconnect using the new credentials.

**Alternative (edit source before flashing):**

Open `src/main.cpp` and find lines 20–21:

```cpp
static char apSSID[33] = "FSD-Controller";
static char apPass[64] = "12345678";
```

Change the quoted values to your preferred name and password (minimum 8 characters).

### 3.2 Know Your Hardware Version

**No code changes needed.** After flashing, select the hardware version from the **Hardware** dropdown in the web panel. The setting is saved to flash and persists across reboots.

Identify your version before connecting so you can select it immediately:

| Display in car | Firmware version | FSD version | Panel selection |
|----------------|-----------------|-------------|-----------------|
| HW4.x | 2026.2.9.x or newer | V14 | `HW4` |
| HW4.x | 2026.8.x or older | V13 | `HW3` ⚠️ |
| HW3.x | Any | V13 | `HW3` |
| Legacy Model S/X (portrait screen) | Any | — | `LEGACY` |

> ⚠️ **HW4 hardware + V13 firmware must use `HW3` mode.** HW4 mode is designed for FSD V14 and causes intermittent behavior on V13 firmware. Check your firmware version under **Controls → Software** in the car.

---

## Step 4: Flash Firmware

### Connect the ESP32

Connect the ESP32 to your computer with a USB cable.

### Flash via VS Code

1. Click **✓ Build** in the bottom status bar to compile and confirm no errors.
2. Click **→ Upload** to flash.
3. The terminal will show `success` when complete.

> **Upload failed?**
> - Check that your USB cable supports data transfer (charge-only cables won't work).
> - Click the port selector in the status bar and confirm the correct COM port is selected.
> - Some ESP32 boards require holding the **BOOT** button until the upload progress bar appears.

---

## Step 5: Using the Control Panel

### 5.1 Connect to WiFi

1. Power the ESP32 (USB power bank or any USB power source).
2. On your phone, find `FSD-Controller` (or your custom name) in WiFi settings and connect.
3. Open `192.168.4.1` in your phone's browser.

### 5.2 Control Panel Reference

| Setting | Description |
|---------|-------------|
| **FSD Enable** | Master switch — when off, no CAN frames are modified |
| **Hardware** | Match to your vehicle hardware (can be changed at runtime) |
| **Speed Profile** | Controls how aggressively FSD drives — see table below |
| **Profile Source** | Auto (Stalk) = follow-distance stalk controls the profile; Manual = fixed to the dropdown selection |
| **ISA Chime Suppress** | Suppresses ISA speed-limit chime (HW4 only) |
| **Emergency Detection** | Enables emergency vehicle detection (HW4 FSD V14 only) |
| **China Mode 🇨🇳** | Required for regions where "Traffic Light and Stop Sign Control" is not available in the car UI (China, Japan, etc.). Without this, the trigger condition is never met and the device has no effect. |
| **WiFi Settings** | Change the hotspot SSID and password. Click Save & Restart — the device reboots with the new credentials. |

### 5.3 Speed Profile Reference

| Profile | HW3 display | HW4 display |
|---------|-------------|-------------|
| Chill | ❄️ Chill | ❄️ Chill |
| Default | 🟢 Normal | 🟢 Normal |
| Moderate | ⚡ Hurry | ⚡ Hurry |
| Aggressive | — | 🔥 Max (fastest) |
| Maximum | — | 🐢 Sloth (slowest) |

> Follow-distance stalk: lower number = more aggressive; higher number = more conservative.

### 5.4 Status Monitor

The status card updates in real time:
- **Modified** — CAN frames modified since boot (> 0 means the device is working)
- **Received** — total CAN frames received
- **Errors** — CAN bus error count (should be 0)
- **Uptime** — time since last boot
- **CAN Bus** — OK means successfully connected to the vehicle CAN bus
- **FSD Triggered** — Yes means FSD has been activated

---

## Step 6: OTA Firmware Update

After modifying and rebuilding the firmware, you can update wirelessly — no USB needed:

1. Build in VS Code (click **Build**).
2. Locate the output file at `.pio/build/esp32/firmware.bin`.
3. In the web panel, find the **OTA Update** card, click **Choose File**, select `firmware.bin`.
4. Click **Upload Firmware** and wait for the progress bar. The device restarts automatically.

---

## FAQ

**Q: Browser can't open 192.168.4.1 after connecting to WiFi?**
> Some phones automatically disconnect from WiFi networks with no internet. In your phone's WiFi settings, find the hotspot and disable "Auto-switch to better network" or "Smart WiFi".

**Q: CAN Bus shows ERROR?**
> Check that CANH/CANL wiring is correct and that the vehicle is powered on (at least in standby). The CAN bus only carries signals when the vehicle is on.

**Q: FSD Triggered shows No?**
> Confirm that "Traffic Light and Stop Sign Control" is enabled under **Controls → Autopilot** in the car. This is the trigger condition.

**Q: Do I need to remove the 120Ω termination resistor from the CAN module?**
> Depends on your module:
> - **SN65HVD230** (recommended): **No action needed** — this module has no built-in termination resistor.
> - **MCP2515 blue module**: **Must remove** the R1/R2 resistors or cut the J1 jumper. Tesla's CAN bus already has built-in termination at both ends; adding another degrades signal quality and causes communication failures.

**Q: Modified count stays at 0, device has no effect? (China, Japan, etc.)**
> Enable **China Mode 🇨🇳** in the control panel. In these regions the car UI hides the "Traffic Light and Stop Sign Control" option due to a regional restriction, so the trigger bit (`UI_fsdStopsControlEnabled`) is always 0 and the mod never fires. China Mode bypasses this check.

**Q: Device appears to work but FSD still doesn't activate?**
> Check that your Tesla account has a valid FSD authorization (purchased or subscribed). The device operates at the CAN level only and cannot bypass Tesla's server-side authorization.

**Q: Speed profile setting has no effect?**
> Make sure **Profile Source** is set to **Manual**. If set to Auto (Stalk), the stalk position overrides the dropdown.

**Q: Flash error "A fatal error occurred: Failed to connect to ESP32"?**
> Hold the **BOOT** button on the dev board while flashing. Release it once the upload progress bar appears.

---

## Project Structure (for developers)

```
src/
  main.cpp              ← Entry point: WiFi AP + web server + CAN task (dual-core)
include/
  handlers.h            ← CAN frame processing (Legacy / HW3 / HW4)
  can_helpers.h         ← Bit manipulation helpers
  can_frame_types.h     ← CAN frame data structures
  web_ui.h              ← Embedded web UI (HTML/CSS/JS)
  drivers/
    can_driver.h        ← Driver abstraction base class
    twai_driver.h       ← ESP32 TWAI driver
platformio.ini          ← Build config (ESP32 + ESPAsyncWebServer)
```

---

## Credits

This project is built on top of the following open-source work. The core CAN frame handling logic is derived directly from the original project:

| Project | Link | License |
|---------|------|---------|
| tesla-open-can-mod | [gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod) | GPLv3 |

---

## License

GPLv3 — derived from [tesla-open-can-mod](https://gitlab.com/Tesla-OPEN-CAN-MOD/tesla-open-can-mod) (GPLv3).
