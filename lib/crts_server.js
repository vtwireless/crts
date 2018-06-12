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


var express = require('express'),
    webSocket = require('ws'),
    app = express();

const { spawn } = require('child_process');

const doc_root = path.normalize(
        path.join(__dirname, "..", "htdocs")),
    spectrumProgram = path.normalize(
        path.join(__dirname, "..", "bin", "crts_spectrumSensing"));


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

// The spectrum TCP connections may exist a little longer than the
// webSocket connects, so we need to keep them in a separate list:
var spectrums = {};



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

        // for socket.IO like interface
        var args = [].slice.call(arguments);
        var name = args.shift();
        ws.send(JSON.stringify({ name: name, args: args }));
    }

    function On(name, func) {

        assert(onCalls[name] === undefined, pre +
            "setting on('" + name + "', ...) callback a second time." +
            " Do you want to override the current callback or " +
            "add an additional callback to '" + name +
            "'.  You need to edit this code.");
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

    // List of spectrums for this web socket with id.
    spectrums[id] = {}; // created in On('launchSpectrumSensing',...)
    spectrums[id].webSocket = ws;

    Emit('init', id);

    On('init', function(message) { spew('got "init": ' + message); });

    On('stopSpectrumSensing', function(tag) {

        if(spectrums[id][tag] !== undefined) {
            spew('Marking child with pid ' + spectrums[id][tag].pid);
            spectrums[id][tag].shutdown();
            // Now any TCP spectrumSensing connect with this WebSocket id
            // and tag tag is marked to be closed.
        } else {
            spew("spectrum [" + tag + "]=" + spectrums[id][tag]);
        }
    });

    On('launchSpectrumSensing', function(freq, bandwidth, bins, rate,
        device, host, tag) {

        // TODO: check that the arguments are valid.

        if(spectrums[id][tag] !== undefined) {
            spew(spectrumProgram + ' is already running with tag: ' + tag);
            return;
        }

        // TODO: launch ../bin/spectrumSensing

        if(host.length > 0) {
            var arg0 = 'ssh';
            var args = [ host, spectrumProgram ];
            //var args = [ host, spectrumProgram ];
        } else {
            var arg0 = spectrumProgram;
            var args = [ ];
        }
        
        args = args.concat( [
            '--freq', freq.toString(),
            '--bandwidth', bandwidth.toString(),
            '--bins', bins.toString(),
            '--rate', rate.toString(),
            '--device', device,
            '--tag', tag,
            '--user_id', id
        ]);

        spew('running: ' + arg0 + ' ' + args.toString());

        var child = spawn(arg0, args, { stdio: 'inherit' });

        if(child.pid > 0) {
            // Save a copy of the pid.  They seem to change it
            // after the child exits.
            spectrums[id][tag] = {
                pid: child.pid,
                connected: false,
                running: true,
                host: host,
                device: device,
                shutdown: function() {
                     spew('Shutting down spectrum child with pid ' +
                         this.pid);
                    if(this.connected === false) {
                        delete spectrums[id][tag];
                    }
                    this.running = false;
                    // If there is a connected client TCP socket the
                    // socket will see that this object, spectrums[id][tag]
                    // no longer is valid and so when the next data is
                    // received it will destroy the connection which will
                    // make spectrumProgram exit.
                }
            };

            child.Cleanup = function() {
                if(spectrums[id][tag] !== undefined)
                    spectrums[id][tag].shutdown();
            };
        }

        spew('child.pid=' + child.pid);


        child.on('error', function(err) {
            console.log('Failed to start program: ' +
                arg0 + ' ' + args.toString() + ' ' + err);
            this.Cleanup();
        });

        child.on('close', function(code) {
            console.log('spectrumSensing process ' +
                this.Pid + ' exited with code ' + code);
            this.Cleanup();
        });

    });

    ws.on('close', function() {

        delete spectrums[id].webSocket;
        // kill any spectrumSensing programs that this
        // webSocket spawned.
        const keys = Object.keys(spectrums[id]);
        for(let i=0; i<keys.length; ++i)
            spectrums[id][keys[i]].shutdown();
        spew('closed');
    });
}


const util = require('util');

if(opt.spectrum_port !== 0) {

    // TODO: Get rid of this TCP crap and use WebRTC instead and let the
    // spectrum service have a peer to peer connection where one peer is
    // the browser and one is the program that reads the USRP spectrum
    // via libUHD.
    //
    // The program connected to the other side of this TCP/IP socket
    // should be
    //               ../bin/spectrumSensing
    //

    var spectrumServer = require('net').createServer(function(socket) {

        // socket is a client TCP/IP socket and not a webSocket.

        var address = socket.remoteAddress + ' :' +
            socket.remotePort;

        socket.setEncoding('utf8');

        var spew = console.log.bind(console, 'spectrum from ' + address);

        spew();


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
            //
            var obj = JSON.parse(message);

            if(obj.args === undefined || obj.args.length !== 3) {
                spew('WARNING: got missaligned spectrum message: ' +
                    message);
                return;
            }


            //console.log("TCP socket from: " + address +
            //    ": Got message: " + message);
            //

            var id = obj.args[0];
            var tag = obj.args[1];
            if(spectrums[id] !== undefined &&
                    spectrums[id][tag] !== undefined &&
                    spectrums[id][tag].running &&
                    spectrums[id].webSocket !== undefined) {

                if(spectrums[id][tag].connected === false) {
                    spectrums[id][tag].connected = true;
                    spew = console.log.bind(console,
                        spectrums[id].webSocket.Prefix +
                        'spectrum from ' + address);
                    socket.Spectrum = spectrums[id][tag];
                }

                spectrums[id].webSocket.send(message);
                //console.log('sent=' + sdata);
            } else {
                spew('dead WebSocket[' + id + '] ' +
                    'destroying TCP socket');
                socket.destroy();

                // The closed socket will cause the program to exit
                // and so the child process will automatically cleanup.
            }
        });

        socket.on('close', function() {

            if(socket.Spectrum !== undefined) {
                // This will un-reserve the tag so that the web
                // browser may now create a spectrum service with that
                // tag again.
                socket.Spectrum.connected = false;
                socket.Spectrum.shutdown();
                delete socket.Spectrum;
            }

            spew('closed');
        });

    }).listen(opt.spectrum_port, function() {

        console.log("listening for spectrum service on port " +
            opt.spectrum_port);
    });
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
