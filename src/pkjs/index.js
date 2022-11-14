const Clay = require('pebble-clay');
const clayConfig = require('./config.json');
const clay = new Clay(clayConfig, null, { autoHandleEvents: false });
const MessageQueue = require('message-queue-pebble');

let tickers = []; //set default tickers in case user doesn't set any
let totalTickers; //to be returned for menu layer construction

const key = "cdond1aad3i3u5goj3agcdond1aad3i3u5goj3b0"

Pebble.addEventListener('showConfiguration', function(e) {
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
    if (e && !e.response) {
        return;
    }
    let configuration = clay.getSettings(e.response, false);
    localStorage.setItem("configuration", JSON.stringify(configuration));
    let str = configuration.Tickers.value;
    tickers = str.split(", ");
    totalTickers = tickers.length;
    //fetchWatchlist(); //figure out full refresh on config change
});

Pebble.addEventListener('ready', function () {
    console.log("Ready!");
    var configuration = JSON.parse(localStorage.getItem("configuration"));
    if (configuration) {
        let str = configuration.Tickers.value;
        tickers = str.split(", ");
        totalTickers = tickers.length;
        fetchWatchlist();
    }
});

Pebble.addEventListener('appmessage', function (e) {
    console.log("AppMessage received!");
});

function fetchWatchlist() {
    for (let i = 0; i < totalTickers; i++) {
        const ticker = tickers[i];
        const url = "https://finnhub.io/api/v1/quote?symbol=" + ticker + "&token=" + key;
        const req = new XMLHttpRequest();
        req.open('GET', url, true);
        req.onload = function (e) {
            if (req.status == 200) {
                const data = JSON.parse(req.responseText);
                let message = {
                    "Symbol": ticker,
                    "Open": data["o"].toString(),
                    "High": data["h"].toString(),
                    "Low": data["l"].toString(),
                    "Price": data["c"].toString(),
                    "PrevClose": data["pc"].toString(),
                    "Change": data["d"].toString(),
                    "ChangePercent": data["dp"].toString(),
                    "TotalTickers": totalTickers
                };

                MessageQueue.sendAppMessage(message, onSuccess, onFailure);
            }     
        } 
        req.send();  
    }
}

function fetchDetails() {
    //fetch details for selected stock
    
}

function onSuccess(data){
    console.log('success');
    console.log(JSON.stringify(data));
}

function onFailure(data, error){
    console.log('error');
    console.log(JSON.stringify(data));
    console.log(JSON.stringify(error));

}