/**
 * api.js — Calls the local Pebble Stocks API server.
 *
 * Set BASE_URL to the server's address on your local network so the phone
 * can reach it (the phone cannot use "localhost" to reach a server on your
 * computer — use the machine's LAN IP instead).
 *
 * All callbacks follow the Node convention: callback(err, data).
 */

var BASE_URL = 'http://192.168.20.151';

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
