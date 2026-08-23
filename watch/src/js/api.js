/*
 * Pebble Stocks — PebbleKit JS client for the Stocks API.
 * Copyright (C) 2026 Claudio Rojas
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Affero General Public License, version 3, as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Affero
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 *
 * SPDX-License-Identifier: AGPL-3.0-only
 */

/**
 * api.js — Calls the Pebble Stocks API server.
 *
 * BASE_URL below is a PLACEHOLDER and must be changed before you build.
 * 127.0.0.1 is the phone's own loopback address, so as shipped this reaches
 * nothing and every request fails with "Network error".
 *
 * Set it to wherever your API is deployed. For local testing use your
 * machine's LAN address (e.g. http://192.168.1.20:3000) — the phone cannot
 * reach your computer through localhost or 127.0.0.1. Note that iOS and
 * Android both block plaintext http:// unless the phone is configured to
 * allow it, so a deployed https:// endpoint is the path of least resistance.
 *
 * All callbacks follow the Node convention: callback(err, data).
 */

var BASE_URL = 'http://127.0.0.1:3000';

function fetchJSON(url, callback) {
    var xhr = new XMLHttpRequest();
    var done = false;

    var timer = setTimeout(function() {
        if (done) { return; }
        done = true;
        xhr.abort();
        callback(new Error('Network error'), null);
    }, 5000);

    xhr.onload = function() {
        if (done) { return; }
        done = true;
        clearTimeout(timer);
        if (xhr.status === 200) {
            try {
                callback(null, JSON.parse(xhr.responseText));
            } catch (e) {
                callback(new Error('JSON parse error: ' + e.message), null);
            }
        } else {
            // console.log('Error!');
            callback(new Error('HTTP ' + xhr.status), null);
        }
    };
    xhr.onerror = function() {
        if (done) { return; }
        done = true;
        clearTimeout(timer);
        callback(new Error('Network error'), null);
    };
    xhr.open('GET', url);
    xhr.send();
}

/**
 * Fetches the current quote for a symbol.
 * On success, data shape: { ticker, price, change, changePercent }
 */
function fetchQuote(symbol, callback) {
    fetchJSON(BASE_URL + '/api/tickers/' + symbol, callback);
}

/**
 * Fetches historical closes for a symbol.
 * range: '1D' | '1W' | '1M' | '3M' | 'YTD' | '1Y' (default: '1M')
 * On success, data shape: [[timestamp, close], ...]
 */
function fetchHistory(symbol, range, callback) {
    fetchJSON(BASE_URL + '/api/tickers/' + symbol + '/history?range=' + (range || '1M'), function(err, json) {
        callback(err, err ? null : json.data);
    });
}

module.exports = { fetchQuote: fetchQuote, fetchHistory: fetchHistory };
