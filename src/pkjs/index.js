const Clay = require('pebble-clay');
const clayConfig = require('./config.json');
const clay = new Clay(clayConfig, null, { autoHandleEvents: false });
const MessageQueue = require('message-queue-pebble');
const _ = require('lodash');

let tickers = []; //set default tickers in case user doesn't set any
let totalTickers; //to be returned for menu layer construction

let key = "cdond1aad3i3u5goj3agcdond1aad3i3u5goj3b0";

Pebble.addEventListener('ready', function (e) {
    console.log("Ready!");
    var configuration = JSON.parse(localStorage.getItem("configuration"));
    if (configuration) {
        let str = configuration.Tickers.value;
        tickers = str.split(", ");
        totalTickers = tickers.length;
        fetchWatchlist();
    }
});

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
    key = configuration.APIKey.value;
    tickers = str.split(", ");
    totalTickers = tickers.length;
    //fetchWatchlist();
});

Pebble.addEventListener('appmessage', function (e) {
    console.log("AppMessage received!");
});

function fetchWatchlist() {
    // let once = true;

    for (let i = 0; i < totalTickers; i++) {
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
            "TotalTickers": totalTickers
        };

        //get basic ticker data
        const ticker = tickers[i];
        const quoteUrl = "https://finnhub.io/api/v1/quote?symbol=" + ticker + "&token=" + key;
        let req = new XMLHttpRequest();
        req.open('GET', quoteUrl, false);
        req.onload = function (e) {
            if (req.status == 200) {
                console.log(`Fetching ${ticker} data...`);
                let data = JSON.parse(req.responseText);
                //add ticker data to dict 
                dict.Symbol = ticker;
                dict.Open = formatStockPrice(data.o).toString();
                dict.High = formatStockPrice(data.h).toString();
                dict.Low = formatStockPrice(data.l).toString();
                dict.Price = "$" + formatStockPrice(data.c).toString();
                dict.PrevClose = formatStockPrice(data.pc).toString();
                dict.Change = formatStockPrice(data.d).toString();
                dict.ChangePercent = formatStockPrice(data.dp).toString() + "%";
            }     
        }
        req.send();

        //get graph data (week)
        //get current unix timestamp
        const timestamp = Math.floor(Date.now() / 1000);
        //get unix timestamp from a month ago
        const monthAgo = timestamp - 2592000;

        const candleUrl = "https://finnhub.io/api/v1/stock/candle?symbol=" + ticker + "&resolution=D&from=" + monthAgo + "&to=" + timestamp + "&token=" + key;
        req = new XMLHttpRequest();
        req.open('GET', candleUrl, false);
        req.onload = function (e) {
            if (req.status == 200) {
                let data = JSON.parse(req.responseText);
                
                let temp = _.values(data.c);
                let closeHistory = [];

                for (let i = 0; i < temp.length; i++) {
                    closeHistory.push(Number(temp[i]).toFixed(2));
                }

                closeHistory.reverse();
                
                console.log(`Close history for ${ticker}: ${closeHistory}`);
                dict.CloseHistory = closeHistory;
            }     
        }
        req.send();

        //send data to watch
        console.log(`Sending ${ticker} data...`);
        MessageQueue.sendAppMessage(dict, onSuccess, onFailure);
    }
}

//from pauleeeeee's pebble-stock-ticker/pkjs/index.js
function formatStockPrice(price){
    if (price < 100){
        price = price.toFixed(2);
    } else if (price >= 100 && price < 1000) {
        price = price.toFixed(1);
    } else if (price >= 1000){
        price = Math.round(price);
    }
    return price+"";
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