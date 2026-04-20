#pragma once

#define FIRMWARE_VERSION "1.4.18"

// Variant tag shown in the web UI alongside the version number.
// Empty string for baseline single-CAN builds.
#ifdef WIFI_BRIDGE_ENABLED
#  define FIRMWARE_VARIANT "Wi-Fi"
#else
#  define FIRMWARE_VARIANT ""
#endif
