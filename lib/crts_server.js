#!/usr/bin/env node


"use strict";


var path = require('path'),
    fs = require('fs'),
    process = require('process'),
    url = require('url');



// This util.js creates the global functions:
//
//   fail('', '', '', ..)  and  assert(bool, '', '', ...)
//
//
require('./util');


// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');


var webrtcSpectrum, spectrum_feed, launchTcpRelay;

if(opt.webrtc)
    // Add Kalyani's webRTC code for peer to peer spectrum feed
    webrtcSpectrum = require('./kg_webrtc');

if(opt.spectrum_feed)
    spectrum_feed = require('./spectrum_feed');

if(opt.radio_port !== 0)
    launchTcpRelay = require('./launchTcpRelay');



var authenticate = false;


// TODO: More authentication stuff.  This is a very simple
// start.
//
//
// Change this one line of code to change how we authenticate.
// It's in the one file.
//
authenticate = require('./authenticateSimple').authenticate;



var express = require('express'),
    webSocket = require('ws'),
    app = express();



const { spawn } = require('child_process');

const doc_root = path.normalize(
        path.join(__dirname, "..", "htdocs")),
    findUSRPProgram = path.normalize(
        path.join(__dirname, "..", "bin", "crts_findUSRPs"));


console.log("Set document root to: " + doc_root);


function sendMimeType(req, res) {


    var extname = path.extname(url.parse(req.url).pathname);

    // reference:
    // https://developer.mozilla.org/en-US/docs/Learn/Server-side/Node_server_without_framework

    // Why does express not do this mimeTypes for me.  I'm a little put
    // off by how little express does.
    //
    var mimeTypes = {
        '.html': 'text/html',
        '.js': 'text/javascript',
        '.css': 'text/css',
        '.json': 'application/json',
        '.png': 'image/png',
        '.jpg': 'image/jpg',
        '.gif': 'image/gif',
        '.svg': 'application/image/svg+xml',
        '.ico': 'image/x-icon'
    };

    var contentType = mimeTypes[extname] || 'text/html';

    console.log("filePath='" + url.parse(req.url).pathname +
        "'  ext=" + extname + ' contentType=' + contentType);

    res.setHeader('Content-Type', contentType);
    console.log("set mime type to: " + contentType);
}


// This is the express catch all callback...??
//
// All requests must pass through this, so I think.
//
app.get('*', function(req, res, next) {

    if(req.socket.localPort === opt.http_port) {
        // Let the localhost work without requiring
        // authentication.
        sendMimeType(req, res);
        // The fast success case.
        next();
        return;
    }

    var auth = authenticate(req, res);

    if(auth.success && auth.method === 'cookie') {
        sendMimeType(req, res);
        // The fast success case.
        next();
        return;
    }

    // else // auth.method === 'query passcode'

    // We delay response so that they cannot keep guessing quickly.  By
    // not responding quickly to either a correct passcode and an
    // incorrect passcode they also cannot close the connection assuming
    // that the guess was incorrect due to the delay.  In order to get
    // good client server response they must except cookies.

    setTimeout(function() {
        if(auth.success) {
            sendMimeType(req, res);
            next();
            return;
        } else {
            res.send("\nBad passcode\n\n");
            res.end();
        }

        // This means the time to guess the correct passcode will be this
        // time times the number of possible passcodes, divided by 2,
        // divided by the number of simultaneous connections.  That should
        // be a long time, like a thousand years.

    }, 200/*milliseconds*/);
});


// configure express routes
app.get('/', function (req, res) {

    res.sendFile(path.join(doc_root, 'index.html'));
});

// configure express routes
app.use(express.static(doc_root));


// This counter, clientCount, only increases.
//
var clientCount = 0;


function onWebSocketConnect(ws) {

    // spew() is just a object local console.log() wrapper to keep prints
    // starting the same prefix for a given websocket connection.
    var id = clientCount++;
    var pre = ws.Prefix = 'WebSocket[' + id +
        '](' + ws._socket.remoteAddress + ' :' +
        ws._socket.remotePort + '):';
    var spew = console.log.bind(console, pre);


    /////////////////////////////////////////////////////////////
    //  BEGIN: Making Socket.IO like interfaces: On() Emit()
    /////////////////////////////////////////////////////////////

    // The list of action functions for this websocket.
    var onCalls = {};

    function Emit(name, data) {

        if(ws.readyState !== webSocket.OPEN) {
            // With this error check we do not need error check
            // before each call to Emit().
            spew("Cannot call Emit('" + name + "', ...)" +
                " because the webSocket is not open anymore");
            // We cannot send if it's not in readyState OPEN
            return;
        }

        // for socket.IO emit() like interface, but we call it
        // Emit() to avoid conflict.
        var args = [].slice.call(arguments);
        var name = args.shift();
        ws.send(JSON.stringify({ name: name, args: args }));
    }

    function On(name, func) {

        assert(onCalls[name] === undefined, pre +
            "setting On('" + name + "', ...) callback a second time." +
            " Do you want to override the current callback or " +
            "add an additional callback to '" + name +
            "'.  You need to edit this code.");
        spew('added On("' + name + '", function())');
        onCalls[name] = func;
    };

    ws.on('message',  function(message) {

        spew("got message: " + message);

        try {
            var obj = JSON.parse(message);
        } catch(e) {

            spew('message was not JSON');
            return;
        }

        let name = obj.name; // callback name (not subscription name)

        // We should have this form:
        // e.data = { name: eventName, args:  [ {}, {}, {}, ... ] }
        if(name === undefined || obj.args === undefined ||
                !(obj.args instanceof Array)) {
            fail(pre + 'Bad "on" message from server\n  ' +
                message);
        }

        if(onCalls[name] === undefined)
            fail(pre + 'message callback "' + name +
                    '" not found for message: ' +
                    '\n  ' + message);

        spew('handling message=\n   ' + message);

        (onCalls[name])(...obj.args);
    });

    /////////////////////////////////////////////////////////////
    //  END: Making Socket.IO like interfaces:  On() Emit()
    /////////////////////////////////////////////////////////////


    spew('connected');

    Emit('init', id);

    On('init', function(message) { spew('got "init": ' + message); });


    On('getUSRPsList', function(hosts) {

        // hosts should be an array of strings with no duplicates.
        // TODO: Check the hosts object.
        //
        //
        // Examples:
        //
        //    hosts = [""]
        //
        //    hosts = [
        //               {"host": "191.87.19.241", "port":"4000"},
        //               {"host": "" }
        //            ]
        //

        var results = {};
        var remain = hosts.length;

        hosts.forEach(function(host) {

            var text = '';
            var program;
            var args;
            var hosT = '';
            var port = '';

            if(host === '' || host.host === '') {
                program = findUSRPProgram;
                args = [];
            } else if(typeof host === 'string') {
                program = 'ssh';
                args = [ host, findUSRPProgram ];
                hosT = host;
            } else if(host.host !== undefined &&
                typeof host.host === 'string') {
                program = 'ssh';
                args = [];
                hosT = host.host;
                if(host.port !== undefined &&
                    typeof host.port === 'string') {
                    // Add ssh option: -p PORT
                    args.push('-p');
                    args.push(host.port);
                    port = host.port;
                }
                args.push(host.host);
                args.push(findUSRPProgram);
            }

            spew('spawn(program="' + program + '", [' + args + '])');

            var child = spawn(program, args, {
                stdio: [null, 'pipe', 'inherit'] });

            child.stdout.on('data', function(data) {
                text += data;
            });
            child.on('close', function(exitCode) {

                if(exitCode == 0) {
                    spew(text);
                    let par = JSON.parse(text);
                    if(Array.isArray(par)) {
                        par.forEach(function(result) {
                            assert(result.serial !== undefined,
                                "We have a USRP with no serial number");
                            assert(results[result.serial] === undefined,
                                "We have more than one USRP with serial=" +
                                result.serial);
                            result.host = hosT;
                            result.port = port;
                            results[result.serial] = result;
                        });
                    }
                }

                if(--remain === 0) {
                    spew(JSON.stringify(results));
                    // All the programs returned.
                    // Send the result to the webSocket client.
                    Emit('getUSRPsList', results);
                }
            });
        });
    });


    var oncloses = [];

    ws.OnClose = function(onclose) {
        oncloses.push(onclose);
    };


    ws.on('close', function() {

        oncloses.forEach(function(onclose) {
            onclose();
        });

        spew('closed');
    });


    // Let other code see these added webSocket things:
    //
    ws.Spew = spew;
    ws.On = On;
    ws.Emit = Emit;
    ws.Id = id;
    ws.assert = assert;


    if(webrtcSpectrum !== undefined)
        // Add Kalyani's webRTC code for peer to peer spectrum feed
        webrtcSpectrum.init(ws);

    if(spectrum_feed !== undefined)
        spectrum_feed.init(ws);

    if(launchTcpRelay !== undefined)
        launchTcpRelay.init(ws);
}


if(opt.http_port !== 0) {

    var http = require('http');
    var server = http.createServer(app);

    var ws = new webSocket.Server({server: server});
    ws.on('connection', onWebSocketConnect);

    //require('./mqttBroker').init(server, opt.http_port);
 
    server.listen(opt.http_port, 'localhost', function() {
        console.log("http server listening on port " +
            opt.http_port);
        console.log("Try: http://localhost" + ":" +
                opt.http_port + "/");
    });
}


if(opt.https_port !== 0) {

    var https = require('https');
    var server = https.createServer({
        key: fs.readFileSync(opt.tls.key),  // private_key
        cert: fs.readFileSync(opt.tls.cert) // public_cert
    }, app);

    
    //TODO: THIS.  HOW?? How do we not interfere with MQTT
    //
    var ws = new webSocket.Server({server: server});
    ws.on('connection', onWebSocketConnect);

    // Add the MQTT Broker service.
    //
    // MQTT, webSockets, and https all use the same TCP/IP connection
    // via the webSocket upgrade to the HTTP protocol.
    //
    //require('./mqttBroker').init(server, opt.https_port);
 
    server.listen(opt.https_port, function() {

        var hostname = require('os').hostname();
        var passcode = '';
        if(opt.passcode)
            passcode = "?passcode=" +
                require('./authenticateSimple').passcode;

        console.log("https server listening on port " +
            opt.https_port);

        console.log("Try: https://" + hostname + ":" +
                opt.https_port + "/" + passcode);
    });
}
