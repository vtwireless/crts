// webrtcSpectrum.js: setup peer to peer connection from browser
// to spectrum Feeder program that reads the USRP.


var connectCount = 0; // this number only increases.


var wrtc = require('wrtc');

console.log("loaded 'wtrc'");


// This takes two client connections one being a (1) browser webSocket and
// the other being a (2) WebSocket (or TCP/IP) connection to the spectrum
// feeder program that reads the USRP (radio thingy); and than makes a
// webRTC peer to peer connection given those two connections.
//
// The browser webSocket must initiates the starting of the spectrum feed
// program via this code.
//




// The browser webSocket must initiates the starting of the spectrum feed
// program.  init() is called to setup this event.
//
function init(ws/*browser webSocket connection*/) {

    var id = connectCount++;

    // List of Peer to Peer spectrum feeds
    var spectrumFeeds = { };

    // TODO: remove this stupid print.
    ws.spew("Adding Peer to Peer SpectrumFeed code for " + id);


    // Event from browser webSocket to start a spectrum feed program.
    //
    // See On('startSpectrumFeed',...) in crts_server.js for how it is
    // done without webRTC.
    //
    ws.On('startPPSpectrumFeed', function(freq, bandwidth, bins, rate,
        device, host, tag) {

        // TODO: ya more here.

        ws.spew("Got 'startPPSpectrumFeed' with (" + arguments + ")");

        spectrumFeeds[tag] = {
            freq: freq,
            bandwidth: bandwidth,
            bins: bins,
            rate: rate,
            device: device,
            host: host,
    
            // TODO: more here...

            cleanup: function() {
                ws.spew("Cleaning up PP spectrum feed with tag: " + tag);
                // TODO: cleanup
            }
        };

        // TODO: launch the Peer to Peer spectrum feeder program here...

    });


    // If the spectrum feeder program uses WebSockets we can use this
    // event to connect to it here.
    ws.On('feederSpectrumFeed', function(tag) {

        ws.spew("PP spectrum feeder has connected with tag: " + tag);


        // TODO: write this ...
        ws.Emit('updatePPSpectrumFeed', 'HERE IT COMES');

    });


    // Register an onclose handler so we may kill all the feed programs
    // and cleanup in general.
    ws.OnClose(function() {

        Object.keys(spectrumFeeds).forEach(function(tag) {
            spectrumFeeds[tag].cleanup();
        });
    });
}


console.log("loading Kalyani's webRTC spectrum feed module");



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
