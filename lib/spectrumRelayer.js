#!/usr/bin/env node

var connectCount = 0; // this number only increases.

var path = require('path'),
    fs = require('fs');


// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');


const spectrumProgram = path.normalize(
        path.join(__dirname, "..", "bin", "crts_spectrumFeed"));


const { spawn } = require('child_process');



var spectrumFeeds = {};  // public interface methods

var webSockets = {}; // list of webSocket IO like thingys

var connections = {}; // list of spectrumFeed TCP connections


spectrumFeeds.addWebSocket = function(io, id, spew) {

    webSockets[id] = {
        io: io,
        id: id,
        spew: spew
    };

    Object.keys(connections).forEach(function(connectionId) {

        // TODO:  ?????

    });


    spew('added webSocket to spectrumFeeds');
};


spectrumFeeds.removeWebSocket = function(ws, id) {

    if(webSockets[id] !== undefined) {

        webSockets[id].spew('removing webSocket from spectrumFeeds');
        delete webSockets[id];
    }
};

function onConnection(socket) {

    // socket is a client TCP/IP socket and not a webSocket.

    var address = socket.remoteAddress + ' :' + socket.remotePort;

    socket.setEncoding('utf8');

    var id = connectCount++;

    var spew = console.log.bind(console, 'spectrum feed[' +
            id + '] from: ' + address);

    var connection = connections[id] = {

        destroy: function() {
            spew("destroy()");
            if(connections[id] !== undefined) {

                delete connections[id];
            }
        }
    };

    spew('connected');

    socket.on('data', function(message) {

        if(message.charAt(0) !== '{' ||
                message.charAt(message.length-1) !== '}') {
                spew('WARNING: got missaligned spectrum message: ' +
                    message);
                return;
        }

        // TODO: this seems a little wasteful to parse the whole thing
        // given we only need to look at the first to objects in the
        // JSON string.
        // 
        //
        try {
            var obj = JSON.parse(message);
        } catch {
            spew('got bad message: ' + message);
            return;
        };

        if(obj.args === undefined || obj.args.length !== 3) {
            spew('WARNING: got missaligned spectrum message: ' +
                message);
            return;
        }

        spew("Got message: " + message);
    });

    socket.on('close', function() {

        spew('closed');
        connection.destroy();
    });
}



function init() {

    console.log('initializing ' + path.basename(__filename));

    require('net').createServer(onConnection).
        listen(opt.spectrum_port, function() {
            console.log("listening for spectrum feed service on port " +
                opt.spectrum_port);
    });

    return spectrumFeeds;
}


console.log('loaded ' + path.basename(__filename));

//
// None of the code in this file is seen in other files except
// module.exports.
//
module.exports = init;
