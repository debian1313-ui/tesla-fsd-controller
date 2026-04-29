// tesla-fsd-controller anonymous usage counter.
//
//   POST /ping  {id, version, env, carVer?}  — records device for today (id = sha256(MAC) hex, 16-64 chars)
//                                              carVer is optional (Tesla MCU software version, user-entered)
//   GET  /stats                      — returns {today, yesterday, month, year, total, date, byEnv, byVersion, byCountry, byCarVer}
//
// Storage — per-device dedup markers + pre-aggregated rollup counters.
// /stats reads 5 counter keys (no list ops). /ping updates counters in place.
//   seen:YYYY-MM-DD:<id> → "1"               TTL 3d   (daily dedup)
//   mo:YYYY-MM:<id>      → "1"               TTL 62d  (MAU dedup)
//   yr:YYYY:<id>         → "1"               TTL 400d (YAU dedup)
//   ever:<id>            → "1"               no TTL   (all-time dedup)
//   agg:YYYY-MM-DD       → {c,e,v,co,cv}     TTL 3d   (daily rollup — byEnv/byVersion/byCountry/byCarVer)
//   agg:mo:YYYY-MM       → "<int>"        TTL 62d  (MAU count)
//   agg:yr:YYYY          → "<int>"        TTL 400d (YAU count)
//   agg:ever             → "<int>"        no TTL   (all-time count)
//
// Concurrency: two concurrent /ping for distinct devices can race the
// read-modify-write on agg:* and lose one increment. Acceptable for anon
// stats. If tighter accuracy is needed later, move counters to a Durable
// Object.

// /ping and /stats are called by the dashboard from a browser, so they need
// permissive CORS. /diag/* is firmware-only (esp_http_client doesn't honour
// CORS) and maintainer-only; no browser site has a legitimate reason to call
// it cross-origin, so we don't advertise it.
const CORS = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
  "Access-Control-Allow-Headers": "Content-Type, Authorization, X-Diag-Key",
};
const NO_CORS = {};

// Edge cache for /stats. Kept long enough that a steady dashboard poll
// (5-min) mostly hits cache — keeps us well under the 1k list-ops/day free
// tier even if listing is reintroduced later.
const STATS_CACHE_TTL_SEC = 600;

const TTL_DAY = 3 * 24 * 3600;
const TTL_MONTH = 62 * 24 * 3600;
const TTL_YEAR = 400 * 24 * 3600;
const TTL_DIAG = 30 * 24 * 3600;            // diagnostic bundle retention
const DIAG_RATE_LIMIT_SEC = 60;             // 1 upload per device per minute
const DIAG_IP_RATE_LIMIT_SEC = 60;          // also cap per source IP (caller-controlled `id` alone is bypass-trivial). KV expirationTtl minimum is 60.
const DIAG_MAX_BYTES = 65536;               // refuse oversized payloads
// 32-char alphabet, no 0/O/1/I to avoid user confusion when typing IDs.
const DIAG_ID_ALPHABET = "ABCDEFGHJKLMNPQRSTUVWXYZ23456789";
const DIAG_ID_LEN = 6;                      // 32^6 ≈ 1.07B keyspace

export default {
  async fetch(request, env, ctx) {
    if (request.method === "OPTIONS") return new Response(null, { headers: CORS });

    const url = new URL(request.url);

    if (request.method === "POST" && url.pathname === "/ping") {
      return handlePing(request, env);
    }
    if (request.method === "GET" && url.pathname === "/stats") {
      return handleStatsCached(request, env, ctx);
    }
    if (request.method === "POST" && url.pathname === "/diag") {
      return handleDiagUpload(request, env);
    }
    if (request.method === "GET" && url.pathname === "/diag/list") {
      return handleDiagList(request, env, url);
    }
    if (request.method === "GET" && url.pathname.startsWith("/diag/")) {
      return handleDiagFetch(request, env, url);
    }
    return new Response(
      "tesla-counter\n\nPOST /ping {id, version, env}\nGET /stats\nPOST /diag {id, version, env, ...}\n",
      { headers: CORS }
    );
  },
};

// ── /diag — diagnostic bundle upload + admin-only retrieval ─────────────────
// POST /diag → store payload, return {id: "AB12CD"} (6-char short id).
// GET  /diag/<id>   (Authorization: Bearer <DIAG_KEY>) → stored payload.
// GET  /diag/list   (Authorization: Bearer <DIAG_KEY>) → enumerate uploads.
//
// The maintainer secret travels in a header (not the URL) so it never lands
// in Cloudflare access logs, browser history, or Referer headers.
//
// Privacy model: short id is intentionally enumerable-resistant via rate
// limiting + 32^6 keyspace, but the real protection is the GET secret. Users
// pasting their id in a public GitHub issue do NOT expose payload contents to
// readers — only the project maintainer (who holds DIAG_KEY) can read them.
//
// The id stays short for usability (typeable / dictate-able). Long random
// tokens were considered but make it harder for users to share over phone /
// Telegram. The maintainer-secret model gets us both.
async function handleDiagUpload(request, env) {
  let raw;
  try { raw = await request.text(); } catch { return txt("read failed", 400); }
  if (raw.length > DIAG_MAX_BYTES) return txt("payload too large", 413);
  let body;
  try { body = JSON.parse(raw); } catch { return txt("bad json", 400); }
  const { id, version, env: fwEnv } = body || {};
  if (typeof id !== "string" || !/^[a-f0-9]{16,64}$/.test(id)) return txt("bad id", 400);
  if (typeof version !== "string" || !/^[0-9a-zA-Z._-]{1,16}$/.test(version)) return txt("bad version", 400);
  if (typeof fwEnv !== "string" || !/^[a-z0-9_-]{1,32}$/.test(fwEnv)) return txt("bad env", 400);

  const ip = request.headers.get("cf-connecting-ip") || "?";
  const rateKey   = `rate:diag:${id}`;
  const ipRateKey = `rate:diag:ip:${ip}`;
  // Two limits: per-id (caller-controlled, easy to rotate) AND per-IP (the
  // attacker actually has to pay for new addresses). Either one trips → 429.
  const [rIdHit, rIpHit] = await Promise.all([
    env.COUNTER.get(rateKey),
    env.COUNTER.get(ipRateKey),
  ]);
  if (rIdHit || rIpHit) return txt("rate limited", 429);

  // Retry on the (extremely rare) collision; 5 attempts is plenty for a 1B
  // keyspace with ~weekly upload volume.
  let shortId = "";
  for (let attempt = 0; attempt < 5; attempt++) {
    const candidate = genDiagId();
    if (!await env.COUNTER.get(`diag:${candidate}`)) { shortId = candidate; break; }
  }
  if (!shortId) return txt("id collision, retry", 503);

  const country = request.headers.get("cf-ipcountry") || "??";
  const ts = Date.now();
  const stored = { ts, country, payload: body };

  // Tiny metadata so /diag/list can show useful info without fetching each
  // value. devId is truncated to 8 hex chars (one byte more is wasteful;
  // the full id stays in the payload). 1024-byte metadata budget is plenty.
  // carVer goes through a strict allowlist — a future maintainer dashboard
  // that renders it as HTML would otherwise be a stored-XSS surface.
  const carVer = typeof body.carSwVer === "string"
    ? body.carSwVer.replace(/[^A-Za-z0-9._-]/g, "").slice(0, 32)
    : "";
  const metadata = {
    ts,
    version,
    env: fwEnv,
    country,
    dev: id.slice(0, 8),
    carVer,
  };

  await Promise.all([
    env.COUNTER.put(`diag:${shortId}`, JSON.stringify(stored), { expirationTtl: TTL_DIAG, metadata }),
    env.COUNTER.put(rateKey,   "1", { expirationTtl: DIAG_RATE_LIMIT_SEC }),
    env.COUNTER.put(ipRateKey, "1", { expirationTtl: DIAG_IP_RATE_LIMIT_SEC }),
  ]);

  // Firmware POSTs from esp_http_client (no CORS) — don't advertise this
  // endpoint to browsers.
  return new Response(JSON.stringify({ id: shortId }), {
    headers: { ...NO_CORS, "Content-Type": "application/json" },
  });
}

// DIAG_KEY arrives via Authorization: Bearer <key> or X-Diag-Key header.
// Header (not query string) so the secret stays out of access logs, browser
// history, and referrer headers.
function getDiagKey(request) {
  const auth = request.headers.get("authorization") || "";
  const m = /^Bearer\s+(.+)$/i.exec(auth);
  if (m) return m[1].trim();
  return request.headers.get("x-diag-key") || "";
}

async function handleDiagFetch(request, env, url) {
  const shortId = url.pathname.slice("/diag/".length);
  if (!new RegExp(`^[${DIAG_ID_ALPHABET}]{${DIAG_ID_LEN}}$`).test(shortId)) return txt("bad id", 400);
  // DIAG_KEY is a Worker secret (wrangler secret put DIAG_KEY). Without it,
  // GET always returns 401 — short ids stay safe to paste publicly.
  const provided = getDiagKey(request);
  const expected = env.DIAG_KEY || "";
  if (!expected || !timingSafeEqual(provided, expected)) return txt("unauthorized", 401);
  const raw = await env.COUNTER.get(`diag:${shortId}`);
  if (!raw) return txt("not found", 404);
  return new Response(raw, {
    headers: { ...NO_CORS, "Content-Type": "application/json" },
  });
}

// GET /diag/list[?cursor=<c>]    (Authorization: Bearer <DIAG_KEY>)
// Maintainer endpoint — returns all diagnostic uploads from the last 30 days
// with their metadata (no payload, that requires GET /diag/<id>).
// Useful for spotting uploads that the user never shared the ID for.
async function handleDiagList(request, env, url) {
  const provided = getDiagKey(request);
  const expected = env.DIAG_KEY || "";
  if (!expected || !timingSafeEqual(provided, expected)) return txt("unauthorized", 401);
  const cursor = url.searchParams.get("cursor") || undefined;
  // KV cap is 1000/page. 30-day retention × realistic volume sits well under
  // this; cursor support is here for completeness, not a hot path.
  const res = await env.COUNTER.list({ prefix: "diag:", cursor, limit: 1000 });
  const items = res.keys.map((k) => ({
    id: k.name.slice("diag:".length),
    expiration: k.expiration || null,
    ...(k.metadata || {}),
  }));
  // Newest first — KV doesn't sort by metadata, so do it client-side.
  items.sort((a, b) => (b.ts || 0) - (a.ts || 0));
  return new Response(JSON.stringify({
    items,
    cursor: res.list_complete ? null : res.cursor,
    count: items.length,
  }), { headers: { ...NO_CORS, "Content-Type": "application/json" } });
}

function genDiagId() {
  const bytes = new Uint8Array(DIAG_ID_LEN);
  crypto.getRandomValues(bytes);
  let s = "";
  for (let i = 0; i < DIAG_ID_LEN; i++) {
    s += DIAG_ID_ALPHABET[bytes[i] % DIAG_ID_ALPHABET.length];
  }
  return s;
}

// Constant-time string compare to keep DIAG_KEY safe from timing oracles.
function timingSafeEqual(a, b) {
  if (a.length !== b.length) return false;
  let diff = 0;
  for (let i = 0; i < a.length; i++) diff |= a.charCodeAt(i) ^ b.charCodeAt(i);
  return diff === 0;
}

async function handlePing(request, env) {
  let body;
  try { body = await request.json(); } catch { return txt("bad json", 400); }
  const { id, version, env: fwEnv } = body || {};
  if (typeof id !== "string" || !/^[a-f0-9]{16,64}$/.test(id)) return txt("bad id", 400);
  if (typeof version !== "string" || !/^[0-9a-zA-Z._-]{1,16}$/.test(version)) return txt("bad version", 400);
  if (typeof fwEnv !== "string" || !/^[a-z0-9_-]{1,32}$/.test(fwEnv)) return txt("bad env", 400);
  // carVer is optional — empty / missing → bucket "(empty)" so we can track
  // how many users haven't filled it in. Strict allowlist mirrors firmware
  // sanitizer; bad chars or oversize → fall back to "(invalid)" so old clients
  // sending malformed values don't get a hard 400 (forward compat).
  let carVer = "(empty)";
  if (typeof body?.carVer === "string" && body.carVer.length > 0) {
    carVer = /^[A-Za-z0-9._-]{1,32}$/.test(body.carVer) ? body.carVer : "(invalid)";
  }

  const today = todayStr();
  const seenKey = `seen:${today}:${id}`;
  if (await env.COUNTER.get(seenKey)) return txt("ok (dedup)");

  const month = today.slice(0, 7);
  const year = today.slice(0, 4);
  const moKey = `mo:${month}:${id}`;
  const yrKey = `yr:${year}:${id}`;
  const everKey = `ever:${id}`;
  const aggDayKey = `agg:${today}`;
  const aggMoKey = `agg:mo:${month}`;
  const aggYrKey = `agg:yr:${year}`;
  const aggEverKey = "agg:ever";

  // Probe dedup markers in parallel so we only increment the rollups we
  // actually need. Reads are cheap (100k/day) — writes and lists are not.
  const [hasMo, hasYr, hasEver, aggDayRaw] = await Promise.all([
    env.COUNTER.get(moKey),
    env.COUNTER.get(yrKey),
    env.COUNTER.get(everKey),
    env.COUNTER.get(aggDayKey),
  ]);

  const [aggMoRaw, aggYrRaw, aggEverRaw] = await Promise.all([
    hasMo ? null : env.COUNTER.get(aggMoKey),
    hasYr ? null : env.COUNTER.get(aggYrKey),
    hasEver ? null : env.COUNTER.get(aggEverKey),
  ]);

  const country = request.headers.get("cf-ipcountry") || "??";

  // Daily rollup — bump count + per-dimension bucket. Schema uses short keys
  // (c/e/v/co/cv) to stay well under KV's 25MB value cap even at thousands of
  // versions/countries/carVers.
  const aggDay = parseAggDay(aggDayRaw);
  aggDay.c = (aggDay.c || 0) + 1;
  aggDay.e[fwEnv] = (aggDay.e[fwEnv] || 0) + 1;
  aggDay.v[version] = (aggDay.v[version] || 0) + 1;
  aggDay.co[country] = (aggDay.co[country] || 0) + 1;
  aggDay.cv[carVer] = (aggDay.cv[carVer] || 0) + 1;

  const writes = [
    env.COUNTER.put(seenKey, "1", { expirationTtl: TTL_DAY }),
    env.COUNTER.put(aggDayKey, JSON.stringify(aggDay), { expirationTtl: TTL_DAY }),
  ];
  if (!hasMo) {
    writes.push(env.COUNTER.put(moKey, "1", { expirationTtl: TTL_MONTH }));
    writes.push(env.COUNTER.put(aggMoKey, String((parseInt(aggMoRaw) || 0) + 1), { expirationTtl: TTL_MONTH }));
  }
  if (!hasYr) {
    writes.push(env.COUNTER.put(yrKey, "1", { expirationTtl: TTL_YEAR }));
    writes.push(env.COUNTER.put(aggYrKey, String((parseInt(aggYrRaw) || 0) + 1), { expirationTtl: TTL_YEAR }));
  }
  if (!hasEver) {
    writes.push(env.COUNTER.put(everKey, "1"));
    writes.push(env.COUNTER.put(aggEverKey, String((parseInt(aggEverRaw) || 0) + 1)));
  }
  await Promise.all(writes);
  return txt("ok");
}

function parseAggDay(raw) {
  if (!raw) return { c: 0, e: {}, v: {}, co: {}, cv: {} };
  try {
    const obj = JSON.parse(raw);
    return {
      c: obj.c || 0,
      e: obj.e || {},
      v: obj.v || {},
      co: obj.co || {},
      cv: obj.cv || {},
    };
  } catch {
    return { c: 0, e: {}, v: {}, co: {}, cv: {} };
  }
}

async function handleStatsCached(request, env, ctx) {
  const cache = caches.default;
  const cacheKey = new Request(request.url, { method: "GET" });
  const hit = await cache.match(cacheKey);
  if (hit) {
    const resp = new Response(hit.body, { headers: new Headers(hit.headers) });
    resp.headers.set("X-Cache", "HIT");
    return resp;
  }
  const resp = await handleStats(env);
  // Write-through off the hot path so a slow/failed cache.put can't block or
  // 500 the response — /stats is an optimization, not a source of truth.
  ctx.waitUntil(cache.put(cacheKey, resp.clone()).catch(() => {}));
  resp.headers.set("X-Cache", "MISS");
  return resp;
}

async function handleStats(env) {
  const now = new Date();
  const today = dayStr(now);
  const yesterday = dayStr(new Date(now.getTime() - 86400_000));
  const month = today.slice(0, 7);
  const year = today.slice(0, 4);

  // 5 reads, 0 list ops. Yesterday's per-dim breakdown is not needed for
  // the dashboard, so only the count is extracted.
  const [aggTodayRaw, aggYdayRaw, aggMoRaw, aggYrRaw, aggEverRaw] = await Promise.all([
    env.COUNTER.get(`agg:${today}`),
    env.COUNTER.get(`agg:${yesterday}`),
    env.COUNTER.get(`agg:mo:${month}`),
    env.COUNTER.get(`agg:yr:${year}`),
    env.COUNTER.get("agg:ever"),
  ]);

  const aggToday = parseAggDay(aggTodayRaw);
  const aggYday = parseAggDay(aggYdayRaw);

  return new Response(JSON.stringify({
    today: aggToday.c,
    yesterday: aggYday.c,
    month: parseInt(aggMoRaw) || 0,
    year: parseInt(aggYrRaw) || 0,
    total: parseInt(aggEverRaw) || 0,
    date: today,
    byEnv: aggToday.e,
    byVersion: aggToday.v,
    byCountry: aggToday.co,
    byCarVer: aggToday.cv,
  }), { headers: {
    ...CORS,
    "Content-Type": "application/json",
    "Cache-Control": `public, s-maxage=${STATS_CACHE_TTL_SEC}`,
  } });
}

function todayStr() { return dayStr(new Date()); }
function dayStr(d) { return d.toISOString().slice(0, 10); }
function txt(s, status = 200) { return new Response(s, { status, headers: CORS }); }
