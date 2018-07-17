#!/usr/bin/env node

var sessionCount = 0; // this number only increases.

var path = require('path'),
    fs = require('fs');

// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');

var modName = path.basename(__filename);
const { spawn } = require('child_process');


console.log("loaded: '" + modName + '"');

// List of sessions.  So that the TCP server below can share data
// with the init() function.
//
sessions = {};


// We initialize this code for each webSocket connection that calls
// this.  We are letting the webSocket connections be one-to-one
// with a what could be considered a web session.
//
function init(ws) {

    // This is the body of actions associated with a given browser
    // webSocket connection.  It sets up a "session" context (javaScript
    // scope).
    //
    var id = ws.Id;
    var spew = ws.spew;
    var On = ws.On;
    var Emit = ws.Emit;
    var sessionId = sessionCount++;

    // Add a session object so that we can share data with
    // the TCP server below.
    //
    var session = sessions[sessionId] = {

        // list of TCP connections.
        //
        tcpConnections: {}
    };

    function destroy() {
        spew('closing ' + modName);

        // Remove any tcp connections.
        Object.keys(session.tcpConnections).forEach(function(key) {
            session.tcpConnections[key].destroy();
        });
    }

    // Add a webSocket close cleanup function.
    //
    ws.OnClose(function() {
        destroy();
    });


    spew('created ' + modName + ' session');
}


// TODO: Get rid of this TCP crap and use WebRTC instead and let this
// service have a peer to peer connection where one peer is the browser
// and one is the crts_radio program.
//
// The program connected to the other side of this TCP/IP socket
// should be
//               ../bin/crts_radio
//




require('net').createServer(function(socket) {

    // socket is a client TCP/IP socket and not a WebSocket.
    //
    // This is a TCP connection.

    var address = socket.remoteAddress + ' :' +
        socket.remotePort;

    socket.setEncoding('utf8');

    var totalMessage = '';

    var sessionId = false;

    var spew = console.log.bind(console, 'crts controller from ' + address);

    spew('connected');

    function destroy() {

        if(sessionId !== false) {
            delete sessions[sessionId];
            sessionId = false;
        }

        socket.destroy();
    }

    socket.on('data', function(message) {

        // TODO: prepend old messages if they fail to JSON parse.

        function fspew(s) {
            spew('WARNING: ' + s + ': message="' + message + '"');
        }

        // TODO: find '}{' break the total Message with '}' and '{'
        //
        //
        //
        //
        // TODO: totalMessage must start with '{'.


        // TCP/IP does not guarantee that we have the whole message,
        // so there may be more added in later reads.

        totalMessage += message;

        if(totalMessage.substr(0,1) !== '{') {
            fspew('missing leading "{"');
            destroy();
            return;
        }

        try {
            var obj = JSON.parse(totalMessage);
        } catch {
            fspew('JSON.parse() failed');
            return;
        };

        // Now we have a JSON object so we will start looking for a new
        // message in the future reads.
        //
        totalMessage = '';

        if(obj.msg === undefined || obj.id === undefined) {
            fspew('bad message');
            destroy();
            return;
        }

        if(sessions[obj.id] === undefined) {

            // The webSocket/session must exist or else there is no client
            // that we are working for.  In this case it does not, so:
            //
            fspew('session with ID: ' + obj.id + ' not found');
            // just drop the connection.
            destroy();
            return;
        }

        console.log("TCP socket from: " + address +
            ": Got message: " + message);

        // The sessions[] is seen by the above init() function so that the
        // WebSocket/session thingy can call destroy() when the webSocket
        // closes.
        //
        sessions[sessionId = obj.id] = { destroy: destroy };
    });


    socket.on('close', function() {

        if(sessionId !== false) {
            delete sessions[sessionId];
            sessionId = false;
        }
        spew('closed');
    });


}).listen(opt.radio_port, function() {

        console.log("listening for crts_radio controller service on port " + opt.radio_port);
});


// Let the module user just see what's in module.exports.  You can have
// your room (module) as messy as you like but don't let others see the
// mess (via module.exports).
//
// None of the code in this file is seen in other files except
// module.exports.
//
module.exports = {
    init: init
};
