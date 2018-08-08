// webrtcSpectrum.js: setup peer to peer connection from browser
// to spectrum Feeder program that reads the USRP.


var connectCount = 0; // this number only increases.
var opt = require('./opt');
var mosca = require('mosca');

var server = new mosca.Server({
    secure : {
        port: opt.mqtt_port,
        keyPath: opt.tls.key,  // private_key
        certPath: opt.tls.cert // public_cert
    }
});

server.on('ready', function(){
    console.log("MQTT Broker listening on port: " + opt.mqtt_port);
});


// Let the module user just see what's in module.exports.  You can have
// your room (module) as messy as you like but don't let others see the
// mess (via module.exports).
//
// None of the code in this file is seen in other files except
// module.exports.
//
module.exports = {
    init: null,
    otherStuff: 'otherStuff' // TODO: remove this...
};
