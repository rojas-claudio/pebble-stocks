# Pebble Stocks

A stock watchlist app for the Pebble smartwatch, plus the small caching API that feeds it.

Track up to 10 tickers, see live prices and daily change on the watch, and drill into any
symbol for a price graph across six timeframes. Runs on every Pebble platform — original
Pebble through Pebble Time 2.

> **For informational use only.** Prices are delayed and may be inaccurate or incomplete.
> This is not financial advice. See [Disclaimer](#disclaimer).

---

## Contents

- [How it works](#how-it-works)
- [Repository layout](#repository-layout)
- [The watch app](#the-watch-app)
  - [Building and installing](#building-and-installing)
  - [Configuring your watchlist](#configuring-your-watchlist)
  - [Controls](#controls)
- [The API](#the-api)
  - [Running it](#running-it)
  - [Environment variables](#environment-variables)
  - [Endpoints](#endpoints)
  - [Caching](#caching)
  - [Deploying](#deploying)
  - [Pointing the watch at your server](#pointing-the-watch-at-your-server)
- [Message protocol](#message-protocol)
- [Known gaps](#known-gaps)
- [Disclaimer](#disclaimer)
- [License](#license)

---

## How it works

Three pieces, in a straight line:

```
┌──────────────┐  AppMessage  ┌───────────────┐   HTTPS   ┌──────────────┐   HTTPS   ┌───────────────┐
│  Watch (C)   │ ◄──────────► │ PebbleKit JS  │ ◄───────► │  Stocks API  │ ◄───────► │ Yahoo Finance │
│  UI + graph  │              │  on the phone │           │ Express+Mongo│           │     chart     │
└──────────────┘              └───────────────┘           └──────────────┘           └───────────────┘
                                                                 │
                                                            MongoDB cache
```

The watch never talks to the network directly. PebbleKit JS on the phone reads the
watchlist out of Clay settings, calls the API for each symbol, and pushes the results
down over AppMessage. Prices are packed as integer cents to keep the messages small;
history is sent as a raw little-endian `int32` byte array (one close per point, max 100
points).

The API sits in front of Yahoo Finance purely as a cache — the Pebble's radio budget and
Yahoo's rate limits both prefer one warm document over ten cold fetches.

## Repository layout

```
watch/          Pebble app
  src/c/          C sources — app entry, windows, custom layers
    stocks.c        AppMessage handling, watchlist state, callbacks
    windows/        splash, watchlist, detail, disclaimer, error
    layers/         graph, info bar, progress bar
  src/js/         PebbleKit JS (runs on the phone)
    index.js        message routing, watchlist loading, history packing
    api.js          HTTP client for the Stocks API
    clay-config.json  settings screen
  resources/      menu icon, error PDC animation
  wscript         waf build script (stock Pebble SDK template)
  release.sh      builds against the real endpoint, restores the placeholder after

api/            Node/Express caching API
  server.js       routes, Mongo connection, cron refresh
  start.js        local entry point (listens on PORT; Vercel uses server.js directly)
  .env.example    template for .env
  provider/       Yahoo Finance client
  models/Ticker.js  Mongoose schema (quote + per-range history map)
  vercel.json     serverless deploy config
```

## The watch app

### Building and installing

Requires the [Pebble SDK](https://github.com/pebble-dev/rebble-tool) (the community
`pebble-tool` from Rebble works; SDK 3, `enableMultiJS`).

```bash
cd watch
npm install          # @rebble/clay, message-queue-pebble
pebble build
pebble install --emulator basalt     # or --phone <ip> for a real watch
pebble logs --emulator basalt        # PKJS + watch logs
```

> **Before building**, change the placeholder API address in `watch/src/js/api.js` — see
> [Pointing the watch at your server](#pointing-the-watch-at-your-server). An unmodified
> build cannot reach any server.


Target platforms built by default: `aplite`, `basalt`, `diorite`, `chalk`, `emery`,
`flint`, `gabbro`.

### Configuring your watchlist

Open the app's settings from the Pebble phone app. The Clay screen takes up to 10 ticker
symbols; blanks are skipped and duplicates are collapsed. With nothing configured the app
falls back to `SPY, AAPL, MSFT, GOOG, META`.

Settings changes require restarting the watch app to take effect.

### Controls

**Watchlist** — the list of configured symbols with price and change.
| Button | Action |
| --- | --- |
| Up / Down | Scroll |
| Select | Open the detail view for that symbol |
| Select on the last row | Show the disclaimer |

**Detail** — price, change, market-hours indicator, and a price graph.
| Button | Action |
| --- | --- |
| Up / Down | Cycle to the previous/next symbol in the watchlist |
| Select | Open the action menu |

The action menu offers **Refresh** (re-pull every quote) and **Graph →** with the
timeframes `1D`, `1W`, `1M`, `3M`, `YTD`, `1Y`. The chosen timeframe persists across
launches.

The info bar shows the current time, the market phase (pre-market, open, post-market,
closed) and the symbol's position in the watchlist. It ticks every minute.

## The API

Express 5 + Mongoose, ESM. One collection: `tickers`.

### Running it

```bash
cd api
npm install
cp .env.example .env    # then fill in MONGODB_URI
npm run dev             # node --watch, loads .env
```

`server.js` exports the Express app rather than listening, because that is what the Vercel
serverless adapter wants. `start.js` is the entry point everywhere else — it imports the app,
listens on `PORT` (default 3000), and exits with a clear message if `MONGODB_URI` is unset.
Use `npm start` for a plain run and `npm run dev` to reload on change.

### Environment variables

| Variable | Required | Purpose |
| --- | --- | --- |
| `MONGODB_URI` | yes | MongoDB connection string for the quote/history cache |
| `CRON_SECRET` | yes, for refresh | Shared secret for `POST /api/refresh`, sent as the `x-cron-secret` header. The endpoint **refuses to run** (503) when this is unset, so refresh is disabled until you configure it. |
| `PORT` | no | Port for `npm start` / `npm run dev`. Ignored on Vercel. |

No API key is needed — Yahoo Finance's public chart endpoint is unauthenticated.

### Endpoints

**`GET /api/tickers/:ticker`** — current quote.

```json
{ "ticker": "AAPL", "price": 213.25, "change": 1.84, "changePercent": 0.87, "marketHours": 1 }
```

`marketHours`: `0` pre-market, `1` regular, `2` post-market, `3` closed.
Returns `404` if the symbol is unknown to Yahoo.

**`GET /api/tickers/:ticker/history?range=1M`** — closes for the graph.

```json
{ "ticker": "AAPL", "range": "1M", "data": [[1714003200, 212.5], [1714089600, 213.1]] }
```

`range` is one of `1D`, `1W`, `1M`, `3M`, `YTD`, `1Y` (default `1M`); anything else is a
`400`. Series are downsampled to at most 100 points to fit the watch's buffer.

**`POST /api/refresh`** — re-fetches every cached ticker. Meant for a cron trigger.
Skips with `{"skipped": true}` outside regular market hours (9:30–16:00 ET, Mon–Fri).

Requires the `x-cron-secret` header to match `CRON_SECRET`, compared in constant time.
This fails **closed**: with no `CRON_SECRET` configured the endpoint returns `503` rather
than running unauthenticated.

Refresh is bounded, because the quote endpoint is public and anyone can create cache
entries. Only tickers requested in the last **7 days** are eligible, at most **250** per
run (most-recently-requested first), fetched in batches of **5** rather than all at once —
a wide parallel burst from one egress IP is what gets that IP rate-limited by Yahoo. Those
limits are the `REFRESH_*` constants at the top of `server.js`.

Tickers nobody asks for simply fall out of rotation; the next request for one re-arms it,
since a cache miss fetches fresh anyway.

**`GET /`** — health check.

### Caching

Every response is served from MongoDB when it is fresh enough, otherwise fetched from
Yahoo and written back.

| Data | TTL |
| --- | --- |
| Quote | 5 min |
| History `1D` | 5 min |
| History `1W` | 30 min |
| History `1M` / `3M` / `YTD` / `1Y` | 24 h |

History responses are sent to the client *before* the cache write, so a slow or failing
write never blocks the watch.

Each ticker also carries a `lastAccessedAt` stamp, updated at most once an hour per symbol,
which is what scopes the cron refresh to symbols actually in use.

### Deploying

`api/vercel.json` routes everything to `server.js` via `@vercel/node`. Set `MONGODB_URI`
and `CRON_SECRET` as project environment variables, then point a scheduler at
`POST /api/refresh` on a 5-minute cadence to match the quote TTL.

### Pointing the watch at your server

**You must edit one line before building.** The watch's API base URL is hard-coded in
`watch/src/js/api.js`, and what ships in this repo is a placeholder:

```js
var BASE_URL = 'http://127.0.0.1:3000';
```

`127.0.0.1` is the *phone's* own loopback address, so an unmodified build reaches nothing
and every request fails with `Network error` — the watch shows the connection error screen.
Change it to wherever your API lives.

For local testing, use your machine's LAN address (`http://192.168.1.20:3000` or similar).
The phone cannot reach your computer via `localhost` or `127.0.0.1`, and both iOS and
Android block plaintext `http://` from PebbleKit JS unless the phone is configured to allow
it — so a deployed `https://` endpoint is usually the least painful option.

Because this is a tracked file, changing it leaves a modification in your working tree.
Take care not to commit your own endpoint.

#### Release builds

`watch/release.sh` exists so release builds don't depend on remembering that. It swaps in
the real endpoint, builds, and restores the placeholder on every exit path — success,
build failure, or Ctrl-C — so the live endpoint never survives in the working tree:

```bash
STOCKS_API_BASE=https://api.example.com ./release.sh
# or write the endpoint once to watch/.release-endpoint (gitignored)
./release.sh
```

It refuses to build with no endpoint configured, with a malformed one, or with the
placeholder itself, and it verifies the endpoint actually landed in
`build/pebble-js-app.js` before reporting success — the bundle is what ships, so that is
what gets checked.

## Message protocol

`Type` selects the message; both directions use the same enum.

| Value | Name | Direction | Meaning |
| --- | --- | --- | --- |
| 0 | `ERROR` | JS → watch | Generic error |
| 1 | `NOCONNECTION` | JS → watch | Network unreachable; watch shows the error screen |
| 2 | `READY` | JS → watch | PebbleKit JS is up (splash → 25%) |
| 3 | `LOADED` | JS → watch | All quotes sent; watch opens the watchlist |
| 4 | `SYMBOLDATA` | JS → watch | One quote |
| 5 | `HISTORYREQUEST` | watch → JS | Ask for `Symbol` over `Timeframe` |
| 6 | `HISTORYDATA` | JS → watch | Packed closes |
| 7 | `REFRESH` | watch → JS | Re-pull the whole watchlist |

Prices, changes and percentages all travel as integers scaled by 100. History closes are
`int32` cents, little-endian, four bytes per point.

## Known gaps

Things a contributor will trip over:

- **`store/` is gitignored**, so the App Store screenshots and icons are not in the repo.
  Un-ignore it if you want them rendered in this README.
- **Watchlist changes need an app restart** — the JS reads Clay settings once at launch.
- **The API endpoint is hard-coded** in `watch/src/js/api.js` and ships as an unusable
  placeholder, so every build starts with an edit to a tracked file. There is no build-time
  or runtime override; adding one would mean a Clay setting or a generated config module.
- **Any symbol Yahoo recognises still creates a cached document.** Refresh is now bounded
  (see [Endpoints](#endpoints)), so the recurring cost is capped, but nothing stops the
  collection itself from growing. Add an allowlist or a TTL index if that matters to you.

## Disclaimer

For informational use only. Data is delayed a minimum of 2 minutes and may be inaccurate
or incomplete. Prices are sourced from third-party providers and are not guaranteed. This
is not financial advice — do not make investment or trading decisions based on data shown
here. The developer assumes no liability for losses incurred from use of this app.

This project is not affiliated with, endorsed by, or sponsored by Yahoo. Use of the Yahoo
Finance endpoint is subject to Yahoo's terms.

## License

Copyright (C) 2026 Claudio Rojas.

This program is free software: you can redistribute it and/or modify it under the terms
of the **GNU Affero General Public License, version 3**, as published by the Free Software
Foundation. It is distributed WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See [LICENSE](LICENSE) for the full
text, or <https://www.gnu.org/licenses/>.

`SPDX-License-Identifier: AGPL-3.0-only`

The AGPL is used rather than the plain GPL because `api/` is meant to be *run as a network
service*. Under [section 13](LICENSE), anyone who deploys a modified version of the API and
lets other people use it over a network must offer those users its source — merely hosting
it, without distributing binaries, does not sidestep the copyleft.

### Third-party code

`watch/src/c/layers/progress_layer.{c,h}` is not covered by the above: it comes from the
Pebble SDK example code and retains its original upstream license and copyright. It carries
no AGPL header for that reason.
