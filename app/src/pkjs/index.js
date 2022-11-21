var Clay = require('pebble-clay');
var clayConfig = require('./config.json');
var clay = new Clay(clayConfig, null, {
	autoHandleEvents: false
});
var MessageQueue = require('message-queue-pebble');
var _ = require('lodash');
var ajax = require('pebblejs/dist/js/lib/ajax');

var tickers = []; 
var totalTickers; 

var key;

Pebble.addEventListener('showConfiguration', function(e) {
	Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
	if (e && !e.response) {
		return;
	}
	var configuration = clay.getSettings(e.response, false);
	localStorage.setItem("configuration", JSON.stringify(configuration));
	var str = configuration.Tickers.value;
	key = configuration.APIKey.value;
	tickers = str.split(", ");
	totalTickers = tickers.length;
});

Pebble.addEventListener('ready', function(e) {
	console.log("Ready!");

	var configuration = JSON.parse(localStorage.getItem("configuration"));
    key = configuration.APIKey.value;
	if (configuration && key) {
		var str = configuration.Tickers.value;
		tickers = str.split(", ");
		totalTickers = tickers.length;
		fetchWatchlist();
	} else {
        console.log("Bad configuration!");
    }

});

Pebble.addEventListener('appmessage', function(e) {
	console.log("AppMessage received!");
});

function fetchWatchlist() {
	for (var i = 0; i < totalTickers; i++) {
		var dict = {
			"Symbol": "",
			"Open": "",
			"High": "",
			"Low": "",
			"Price": "",
			"CloseHistory": [],
			"PrevClose": "",
			"Change": 0,
            "ChangeInt": 0,
			"ChangePercent": "",
			"TotalTickers": totalTickers
		};

		var ticker = tickers[i];

		ajax({
				url: "https://finnhub.io/api/v1/quote?symbol=" + ticker + "&token=" + key,
				type: 'json',
				async: false
			},
			function(data) {
				//add ticker data to dict 
				dict.Symbol = ticker.toString();
				dict.Open = formatStockPrice(data.o).toString();
				dict.High = formatStockPrice(data.h).toString();
				dict.Low = formatStockPrice(data.l).toString();
				dict.Price = "$" + formatStockPrice(data.c).toString();
				dict.PrevClose = formatStockPrice(data.pc).toString();
				dict.Change = formatStockPrice(data.d).toString();
                if (data.d > 0) {
                    //round up ceiling
                    dict.ChangeInt = Math.ceil(data.d);
                } else if (data.d < 0) {
                    //round down floor
                    dict.ChangeInt = Math.floor(data.d);
                } else {
                    dict.ChangeInt = 0;
                }
				dict.ChangePercent = formatStockPrice(data.dp).toString() + "%";
			}
		);

		var timestamp = Math.floor(Date.now() / 1000);
        //28 weeks and three days ago
        var timestamp28 = timestamp - 18144000;

		var candleUrl = "https://finnhub.io/api/v1/stock/candle?symbol=" + ticker + "&resolution=D&from=" + timestamp28 + "&to=" + timestamp + "&token=" + key;
		ajax({
				url: candleUrl,
				type: 'json',
				async: false
			},
			function(data) {
				var temp = _.values(data.c);
				var closeHistory = [];

                for (var i = 0; i < temp.length; i++) {
                    closeHistory.push(Number(temp[i]).toFixed(2));
                }


                var closeMax = closeHistory.reduce(function(a, b) {
                    return Math.max(a, b);
                });
                var closeMin = closeHistory.reduce(function(a, b) {
                    return Math.min(a, b);
                });
                var closeRange = closeMax - closeMin;

                var closeHis = [];
                for (var i = 0; i < closeHistory.length; i++) {
                    closeHis.push({
                        x: i,
                        price: Math.round((closeHistory[i] - closeMin) / closeRange * 100)
                    });
                }
                closeHis = largestTriangleThreeBuckets(closeHis, 140, "x", "price");
                closeHistory = [];
                for (var i = 0; i < closeHis.length; i++) {
                    closeHistory.push(110-closeHis[i].price);
                }

                dict.CloseHistory = closeHistory;
			}
		);

        MessageQueue.sendAppMessage(dict, onSuccess(dict), onFailure);
	}
}

//from pauleeeeee's pebble-stock-ticker/pkjs/index.js
function formatStockPrice(price) {
	if (price < 100) {
		price = price.toFixed(2);
	} else if (price >= 100 && price < 1000) {
		price = price.toFixed(1);
	} else if (price >= 1000) {
		price = Math.round(price);
	}
	return price;
}

function onSuccess(dict) {
	console.log('success');
}

function onFailure(data, error) {
	console.log('error');
}

//from pauleeeeee's pebble-stock-ticker/pkjs/index.js
function largestTriangleThreeBuckets(data, threshold, xAccessor, yAccessor) {

    var floor = Math.floor,
      abs = Math.abs;

    var daraLength = data.length;
    if (threshold >= daraLength || threshold === 0) {
      return data; // Nothing to do
    }

    var sampled = [],
      sampledIndex = 0;

    // Bucket size. Leave room for start and end data points
    var every = (daraLength - 2) / (threshold - 2);

    var a = 0,  // Initially a is the first point in the triangle
      maxAreaPoint,
      maxArea,
      area,
      nextA;

    sampled[ sampledIndex++ ] = data[ a ]; // Always add the first point

    for (var i = 0; i < threshold - 2; i++) {

      // Calculate point average for next bucket (containing c)
      var avgX = 0,
        avgY = 0,
        avgRangeStart  = floor( ( i + 1 ) * every ) + 1,
        avgRangeEnd    = floor( ( i + 2 ) * every ) + 1;
      avgRangeEnd = avgRangeEnd < daraLength ? avgRangeEnd : daraLength;

      var avgRangeLength = avgRangeEnd - avgRangeStart;

      for ( ; avgRangeStart<avgRangeEnd; avgRangeStart++ ) {
        avgX += data[ avgRangeStart ][ xAccessor ] * 1; // * 1 enforces Number (value may be Date)
        avgY += data[ avgRangeStart ][ yAccessor ] * 1;
      }
      avgX /= avgRangeLength;
      avgY /= avgRangeLength;

      // Get the range for this bucket
      var rangeOffs = floor( (i + 0) * every ) + 1,
        rangeTo   = floor( (i + 1) * every ) + 1;

      // Point a
      var pointAX = data[ a ][ xAccessor ] * 1, // enforce Number (value may be Date)
        pointAY = data[ a ][ yAccessor ] * 1;

      maxArea = area = -1;

      for ( ; rangeOffs < rangeTo; rangeOffs++ ) {
        // Calculate triangle area over three buckets
        area = abs( ( pointAX - avgX ) * ( data[ rangeOffs ][ yAccessor ] - pointAY ) -
              ( pointAX - data[ rangeOffs ][ xAccessor ] ) * ( avgY - pointAY )
              ) * 0.5;
        if ( area > maxArea ) {
          maxArea = area;
          maxAreaPoint = data[ rangeOffs ];
          nextA = rangeOffs; // Next a is this b
        }
      }

      sampled[ sampledIndex++ ] = maxAreaPoint; // Pick this point from the bucket
      a = nextA; // This a is the next a (chosen b)
    }

    sampled[ sampledIndex++ ] = data[ daraLength - 1 ]; // Always add last

    return sampled;
}