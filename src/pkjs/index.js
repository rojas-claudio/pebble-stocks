const Clay = require('pebble-clay');
const clayConfig = require('./config.json');
const clay = new Clay(clayConfig, null, { autoHandleEvents: false });
const MessageQueue = require('message-queue-pebble');

let tickers = []; //set default tickers in case user doesn't set any
let totalTickers; //to be returned for menu layer construction

const key = "cdond1aad3i3u5goj3agcdond1aad3i3u5goj3b0";

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
    //create dict to send to watch
    let dict = {
        "Symbol": "",
        "Open": "",
        "High": "",
        "Low": "",
        "Price": "",
        "CloseHistory": [],
        "PrevClose": "",
        "Change": "",
        "ChangePercent": "",
        "TotalTickers": 0
    };

    for (let i = 0; i < totalTickers; i++) {

        //get basic ticker data
        const ticker = tickers[i];
        console.log("Fetching data for " + ticker);
        let url = "https://finnhub.io/api/v1/quote?symbol=" + ticker + "&token=" + key;
        let req = new XMLHttpRequest();
        req.open('GET', url, true);
        req.onload = function (e) {
            if (req.status == 200) {
                const data = JSON.parse(req.responseText);
                //add ticker data to dict 
                dict.Symbol = ticker;
                dict.Open = data["o"].toString();
                dict.High = data["h"].toString();
                dict.Low = data["l"].toString();
                dict.Price = data["c"].toString();
                dict.PrevClose = data["pc"].toString();
                dict.Change = data["d"].toString();
                dict.ChangePercent = data["dp"].toString() + "%";
                dict.TotalTickers = totalTickers;
            }     
        } 
        req.send(); 

        //get graph data (week)
        //get current unix timestamp
        const timestamp = Math.floor(Date.now() / 1000);
        //get unix timestamp from a week ago (modifiable in the future)
        const weekAgo = timestamp - 604800;

        url = "https://finnhub.io/api/v1/stock/candle?symbol=" + ticker + "&resolution=D&from=" + weekAgo + "&to=" + timestamp + "&token=" + key;
        req = new XMLHttpRequest();
        req.open('GET', url, true);
        req.onload = function (e) {
            if (req.status == 200) {
                const data = JSON.parse(req.responseText);
                //add graph data to dict
                dict.CloseHistory = data["c"];
            }     
        }
        req.send();

        console.log(dict.toString());
        MessageQueue.sendAppMessage(dict, onSuccess, onFailure);
    }
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