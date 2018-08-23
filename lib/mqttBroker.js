// This is dead code.  The mosca MQTT broker does not install with
// the latest node JS (Aug, 2018) and it takes away the webSocket
// connections from the rest of the server code.

// webrtcSpectrum.js: setup peer to peer connection from browser
// to spectrum Feeder program that reads the USRP.


var connectCount = 0; // this number only increases.
var opt = require('./opt');
var mosca = require('mosca');

var server = new mosca.Server({
    logger: {
        level: 'debug'
    },
});


function init(httpsServer, port) {

    server.attachHttpServer(httpsServer);

    server.on('ready', function(){
        console.log("MQTT Broker listening on port: " + port);
    });
}


// Let the module user just see what's in module.exports.  You can have
// your room (module) as messy as you like but don't let others see the
// mess (via module.exports).
//
// None of the code in this file is seen in other files except
// module.exports.
//
module.exports = {
    init: init,
    otherStuff: 'otherStuff' // TODO: remove this...
};
