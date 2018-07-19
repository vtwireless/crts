#!/usr/bin/env node


"use strict";


var path = require('path'),
    fs = require('fs'),
    process = require('process');


// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');


if(opt.debug) {
    process.env.DEBUG = "express:*";
}


if(opt.http_port === opt.https_port) {
    console.log("Failed start: both the http and the https ports are " +
            opt.https_port);
    process.exit(1);
}

var webrtcSpectrum, spectrum_feed, tcpLaunchRelay;

if(opt.webrtc)
    // Add Kalyani's webRTC code for peer to peer spectrum feed
    var webrtcSpectrum = require('./webrtcSpectrum');

if(opt.spectrum_feed)
    var spectrum_feed = require('./spectrum_feed');

if(opt.radio_port !== 0)
    var tcpLaunchRelay = require('./tcpLaunchRelay');



var express = require('express'),
    webSocket = require('ws'),
    app = express();



const { spawn } = require('child_process');

const doc_root = path.normalize(
        path.join(__dirname, "..", "htdocs")),
    findUSRPProgram = path.normalize(
        path.join(__dirname, "..", "bin", "crts_findUSRPs"));


console.log("Set document root to: " + doc_root);


// configure routes
app.get('/', function (req, res) {
    res.sendFile(path.join(doc_root, 'index.html'));
});

app.use(express.static(doc_root));

function fail() {

    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    const line = '\n--------------------------------------------------------';
    text += line + '\nCALL STACK' + line + '\n' +
        new Error().stack + line;
    console.log(text);
    if(opt.debug)
        throw text;
}


function assert(val, msg=null) {

    if(!val) {
        if(msg)
            fail(msg);
        else
            fail("JavaScript failed");
    }
}


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

        let obj = JSON.parse(message);
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


    if(webrtcSpectrum !== undefined)
        // Add Kalyani's webRTC code for peer to peer spectrum feed
        webrtcSpectrum.init(ws);

    if(spectrum_feed !== undefined)
        spectrum_feed.init(ws);

    if(tcpLaunchRelay !== undefined)
        tcpLaunchRelay.init(ws);
}


if(opt.http_port !== 0) {

    var http = require('http');
    var server = http.createServer(app);
    var ws = new webSocket.Server({server: server});
    ws.on('connection', onWebSocketConnect);

    server.listen(opt.http_port, 'localhost', function() {
        console.log("http server listening on port " +
            opt.http_port);
    });
}

if(opt.https_port !== 0)
{
    const keys_dir = path.normalize(
        path.join(__dirname, "..", "etc")
    );
    const options = {
        key : fs.readFileSync(path.join(keys_dir, 'private_key.pem')),
        cert: fs.readFileSync(path.join(keys_dir, 'public_cert.pem'))
    };

    var https = require('https');
    var server = https.createServer(options, app);

    var ws = new webSocket.Server({server: server});
    ws.on('connection', onWebSocketConnect);

    server.listen(opt.https_port, function() {
        console.log("https server listening on port " +
            opt.https_port);
    });
}
