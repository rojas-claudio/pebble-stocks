const Clay = require('pebble-clay');
const clayConfig = require('./config.json');
const clay = new Clay(clayConfig);
const MessageQueue = require('message-queue-pebble');

const tickers = ["AAPL", "MSFT", "AMZN", "META"]; //set default tickers in case user doesn't set any
const totalTickers = tickers.length; //to be returned for menu layer construction

const key = "KOBYCP2KJDXZNUYP";

Pebble.addEventListener('showConfiguration', function(e) {
    Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclsoed', function(e) {

});

Pebble.addEventListener('ready', function () {
    console.log("Ready!");
    fetch();
});

Pebble.addEventListener('appmessage', function (e) {
    console.log("AppMessage received!");
});



function fetch() {
    for (let i = 0; i < totalTickers; i++) {
        const ticker = tickers[i];
        const url = "https://www.alphavantage.co/query?function=GLOBAL_QUOTE&symbol=" + ticker + "&apikey=" + key;
        const req = new XMLHttpRequest();
        req.open('GET', url, true);
        req.onload = function(e) {
            if (req.status == 200) {
                var data = JSON.parse(req.responseText);

                if(data["Global Quote"]) {
                    var quote = data["Global Quote"];

                    var message = {
                        "Symbol": quote["01. symbol"],
                        "Open": quote["02. open"],
                        "High": quote["03. high"],
                        "Low": quote["04. low"],
                        "Price": quote["05. price"],
                        "Volume": quote["06. volume"],
                        "PrevClose": quote["08. previous close"],
                        "Change": quote["09. change"],
                        "ChangePercent": quote["10. change percent"],
                        "Ready": 1,
                        "TotalTickers": totalTickers
                    };

                    MessageQueue.sendAppMessage(message, onSuccess, onFailure);
                } else {

                }
            } else {
                console.log("Error");
            }
        }
        req.send();
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