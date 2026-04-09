var api = require ("./api");

var Clay = require('@rebble/clay');
var clayConfig = require('./clay-config');
var clay = new Clay(clayConfig);

var MessageQueue = require("message-queue-pebble");

var MESSAGETYPE = {
    ERROR: 0,
    NOCONNECTION: 1,
    READY: 2,
    LOADED: 3,
    SYMBOLDATA: 4,
    HISTORYREQUEST: 5,
    HISTORYDATA: 6,
    REFRESH: 7,
};

var DEFAULT_WATCHLIST = ['SPY', 'AAPL', 'MSFT', 'GOOG', 'META'];

function getWatchlist() {
    var settings = {};
    try {
        var raw = localStorage.getItem('clay-settings');
        if (raw) settings = JSON.parse(raw);
    } catch (e) {}

    var list = [];
    for (var i = 1; i <= 10; i++) {
        var val = ((settings['Ticker_' + i] || '') + '').trim().toUpperCase();
        if (val) list.push(val);
    }
    return list.length ? list : DEFAULT_WATCHLIST;
}

function loadHistory(symbol, timeframe) {
    console.log("[PKJS][HISTORY] loadHistory called: " + symbol + " / " + timeframe);

    var watchlist = getWatchlist();
    var position = watchlist.indexOf(symbol);
    console.log("[PKJS][HISTORY] position=" + position + " queueSize=" + MessageQueue.size());

    api.fetchHistory(symbol, timeframe, function(err, history) {
        if (err) {
            console.log("[PKJS][HISTORY] API error: " + err.message);
            return;
        }
        if (!history || !history.length) {
            console.log("[PKJS][HISTORY] API returned empty history");
            return;
        }

        console.log("[PKJS][HISTORY] Got " + history.length + " points from API");

        var closes = history.map(function(point) { return point[1]; });

        // Pack each close price as a signed int32 (cents) in little-endian byte order
        var buf = [];
        closes.forEach(function(c) {
            var val = Math.round(c * 100);
            buf.push(val & 0xFF);
            buf.push((val >> 8) & 0xFF);
            buf.push((val >> 16) & 0xFF);
            buf.push((val >> 24) & 0xFF);
        });

        var data = {
            'Type': MESSAGETYPE.HISTORYDATA,
            'WatchlistPosition': position,
            'Symbol': symbol,
            'Timeframe': timeframe,
            'HistoryData': buf,
            'HistoryDataSize': closes.length
        };

        console.log("[PKJS][HISTORY] Sending " + closes.length + " points (" + buf.length + " bytes), pos=" + position);
        Pebble.sendAppMessage(data, function() {
            console.log("[PKJS][HISTORY] ACK: " + symbol + " delivered to watch.");
        }, function(e) {
            console.log("[PKJS][HISTORY] NACK: " + JSON.stringify(e));
        });
    });
}

function loadWatchlist(watchlist) {
    var completed = 0;
    var noConnectionSent = false;

    watchlist.forEach(function(symbol) {
        api.fetchQuote(symbol, function(err, quote) {
            if (err && err.message === 'Network error') {
                console.log("[Quote] No connection: " + symbol);
                if (!noConnectionSent) {
                    noConnectionSent = true;
                    MessageQueue.sendAppMessage({ 'Type': MESSAGETYPE.NOCONNECTION });
                }
                return;
            }

            var data = {
                'WatchlistPosition': watchlist.indexOf(symbol),
                'Type': MESSAGETYPE.SYMBOLDATA,
                'WatchlistSize': watchlist.length,
                'Symbol': symbol,
                'Price': err ? 0 : Math.round(quote.price * 100),
                'Change': err ? 0 : Math.round(quote.change * 100),
                'ChangePercent': err ? 0 : Math.round(quote.changePercent * 100),
                'LastUpdated': err ? 0 : Math.floor(Date.now() / 1000)
            };

            if (err) {
                console.log("[Quote] " + symbol + ": " + err.message);
            }

            MessageQueue.sendAppMessage(data, function() {
                console.log("[Quote] " + symbol + " data sent to watch: " + JSON.stringify(data));
            }, function(e) {
                console.log("[Quote] Error sending quote data: " + JSON.stringify(e));
            });

            completed++;
            if (completed === watchlist.length) {
                MessageQueue.sendAppMessage({ 'Type': MESSAGETYPE.LOADED });
            }
        });
    });
}

Pebble.addEventListener('appmessage', function(e) {
    var dict = e.payload;

    if (dict.Type == MESSAGETYPE.HISTORYREQUEST) {
        console.log('[PKJS][HISTORY] Got request for ' + dict.Symbol + ' over ' + dict.Timeframe);
        loadHistory(dict.Symbol, dict.Timeframe);
    } else if (dict.Type == MESSAGETYPE.REFRESH) {
        console.log('[PKJS] Got refresh request from watch');
        loadWatchlist(getWatchlist());
    } else {
        console.log('[PKJS] Received unknown message: ' + JSON.stringify(dict));
    }

});

Pebble.addEventListener('ready', function() {
    console.log('[PKJS] PebbleKit JS ready!');

    MessageQueue.sendAppMessage({ 'Type': MESSAGETYPE.READY }, function() {
        console.log('[PKJS] Watch notified of PKJS Ready Event');
        loadWatchlist(getWatchlist());
    }, function(e) {
        console.log('[PKJS] Error notifying watch of PKJS Ready Event: ' + JSON.stringify(e));
    });
});
