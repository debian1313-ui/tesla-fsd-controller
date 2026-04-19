#pragma once
// LWIP IP-layer hook — only active when WIFI_BRIDGE_ENABLED
// Used to gate forwarded packets by destination IP (DNS whitelist pre-resolved IPs).
// See mod_dns.h for dnsHookIp4CanForward() implementation.

#ifdef WIFI_BRIDGE_ENABLED
#include "mod_dns.h"
#define LWIP_HOOK_IP4_CANFORWARD(p, dest) dnsHookIp4CanForward(dest)
#endif
