#!/usr/bin/env node

var connectCount = 0; // this number only increases.

var path = require('path'),
    fs = require('fs');


// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');

const maxMessageLength = 1024*512;

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

    var totalMessage = '';

    var connection = connections[id] = {

        // We get this description after the TCP/IP connection
        // sends us the launch ID.  We need to wait for a message
        // to get information about this program that was launched.
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

        spew("Got message: " + JSON.stringify(obj));
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
