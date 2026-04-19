#pragma once
// ── DNS whitelist/blacklist module ────────────────────────────────────────────
// Compiled only when WIFI_BRIDGE_ENABLED. Single-TU: handlers.h defines
// MOD_DNS_IMPLEMENTATION before including this so the definition compiles
// exactly once in main.cpp's TU.
//
// This header is ALSO included from IDF lwIP C code via lwip_hooks.h — from
// that side only the extern "C" declarations are visible (no Arduino/WiFi.h).
//
// Port of pudge9527/tesla-fsd-wifi-controller dns_whitelist.cpp +
// dns_ip_blocker.cpp merged into a single translation-unit header.

#ifdef WIFI_BRIDGE_ENABLED

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

// Return 1=allow, 0=drop, -1=fall through to default lwIP forwarding.
// Takes destination IP in NETWORK byte order (as lwIP stores ip4_addr.addr).
int dnsHookIp4CanForward(uint32_t destAddrNetOrder);

// Periodic service: keeps the blocklist IP cache fresh. Call from loop().
void dnsIpPolicyService(const char* allowlistRules,
                        const char* blocklistRules,
                        int rulesEnabled,
                        int upstreamReady);

// Record an IP just resolved for `domain` (called when a blocked-name query
// comes in so subsequent hardcoded-IP traffic to that host is dropped).
void dnsIpBlockerRememberDomain(const char* domain, int upstreamReady);

// Flush the IP cache (e.g. when rules change or WiFi drops).
void dnsIpBlockerClear(void);

// Observability: outDrops = cumulative packets dropped by the lwIP hook,
// outCacheCount = current number of blocked IPs cached,
// outCachePeak = historical high-water mark since boot or last clear.
// Pass NULL for any output to skip.
void dnsIpBlockerGetStats(uint32_t* outDrops, uint32_t* outCacheCount, uint32_t* outCachePeak);

#ifdef __cplusplus
}
#endif

// ── C++-side Arduino interface (only visible when compiled as C++) ────────────
#ifdef __cplusplus

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <cctype>
#include <cstring>

struct DNSFilterConfig {
    bool enabled = false;
    char allowlist[1024] = {};
    char blocklist[1024] = {};
};

static constexpr size_t kDnsBlockedDomainCapacity = 20;

struct DNSBlockedDomainStatEntry {
    char domain[128] = {};
    uint32_t count = 0;
    uint32_t lastBlockedAtUptimeSeconds = 0;
};

class DNSWhitelistServer {
  public:
    bool begin(uint16_t port = 53);
    void stop();
    void processNextRequest(const DNSFilterConfig& cfg, bool upstreamReady);
    size_t copyBlockedDomains(DNSBlockedDomainStatEntry* dest, size_t maxEntries, uint32_t& totalBlockedCount);
    void clearBlockedRequests();

  private:
    WiFiUDP udp_;
    WiFiUDP upstreamUdp_;
    bool started_ = false;
    DNSBlockedDomainStatEntry blockedDomains_[kDnsBlockedDomainCapacity];
    size_t blockedDomainCount_ = 0;
    uint32_t totalBlockedCount_ = 0;

    String normalizeDomain(const String& domain) const;
    bool parseDomainName(const uint8_t* query, size_t length, size_t& offset, String& domain) const;
    bool skipDomainName(const uint8_t* packet, size_t length, size_t& offset) const;
    bool hasDomainRules(const char* rules) const;
    bool listContainsDomain(const char* rules, const String& domain) const;
    bool isAllowedDomain(const DNSFilterConfig& cfg, const String& domain) const;
    void logBlockedRequest(const String& domain);
    bool forwardUpstreamQuery(const uint8_t* query, size_t queryLength, uint8_t* response, size_t responseCapacity, size_t& responseLength);
    void cacheAllowedAddressesFromResponse(const String& domain, const uint8_t* response, size_t responseLength);
    void sendBlockedResponse(const uint8_t* query, size_t questionEnd, uint16_t requestFlags, uint16_t qType);
    void sendErrorResponse(const uint8_t* query, size_t questionEnd, uint16_t requestFlags, uint16_t rcode);
    void sendNoAnswerResponse(const uint8_t* query, size_t questionEnd, uint16_t requestFlags);
    void sendIPv4Answer(const uint8_t* query, size_t questionEnd, uint16_t requestFlags, const IPAddress& ip);
};

#endif // __cplusplus

// ── Implementation ────────────────────────────────────────────────────────────
// Defined once in the main TU (handlers.h sets MOD_DNS_IMPLEMENTATION before
// including this header). All internal helpers are `static` to keep linkage
// private to this TU.
#ifdef MOD_DNS_IMPLEMENTATION

namespace {

// ── DNS packet helpers ───────────────────────────────────────────────────────
constexpr uint16_t kDnsHeaderSize = 12;
constexpr uint16_t kDnsTypeA = 1;
constexpr uint16_t kDnsTypeAAAA = 28;
constexpr uint16_t kDnsClassIN = 1;
constexpr uint32_t kDnsTtlSeconds = 60;
constexpr uint32_t kDnsForwardTimeoutMs = 1200;

portMUX_TYPE gDnsBlockedLogMux = portMUX_INITIALIZER_UNLOCKED;

uint16_t readU16(const uint8_t* ptr) {
    return static_cast<uint16_t>((ptr[0] << 8) | ptr[1]);
}

void writeU16(uint8_t* ptr, uint16_t value) {
    ptr[0] = static_cast<uint8_t>((value >> 8) & 0xFF);
    ptr[1] = static_cast<uint8_t>(value & 0xFF);
}

void writeU32(uint8_t* ptr, uint32_t value) {
    ptr[0] = static_cast<uint8_t>((value >> 24) & 0xFF);
    ptr[1] = static_cast<uint8_t>((value >> 16) & 0xFF);
    ptr[2] = static_cast<uint8_t>((value >> 8) & 0xFF);
    ptr[3] = static_cast<uint8_t>(value & 0xFF);
}

bool domainMatchesRule(const String& domain, const String& rule) {
    if (rule.isEmpty()) return false;
    if (domain == rule) return true;
    if (domain.length() <= rule.length()) return false;

    size_t offset = domain.length() - rule.length();
    return domain.endsWith(rule) && domain[offset - 1] == '.';
}

// ── IP blocker state (shared with lwIP hook) ─────────────────────────────────
constexpr size_t kBlockedIpCapacity = 256;
constexpr uint32_t kIpEntryTtlSeconds = 3600;
constexpr uint32_t kRuleResolveIntervalSeconds = 15;
constexpr uint32_t kFullRefreshIntervalSeconds = 300;

struct CachedIpEntry {
    uint32_t ipHostOrder = 0;
    char domain[128] = {};
    uint32_t lastSeenAtSeconds = 0;
};

portMUX_TYPE gDnsIpPolicyMux = portMUX_INITIALIZER_UNLOCKED;
CachedIpEntry gBlockedIps[kBlockedIpCapacity];
size_t gBlockedIpCount = 0;
char gBlocklistSnapshot[1024] = {};
uint32_t gLastBlockResolveAtSeconds = 0;
uint32_t gLastFullRefreshAtSeconds = 0;
size_t gNextBlockResolveRuleIndex = 0;
volatile uint32_t gIpForwardDropCount = 0;
volatile uint32_t gIpCachePeakCount = 0;

String normalizeDomainCstr(const char* domain) {
    String normalized = domain == nullptr ? "" : String(domain);
    normalized.trim();
    normalized.toLowerCase();
    while (normalized.endsWith(".")) {
        normalized.remove(normalized.length() - 1);
    }
    return normalized;
}

bool hasRules(const char* rules) {
    if (rules == nullptr) return false;
    const char* cursor = rules;
    while (*cursor) {
        while (*cursor && (isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',' || *cursor == ';')) {
            ++cursor;
        }
        if (!*cursor) break;
        return true;
    }
    return false;
}

bool snapshotMatches(const char* rules, const char* snapshot) {
    String current = rules == nullptr ? "" : String(rules);
    return current.equals(snapshot == nullptr ? "" : String(snapshot));
}

void clearEntries(CachedIpEntry* entries, size_t capacity, size_t& count) {
    for (size_t i = 0; i < capacity; ++i) {
        entries[i] = CachedIpEntry{};
    }
    count = 0;
}

void pruneEntriesLocked(CachedIpEntry* entries, size_t capacity, size_t& count, uint32_t nowSeconds) {
    size_t writeIndex = 0;
    for (size_t i = 0; i < count; ++i) {
        if (nowSeconds - entries[i].lastSeenAtSeconds <= kIpEntryTtlSeconds) {
            if (writeIndex != i) {
                entries[writeIndex] = entries[i];
            }
            ++writeIndex;
        }
    }
    for (size_t i = writeIndex; i < capacity; ++i) {
        entries[i] = CachedIpEntry{};
    }
    count = writeIndex;
}

void addOrRefreshEntryLocked(CachedIpEntry* entries, size_t capacity, size_t& count,
                             const String& domain, uint32_t ipHostOrder, uint32_t nowSeconds) {
    if (ipHostOrder == 0) return;

    for (size_t i = 0; i < count; ++i) {
        if (entries[i].ipHostOrder == ipHostOrder) {
            entries[i].lastSeenAtSeconds = nowSeconds;
            if (domain.length() > 0) {
                domain.toCharArray(entries[i].domain, sizeof(entries[i].domain));
            }
            return;
        }
    }

    CachedIpEntry entry = {};
    entry.ipHostOrder = ipHostOrder;
    entry.lastSeenAtSeconds = nowSeconds;
    domain.toCharArray(entry.domain, sizeof(entry.domain));

    if (count < capacity) {
        entries[count++] = entry;
        if (count > gIpCachePeakCount) gIpCachePeakCount = count;
        return;
    }
    gIpCachePeakCount = capacity;  // cache is full → peak at capacity

    size_t oldestIndex = 0;
    for (size_t i = 1; i < count; ++i) {
        if (entries[i].lastSeenAtSeconds < entries[oldestIndex].lastSeenAtSeconds) {
            oldestIndex = i;
        }
    }
    entries[oldestIndex] = entry;
}

bool getRuleByIndex(const char* rules, size_t targetIndex, String& outRule) {
    outRule = "";
    if (rules == nullptr) return false;

    size_t currentIndex = 0;
    const char* cursor = rules;
    while (*cursor) {
        while (*cursor && (isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',' || *cursor == ';')) {
            ++cursor;
        }
        if (!*cursor) break;

        const char* start = cursor;
        while (*cursor && !(isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',' || *cursor == ';')) {
            ++cursor;
        }

        if (currentIndex == targetIndex) {
            String rule(start, cursor - start);
            outRule = normalizeDomainCstr(rule.c_str());
            return !outRule.isEmpty();
        }
        ++currentIndex;
    }
    return false;
}

size_t countRules(const char* rules) {
    size_t count = 0;
    String rule;
    while (getRuleByIndex(rules, count, rule)) {
        ++count;
    }
    return count;
}

uint32_t ipAddressToHostOrder(const IPAddress& ip) {
    return
        (static_cast<uint32_t>(ip[0]) << 24) |
        (static_cast<uint32_t>(ip[1]) << 16) |
        (static_cast<uint32_t>(ip[2]) << 8) |
        static_cast<uint32_t>(ip[3]);
}

void rememberResolvedIp(const String& domain, uint32_t ipHostOrder) {
    uint32_t nowSeconds = millis() / 1000;

    portENTER_CRITICAL(&gDnsIpPolicyMux);
    pruneEntriesLocked(gBlockedIps, kBlockedIpCapacity, gBlockedIpCount, nowSeconds);
    addOrRefreshEntryLocked(gBlockedIps, kBlockedIpCapacity, gBlockedIpCount, domain, ipHostOrder, nowSeconds);
    portEXIT_CRITICAL(&gDnsIpPolicyMux);
}

void resolveAndRememberDomain(const String& domain) {
    if (domain.isEmpty()) return;

    IPAddress resolved;
    if (WiFi.hostByName(domain.c_str(), resolved) == 1) {
        rememberResolvedIp(domain, ipAddressToHostOrder(resolved));
    }
}

void resolveAllRules(const char* rules) {
    size_t ruleCount = countRules(rules);
    for (size_t i = 0; i < ruleCount; ++i) {
        String rule;
        if (getRuleByIndex(rules, i, rule)) {
            resolveAndRememberDomain(rule);
        }
    }
}

void resetStateLocked(const char* blocklistRules, uint32_t nowSeconds) {
    clearEntries(gBlockedIps, kBlockedIpCapacity, gBlockedIpCount);

    memset(gBlocklistSnapshot, 0, sizeof(gBlocklistSnapshot));
    if (blocklistRules != nullptr) {
        String(blocklistRules).toCharArray(gBlocklistSnapshot, sizeof(gBlocklistSnapshot));
    }

    gLastBlockResolveAtSeconds = 0;
    gLastFullRefreshAtSeconds = nowSeconds;
    gNextBlockResolveRuleIndex = 0;
}

bool containsIpLocked(const CachedIpEntry* entries, size_t count, uint32_t ipHostOrder) {
    for (size_t i = 0; i < count; ++i) {
        if (entries[i].ipHostOrder == ipHostOrder) return true;
    }
    return false;
}

}  // namespace

// ── DNSWhitelistServer member definitions ────────────────────────────────────
bool DNSWhitelistServer::begin(uint16_t port) {
    stop();
    started_ = udp_.begin(port) == 1;
    return started_;
}

void DNSWhitelistServer::stop() {
    udp_.stop();
    started_ = false;
}

String DNSWhitelistServer::normalizeDomain(const String& domain) const {
    String normalized = domain;
    normalized.trim();
    normalized.toLowerCase();
    while (normalized.endsWith(".")) {
        normalized.remove(normalized.length() - 1);
    }
    return normalized;
}

bool DNSWhitelistServer::parseDomainName(const uint8_t* query, size_t length, size_t& offset, String& domain) const {
    domain = "";
    bool first = true;

    while (offset < length) {
        uint8_t labelLength = query[offset++];
        if (labelLength == 0) return true;
        if ((labelLength & 0xC0) != 0 || offset + labelLength > length) return false;

        if (!first) domain += '.';
        for (uint8_t i = 0; i < labelLength; ++i) {
            domain += static_cast<char>(query[offset++]);
        }
        first = false;
    }
    return false;
}

bool DNSWhitelistServer::skipDomainName(const uint8_t* packet, size_t length, size_t& offset) const {
    while (offset < length) {
        uint8_t labelLength = packet[offset++];
        if (labelLength == 0) return true;
        if ((labelLength & 0xC0) == 0xC0) {
            return offset < length;
        }
        if ((labelLength & 0xC0) != 0 || offset + labelLength > length) return false;
        offset += labelLength;
    }
    return false;
}

bool DNSWhitelistServer::hasDomainRules(const char* rules) const {
    return hasRules(rules);
}

bool DNSWhitelistServer::listContainsDomain(const char* rules, const String& domain) const {
    if (rules == nullptr || domain.isEmpty()) return false;

    const char* cursor = rules;
    while (*cursor) {
        while (*cursor && (isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',' || *cursor == ';')) {
            ++cursor;
        }
        if (!*cursor) break;

        const char* start = cursor;
        while (*cursor && !(isspace(static_cast<unsigned char>(*cursor)) || *cursor == ',' || *cursor == ';')) {
            ++cursor;
        }

        String rule(start, cursor - start);
        rule = normalizeDomain(rule);
        if (domainMatchesRule(domain, rule)) return true;
    }
    return false;
}

// Longest matching rule length (0 = no match). Used for dnsmasq-style
// "more specific rule wins" semantics between allow and block lists.
static size_t longestRuleMatchLen(const char* rules, const String& domain) {
    if (rules == nullptr || domain.isEmpty()) return 0;
    size_t best = 0;
    size_t idx = 0;
    String rule;
    while (getRuleByIndex(rules, idx, rule)) {
        if (!rule.isEmpty() && domainMatchesRule(domain, rule)) {
            if (rule.length() > best) best = rule.length();
        }
        ++idx;
    }
    return best;
}

bool DNSWhitelistServer::isAllowedDomain(const DNSFilterConfig& cfg, const String& domain) const {
    if (!cfg.enabled) return true;
    if (domain.isEmpty()) return false;

    size_t blockLen = longestRuleMatchLen(cfg.blocklist, domain);
    size_t allowLen = longestRuleMatchLen(cfg.allowlist, domain);

    // dnsmasq semantics: more specific (longer) rule wins.
    if (allowLen > 0 && allowLen >= blockLen) return true;
    if (blockLen > 0) return false;
    if (!hasDomainRules(cfg.allowlist)) return true;
    return false;
}

void DNSWhitelistServer::logBlockedRequest(const String& domain) {
    String normalizedDomain = normalizeDomain(domain);
    if (normalizedDomain.isEmpty()) return;

    uint32_t blockedAtUptimeSeconds = millis() / 1000;

    portENTER_CRITICAL(&gDnsBlockedLogMux);
    bool found = false;
    for (size_t i = 0; i < blockedDomainCount_; ++i) {
        if (normalizedDomain.equals(blockedDomains_[i].domain)) {
            ++blockedDomains_[i].count;
            blockedDomains_[i].lastBlockedAtUptimeSeconds = blockedAtUptimeSeconds;
            found = true;
            break;
        }
    }

    if (!found && blockedDomainCount_ < kDnsBlockedDomainCapacity) {
        DNSBlockedDomainStatEntry entry = {};
        normalizedDomain.toCharArray(entry.domain, sizeof(entry.domain));
        entry.count = 1;
        entry.lastBlockedAtUptimeSeconds = blockedAtUptimeSeconds;
        blockedDomains_[blockedDomainCount_++] = entry;
    }
    ++totalBlockedCount_;
    portEXIT_CRITICAL(&gDnsBlockedLogMux);
}

bool DNSWhitelistServer::forwardUpstreamQuery(const uint8_t* query, size_t queryLength, uint8_t* response, size_t responseCapacity, size_t& responseLength) {
    responseLength = 0;
    if (query == nullptr || response == nullptr || queryLength == 0 || responseCapacity == 0) return false;

    IPAddress dnsServerIP = WiFi.dnsIP(0);
    if (dnsServerIP == IPAddress(0, 0, 0, 0)) return false;

    upstreamUdp_.stop();
    if (upstreamUdp_.begin(0) != 1) return false;

    bool success = false;
    do {
        if (!upstreamUdp_.beginPacket(dnsServerIP, 53)) break;
        if (upstreamUdp_.write(query, queryLength) != queryLength) break;
        if (!upstreamUdp_.endPacket()) break;

        uint32_t start = millis();
        while (millis() - start < kDnsForwardTimeoutMs) {
            int packetSize = upstreamUdp_.parsePacket();
            if (packetSize <= 0) {
                delay(5);
                continue;
            }
            if (packetSize > static_cast<int>(responseCapacity)) break;
            int read = upstreamUdp_.read(response, packetSize);
            if (read > 0) {
                responseLength = static_cast<size_t>(read);
                success = true;
            }
            break;
        }
    } while (false);

    upstreamUdp_.stop();
    return success;
}

void DNSWhitelistServer::cacheAllowedAddressesFromResponse(const String& domain, const uint8_t* response, size_t responseLength) {
    if (response == nullptr || responseLength < kDnsHeaderSize) return;

    uint16_t qdCount = readU16(response + 4);
    uint16_t anCount = readU16(response + 6);
    size_t offset = kDnsHeaderSize;

    for (uint16_t i = 0; i < qdCount; ++i) {
        if (!skipDomainName(response, responseLength, offset) || offset + 4 > responseLength) return;
        offset += 4;
    }

    for (uint16_t i = 0; i < anCount; ++i) {
        if (!skipDomainName(response, responseLength, offset) || offset + 10 > responseLength) return;

        uint16_t rrType = readU16(response + offset);
        uint16_t rrClass = readU16(response + offset + 2);
        uint16_t rdLength = readU16(response + offset + 8);
        offset += 10;

        if (offset + rdLength > responseLength) return;

        if (rrType == kDnsTypeA && rrClass == kDnsClassIN && rdLength == 4) {
            (void)domain;
        }
        offset += rdLength;
    }
}

size_t DNSWhitelistServer::copyBlockedDomains(DNSBlockedDomainStatEntry* dest, size_t maxEntries, uint32_t& totalBlockedCount) {
    totalBlockedCount = 0;
    if (dest == nullptr || maxEntries == 0) return 0;

    portENTER_CRITICAL(&gDnsBlockedLogMux);
    totalBlockedCount = totalBlockedCount_;
    size_t count = blockedDomainCount_ < maxEntries ? blockedDomainCount_ : maxEntries;

    for (size_t i = 0; i < count; ++i) {
        dest[i] = blockedDomains_[i];
    }
    portEXIT_CRITICAL(&gDnsBlockedLogMux);

    for (size_t i = 0; i < count; ++i) {
        size_t bestIndex = i;
        for (size_t j = i + 1; j < count; ++j) {
            if (dest[j].count < dest[bestIndex].count ||
                (dest[j].count == dest[bestIndex].count && strcmp(dest[j].domain, dest[bestIndex].domain) < 0)) {
                bestIndex = j;
            }
        }
        if (bestIndex != i) {
            DNSBlockedDomainStatEntry tmp = dest[i];
            dest[i] = dest[bestIndex];
            dest[bestIndex] = tmp;
        }
    }
    return count;
}

void DNSWhitelistServer::clearBlockedRequests() {
    portENTER_CRITICAL(&gDnsBlockedLogMux);
    for (size_t i = 0; i < kDnsBlockedDomainCapacity; ++i) {
        blockedDomains_[i] = DNSBlockedDomainStatEntry{};
    }
    blockedDomainCount_ = 0;
    totalBlockedCount_ = 0;
    portEXIT_CRITICAL(&gDnsBlockedLogMux);
}

void DNSWhitelistServer::sendErrorResponse(const uint8_t* query, size_t questionEnd, uint16_t requestFlags, uint16_t rcode) {
    if (!started_) return;

    uint8_t response[512] = {};
    if (questionEnd > sizeof(response)) return;

    memcpy(response, query, questionEnd);
    uint16_t responseFlags = static_cast<uint16_t>(0x8000 | 0x0080 | (requestFlags & 0x0100) | (rcode & 0x000F));
    writeU16(response + 2, responseFlags);
    writeU16(response + 4, questionEnd > kDnsHeaderSize ? 1 : 0);
    writeU16(response + 6, 0);
    writeU16(response + 8, 0);
    writeU16(response + 10, 0);

    udp_.beginPacket(udp_.remoteIP(), udp_.remotePort());
    udp_.write(response, questionEnd);
    udp_.endPacket();
}

void DNSWhitelistServer::sendNoAnswerResponse(const uint8_t* query, size_t questionEnd, uint16_t requestFlags) {
    sendErrorResponse(query, questionEnd, requestFlags, 0);
}

void DNSWhitelistServer::sendBlockedResponse(const uint8_t* query, size_t questionEnd, uint16_t requestFlags, uint16_t qType) {
    if (qType == kDnsTypeA) {
        sendIPv4Answer(query, questionEnd, requestFlags, IPAddress(0, 0, 0, 0));
        return;
    }
    // Return a successful empty answer instead of REFUSED to reduce client
    // fallback to alternate DNS paths when a domain is intentionally blocked.
    sendNoAnswerResponse(query, questionEnd, requestFlags);
}

void DNSWhitelistServer::sendIPv4Answer(const uint8_t* query, size_t questionEnd, uint16_t requestFlags, const IPAddress& ip) {
    if (!started_) return;

    uint8_t response[512] = {};
    const size_t answerSize = 16;
    const size_t responseSize = questionEnd + answerSize;
    if (responseSize > sizeof(response)) return;

    memcpy(response, query, questionEnd);
    uint16_t responseFlags = static_cast<uint16_t>(0x8000 | 0x0080 | (requestFlags & 0x0100));
    writeU16(response + 2, responseFlags);
    writeU16(response + 4, 1);
    writeU16(response + 6, 1);
    writeU16(response + 8, 0);
    writeU16(response + 10, 0);

    size_t offset = questionEnd;
    response[offset++] = 0xC0;
    response[offset++] = 0x0C;
    writeU16(response + offset, kDnsTypeA);
    offset += 2;
    writeU16(response + offset, kDnsClassIN);
    offset += 2;
    writeU32(response + offset, kDnsTtlSeconds);
    offset += 4;
    writeU16(response + offset, 4);
    offset += 2;
    response[offset++] = ip[0];
    response[offset++] = ip[1];
    response[offset++] = ip[2];
    response[offset++] = ip[3];

    udp_.beginPacket(udp_.remoteIP(), udp_.remotePort());
    udp_.write(response, responseSize);
    udp_.endPacket();
}

void DNSWhitelistServer::processNextRequest(const DNSFilterConfig& cfg, bool upstreamReady) {
    if (!started_) return;

    int packetSize = udp_.parsePacket();
    if (packetSize <= 0 || packetSize > 512) return;

    uint8_t query[512] = {};
    int read = udp_.read(query, packetSize);
    if (read < static_cast<int>(kDnsHeaderSize)) return;

    uint16_t requestFlags = readU16(query + 2);
    uint16_t qdCount = readU16(query + 4);
    uint16_t qr = requestFlags & 0x8000;
    uint16_t opcode = (requestFlags >> 11) & 0x0F;

    if (qr != 0 || opcode != 0 || qdCount != 1) {
        sendErrorResponse(query, kDnsHeaderSize, requestFlags, 4);
        return;
    }

    size_t offset = kDnsHeaderSize;
    String domain;
    if (!parseDomainName(query, read, offset, domain) || offset + 4 > static_cast<size_t>(read)) {
        sendErrorResponse(query, kDnsHeaderSize, requestFlags, 1);
        return;
    }

    const size_t questionEnd = offset + 4;
    domain = normalizeDomain(domain);

    uint16_t qType = readU16(query + offset);
    uint16_t qClass = readU16(query + offset + 2);

    if (qClass != kDnsClassIN) {
        sendErrorResponse(query, questionEnd, requestFlags, 5);
        return;
    }

    if (!isAllowedDomain(cfg, domain)) {
        logBlockedRequest(domain);
        dnsIpBlockerRememberDomain(domain.c_str(), upstreamReady ? 1 : 0);
        sendBlockedResponse(query, questionEnd, requestFlags, qType);
        return;
    }

    if (!upstreamReady) {
        sendErrorResponse(query, questionEnd, requestFlags, 2);
        return;
    }

    uint8_t upstreamResponse[512] = {};
    size_t upstreamResponseLength = 0;
    if (forwardUpstreamQuery(query, read, upstreamResponse, sizeof(upstreamResponse), upstreamResponseLength)) {
        cacheAllowedAddressesFromResponse(domain, upstreamResponse, upstreamResponseLength);
        udp_.beginPacket(udp_.remoteIP(), udp_.remotePort());
        udp_.write(upstreamResponse, upstreamResponseLength);
        udp_.endPacket();
        return;
    }

    if (qType != kDnsTypeA) {
        sendNoAnswerResponse(query, questionEnd, requestFlags);
        return;
    }

    IPAddress resolved;
    if (WiFi.hostByName(domain.c_str(), resolved) == 1) {
        sendIPv4Answer(query, questionEnd, requestFlags, resolved);
    } else {
        sendErrorResponse(query, questionEnd, requestFlags, 3);
    }
}

// ── C-linkage entry points (visible to lwIP hook) ────────────────────────────
extern "C" void dnsIpPolicyService(const char* allowlistRules, const char* blocklistRules, int rulesEnabled, int upstreamReady) {
    uint32_t nowSeconds = millis() / 1000;
    bool blocklistHasRules = hasRules(blocklistRules);
    bool shouldPrimeBlocklist = false;
    bool didReset = false;
    (void)allowlistRules;

    portENTER_CRITICAL(&gDnsIpPolicyMux);
    bool shouldReset =
        !rulesEnabled ||
        !snapshotMatches(blocklistRules, gBlocklistSnapshot);

    if (shouldReset) {
        resetStateLocked(blocklistRules, nowSeconds);
        didReset = true;
    } else {
        pruneEntriesLocked(gBlockedIps, kBlockedIpCapacity, gBlockedIpCount, nowSeconds);
        shouldPrimeBlocklist = blocklistHasRules && gBlockedIpCount == 0;
    }
    portEXIT_CRITICAL(&gDnsIpPolicyMux);

    if (!rulesEnabled || !upstreamReady) return;

    if (didReset || shouldPrimeBlocklist) {
        if (blocklistHasRules) {
            resolveAllRules(blocklistRules);
            gLastBlockResolveAtSeconds = nowSeconds;
        }
        if (didReset) {
            gNextBlockResolveRuleIndex = 0;
        }
        if (didReset || shouldPrimeBlocklist) {
            return;
        }
    }

    if (nowSeconds - gLastFullRefreshAtSeconds >= kFullRefreshIntervalSeconds) {
        portENTER_CRITICAL(&gDnsIpPolicyMux);
        clearEntries(gBlockedIps, kBlockedIpCapacity, gBlockedIpCount);
        gLastFullRefreshAtSeconds = nowSeconds;
        gNextBlockResolveRuleIndex = 0;
        portEXIT_CRITICAL(&gDnsIpPolicyMux);
    }

    if (blocklistHasRules && nowSeconds - gLastBlockResolveAtSeconds >= kRuleResolveIntervalSeconds) {
        size_t ruleCount = countRules(blocklistRules);
        if (ruleCount > 0) {
            String rule;
            if (!getRuleByIndex(blocklistRules, gNextBlockResolveRuleIndex, rule)) {
                gNextBlockResolveRuleIndex = 0;
                getRuleByIndex(blocklistRules, gNextBlockResolveRuleIndex, rule);
            }
            resolveAndRememberDomain(rule);
            gLastBlockResolveAtSeconds = nowSeconds;
            gNextBlockResolveRuleIndex = (gNextBlockResolveRuleIndex + 1) % ruleCount;
        }
    }
}

extern "C" void dnsIpBlockerRememberDomain(const char* domain, int upstreamReady) {
    if (!upstreamReady) return;
    String normalized = normalizeDomainCstr(domain);
    if (normalized.isEmpty()) return;
    resolveAndRememberDomain(normalized);
}

extern "C" void dnsIpBlockerClear(void) {
    portENTER_CRITICAL(&gDnsIpPolicyMux);
    clearEntries(gBlockedIps, kBlockedIpCapacity, gBlockedIpCount);
    gIpForwardDropCount = 0;
    gIpCachePeakCount   = 0;
    portEXIT_CRITICAL(&gDnsIpPolicyMux);
}

extern "C" int dnsHookIp4CanForward(uint32_t destAddrNetOrder) {
    // lwIP passes ip4_addr.addr which is stored in NETWORK byte order on ESP32
    // (little-endian host). Our cache stores in HOST order — byte-swap to match.
    uint32_t destAddrHostOrder =
        ((destAddrNetOrder & 0xFF000000u) >> 24) |
        ((destAddrNetOrder & 0x00FF0000u) >> 8)  |
        ((destAddrNetOrder & 0x0000FF00u) << 8)  |
        ((destAddrNetOrder & 0x000000FFu) << 24);
    uint32_t nowSeconds = millis() / 1000;

    portENTER_CRITICAL(&gDnsIpPolicyMux);
    pruneEntriesLocked(gBlockedIps, kBlockedIpCapacity, gBlockedIpCount, nowSeconds);

    if (containsIpLocked(gBlockedIps, gBlockedIpCount, destAddrHostOrder)) {
        gIpForwardDropCount++;
        portEXIT_CRITICAL(&gDnsIpPolicyMux);
        return 0;  // drop
    }
    portEXIT_CRITICAL(&gDnsIpPolicyMux);
    return -1;  // fall through to default lwIP forwarding
}

extern "C" void dnsIpBlockerGetStats(uint32_t* outDrops, uint32_t* outCacheCount, uint32_t* outCachePeak) {
    portENTER_CRITICAL(&gDnsIpPolicyMux);
    if (outDrops)      *outDrops      = gIpForwardDropCount;
    if (outCacheCount) *outCacheCount = (uint32_t)gBlockedIpCount;
    if (outCachePeak)  *outCachePeak  = gIpCachePeakCount;
    portEXIT_CRITICAL(&gDnsIpPolicyMux);
}

#endif // MOD_DNS_IMPLEMENTATION

#endif // WIFI_BRIDGE_ENABLED
