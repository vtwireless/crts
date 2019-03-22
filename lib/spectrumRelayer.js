#!/usr/bin/env node

// This file makes a singleton object, so yes it's limited to making one
// TCP/IP spectrum feed to the server.  Any number of feed clients may
// connect to it, but it only adds one server listening port.

var connectCount = 0; // this number only increases

var path = require('path'),
    fs = require('fs');


// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');

const maxMessageLength = 1024*512;

// We find the path to the crts_spectrumFeed client program wither this
// CRTS package be installed or be running from a source directory tree.
// The relative path to this the crts_spectrumFeed client program is the
// same either way.
const spectrumProgram = path.normalize(
        path.join(__dirname, "..", "bin", "crts_spectrumFeed"));


const { spawn } = require('child_process');


// This file makes a singleton object, so we just keep the object state in
// these 3 file scope variables.  This way there's less nesting of
// brackets, {}, when compared to making object scope variables.
//
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

        let connection = connections[connectionId];

        // Tell this new webSocket client about this spectrum feed;
        // so they may subscribe if they like.
        //
        // TODO: user based access control???

        io.On('newSpectrum', connectionId, connection.description);
    });

    spew('added webSocket to spectrumFeeds');
};


spectrumFeeds.removeWebSocket = function(ws, id) {

    if(webSockets[id] !== undefined) {

        webSockets[id].spew('removing webSocket from spectrumFeeds');
        delete webSockets[id];
    }
};


// socket is the TCP/IP spectrum feed client.
//
function onConnection(socket) {

    // socket is a client TCP/IP socket and not a webSocket.  It is a
    // TCP/IP connection from the python script ../bin/crts_spectrumFeed

    var address = socket.remoteAddress + ' :' + socket.remotePort;

    socket.setEncoding('utf8');

    var id = connectCount++;

    var spew = console.log.bind(console, 'spectrum feed[' +
            id + '] from: ' + address);


    var io = // io will be a socketIO like thingy
        require('./socketIO')(function(msg) { socket.write(msg); }, spew);


    io.On("spectrumUpdate", function(id, tag, cFreq, bandwidth, bins, data) {

        console.log('got spectrumFeed(' + id + ') with tag="' + tag + '" : ' +
            ' cFreq=' + cFreq + ' bandwidth=' + bandwidth +
            ' bins=' + bins + ' amplitudes=' + data + "\n\n");
    });


    var totalMessage = '';

    var connection = connections[id] = {

        // We get this description after the TCP/IP connection sends us
        // the launch ID.  We need to wait for a message to get
        // information about this program that was launched.
        description: '', // The human web client can read.

        destroy: function() {
            spew("destroy()");
            if(connections[id] !== undefined) {

                delete connections[id];
            }
        }
    };

    spew('connected');

    socket.on('data', function(message) {

        if(io.CheckForSocketIOMessage(message))
            // If it was a socketIO like message than it was handled in
            // CheckForSocketIOMessage().
            return;

        // TODO: There is nothing in this and the client code that is
        // sending to this that frames this message.  We could get two
        // JSON expressions in one message, or the first char may not
        // be a '{'.  It works because the frames are small and there
        // is not much message congestion in the kernel TCP/IP code.
        // Under TCP/IP heavy loads this code will fail.

        if(message.charAt(0) !== '{' ||
                message.charAt(message.length-1) !== '}') {
            spew('WARNING: got missaligned spectrum message: ' + message);
            return;
        }

        // TODO: this seems a little wasteful to parse the whole thing
        // given we only need to look at the first two objects in the
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
            spew('WARNING: got missaligned spectrum message: ' + message);
            return;
        }

        spew("Got message: " + JSON.stringify(obj));
    });

    socket.on('close', function() {

        spew('closed');
        connection.destroy();
    });
}



function init() {

    require('net').createServer(onConnection).
        listen(opt.spectrum_port, function() {
            console.log("listening for spectrum feed service on port " +
                opt.spectrum_port);
    });

    return spectrumFeeds; // return spectrumFeeds as the interface.
}


console.log('loading ' + path.basename(__filename));

//
// None of the code in this file is seen in other files except
// module.exports.
//
module.exports = init;
