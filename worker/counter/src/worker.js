// tesla-fsd-controller anonymous usage counter.
//
//   POST /ping  {id, version, env}  — records device for today (id = sha256(MAC) hex, 16-64 chars)
//   GET  /stats                      — returns {today, yesterday, month, year, total, date, byEnv, byVersion, byCountry}
//
// Storage — per-device dedup markers + pre-aggregated rollup counters.
// /stats reads 5 counter keys (no list ops). /ping updates counters in place.
//   seen:YYYY-MM-DD:<id> → "1"            TTL 3d   (daily dedup)
//   mo:YYYY-MM:<id>      → "1"            TTL 62d  (MAU dedup)
//   yr:YYYY:<id>         → "1"            TTL 400d (YAU dedup)
//   ever:<id>            → "1"            no TTL   (all-time dedup)
//   agg:YYYY-MM-DD       → {c, e, v, co}  TTL 3d   (daily rollup — count + byEnv/byVersion/byCountry)
//   agg:mo:YYYY-MM       → "<int>"        TTL 62d  (MAU count)
//   agg:yr:YYYY          → "<int>"        TTL 400d (YAU count)
//   agg:ever             → "<int>"        no TTL   (all-time count)
//
// Concurrency: two concurrent /ping for distinct devices can race the
// read-modify-write on agg:* and lose one increment. Acceptable for anon
// stats. If tighter accuracy is needed later, move counters to a Durable
// Object.

const CORS = {
  "Access-Control-Allow-Origin": "*",
  "Access-Control-Allow-Methods": "GET, POST, OPTIONS",
  "Access-Control-Allow-Headers": "Content-Type",
};

// Edge cache for /stats. Kept long enough that a steady dashboard poll
// (5-min) mostly hits cache — keeps us well under the 1k list-ops/day free
// tier even if listing is reintroduced later.
const STATS_CACHE_TTL_SEC = 600;

const TTL_DAY = 3 * 24 * 3600;
const TTL_MONTH = 62 * 24 * 3600;
const TTL_YEAR = 400 * 24 * 3600;

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
    return new Response(
      "tesla-counter\n\nPOST /ping {id, version, env}\nGET /stats\n",
      { headers: CORS }
    );
  },
};

async function handlePing(request, env) {
  let body;
  try { body = await request.json(); } catch { return txt("bad json", 400); }
  const { id, version, env: fwEnv } = body || {};
  if (typeof id !== "string" || !/^[a-f0-9]{16,64}$/.test(id)) return txt("bad id", 400);
  if (typeof version !== "string" || !/^[0-9a-zA-Z._-]{1,16}$/.test(version)) return txt("bad version", 400);
  if (typeof fwEnv !== "string" || !/^[a-z0-9_-]{1,32}$/.test(fwEnv)) return txt("bad env", 400);

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
  // (c/e/v/co) to stay well under KV's 25MB value cap even at thousands of
  // versions/countries.
  const aggDay = parseAggDay(aggDayRaw);
  aggDay.c = (aggDay.c || 0) + 1;
  aggDay.e[fwEnv] = (aggDay.e[fwEnv] || 0) + 1;
  aggDay.v[version] = (aggDay.v[version] || 0) + 1;
  aggDay.co[country] = (aggDay.co[country] || 0) + 1;

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
  if (!raw) return { c: 0, e: {}, v: {}, co: {} };
  try {
    const obj = JSON.parse(raw);
    return {
      c: obj.c || 0,
      e: obj.e || {},
      v: obj.v || {},
      co: obj.co || {},
    };
  } catch {
    return { c: 0, e: {}, v: {}, co: {} };
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
  }), { headers: {
    ...CORS,
    "Content-Type": "application/json",
    "Cache-Control": `public, s-maxage=${STATS_CACHE_TTL_SEC}`,
  } });
}

function todayStr() { return dayStr(new Date()); }
function dayStr(d) { return d.toISOString().slice(0, 10); }
function txt(s, status = 200) { return new Response(s, { status, headers: CORS }); }
