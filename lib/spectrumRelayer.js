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


// This is called anytime a webSocket connects to the server.
//
spectrumFeeds.addWebSocket = function(io, id, spew) {

    // id is the webSocket id.
    webSockets[id] = {
        io: io,
        id: id,
        spew: spew
    };

    io.On('spectrumSubscribe', function(spectrumId) {
        if(connections[spectrumId] !== undefined) {
            spew('spectrumSubscribe[' + spectrumId + '] webSocket id=' + id);
            connections[spectrumId].webSocketIOs[id] = io;
        }
    });

    io.On('spectrumUnsubscribe', function(spectrumId) {
        if(connections[spectrumId] !== undefined) {
            spew('spectrumUnsubscribe[' + spectrumId + '] webSocket id=' + id);
            delete connections[spectrumId].webSocketIOs[id];
        }
    });


    Object.keys(connections).forEach(function(connectionId) {
        var connection = connections[connectionId];
        if(connection.started) {
            // Tell this new webSocket client about this spectrum feed; so
            // they may subscribe if they like.
            //
            // TODO: user based access control???
            io.Emit('newSpectrum', connectionId, connection.description);
        }
    });

    spew('told webSocket about all running spectrumFeeds');
};


spectrumFeeds.removeWebSocket = function(ws, id) {

    if(webSockets[id] !== undefined) {
        webSockets[id].spew('removing webSocket from spectrumFeeds');
        delete webSockets[id];
    }
    Object.keys(connections).forEach(function(connectionId) {
        let connection = connections[connectionId];
        if(connection.webSocketIOs[id] !== undefined)
            delete connection.webSocketIOs[id];
    });
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
        // From this socket connection.
        require('./socketIO')(function(msg) { socket.write(msg); }, spew);


    io.On("spectrumUpdate", function(cFreq, bandwidth, bins,
            updatePeriod, data) {

        /*console.log('got spectrumFeed(' + id + '): ' +
            ' cFreq=' + cFreq + ' bandwidth=' + bandwidth +
            ' bins=' + bins + ' period= +  updatePeriod +
            ' amplitudes=' + data + "\n\n");
            */

        Object.keys(connection.webSocketIOs).forEach(function(wsId) {

            connection.webSocketIOs[wsId].Emit("spectrumUpdate", id,
                    cFreq, bandwidth, bins, updatePeriod, data);
        });
    });


    var totalMessage = '';
    var connection = connections[id] = {

        started: false,
        destroy: function() {
            spew("destroy()");
            if(connections[id] !== undefined) {
                var webSocketIOs = connection.webSocketIOs;
                Object.keys(webSocketIOs).forEach(function(wsId) {
                    webSocketIOs[wsId].Emit('spectrumEnd', id);
                });
                delete connections[id];
            }
        },
        // list of this spectrum subscribers:
        webSocketIOs: {}
    };

    io.On('spectrumStart', function(description) {
        // Tell the webSockets about this new spectrum feed.
        Object.keys(webSockets).forEach(function(wsId) {
            let io = webSockets[wsId].io;
            // Spew the 'newSpectrum' too all the webSocket that currently
            // exist.
            io.Emit('newSpectrum', id, description);
        });
        spew("'spectrumStart'  description=" + description);

        connections[id].description = description;
        connections[id].started = true;
    });

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
