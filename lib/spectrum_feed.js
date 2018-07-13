#!/usr/bin/env node

var connectCount = 0; // this number only increases.

var path = require('path'),
    fs = require('fs'),
    process = require('process');


// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');


const spectrumProgram = path.normalize(
        path.join(__dirname, "..", "bin", "crts_spectrumFeed"));


const { spawn } = require('child_process');

console.log("loaded 'spectrum_feed'");


// The spectrum TCP connections may exist a little longer than the
// webSocket connects, so we need to keep them in a separate list:
var spectrumFeeds = {};



function init(ws) {

    var id = ws.Id;
    var spew = ws.spew;
    var On = ws.On;
    var Emit = ws.Emit;

    // List of spectrum feeds for this web socket with id.
    spectrumFeeds[id] = {}; // created in On('launchSpectrumSensing',...)
    // export this webSocket to the TCP/IP server below.
    spectrumFeeds[id].webSocket = ws;

    // Add a webSocket close cleanup function.
    ws.OnClose(function() {
        delete spectrumFeeds[id].webSocket;
        const keys = Object.keys(spectrumFeeds[id]);
        for(let i=0; i<keys.length; ++i)
            spectrumFeeds[id][keys[i]].shutdown();
        delete spectrumFeeds[id];
    });

    On('destroySpectrumFeed', function(tag) {

        if(spectrumFeeds[id][tag] !== undefined) {
            spew('Marking child with pid ' + spectrumFeeds[id][tag].pid);
            spectrumFeeds[id][tag].shutdown();
            // Now any TCP spectrumSensing connection with this WebSocket
            // id and tag tag is marked to be closed.
        } else
            spew("destroySpectrumFeed [tag=" + tag + "] bad tag");
    });


    On('stopSpectrumFeed', function(tag) {

        // For now this is like On('destroySpectrumFeed',...)
        //
        if(spectrumFeeds[id][tag] !== undefined) {
            spew('Marking child with pid ' + spectrumFeeds[id][tag].pid);
            spectrumFeeds[id][tag].shutdown();
            // Now any TCP spectrumSensing connection with this WebSocket
            // id and tag tag is marked to be closed.
        } else
            spew("stopSpectrumFeed [tag=" + tag + "] bad tag");
    });


    On('setSpectrumFeed', function(tag, freq, bandwidth, bins, updateRate) {

        if(spectrumFeeds[id][tag] === undefined) {
            spew("setSpectrumFeed [tag=" + tag + "] bad tag");
            return;
        }

        var sf = spectrumFeeds[id][tag];

        var parameters = JSON.stringify({
            freq: freq,
            bandwidth: bandwidth,
            bins: bins,
            updateRate: updateRate
        });

        if(sf.tcpSocket === false) {
            // Save the parameters to use to set when we get a tcpSocket,
            // if we do.
            sf.parametersChanged = parameters;
            spew('setSpectrumFeed: ' + sf.parametersChanged);
        } else {
            // send the command to the TCP socket.
            sf.tcpSocket.write(parameters);
            sf.parametersChanged = false;
        }
    });

    On('startSpectrumFeed', function(freq, bandwidth, bins, rate,
        device, host, port, tag) {

        // TODO: check that the arguments are valid.

        if(spectrumFeeds[id][tag] !== undefined) {
            spew(spectrumProgram + ' is already running with tag: ' + tag);
            return;
        }

        // TODO: launch ../bin/crts_spectrumFeed

        if(host && host.length > 0) {
            var arg0 = 'ssh';
            if(port)
                var args = [ '-p', port.toString(), host, spectrumProgram ];
            else
                var args = [ host, spectrumProgram ];
        } else {
            var arg0 = spectrumProgram;
            var args = [ ];
        }

        args = args.concat( [
            '--freq', freq.toString(),
            '--bandwidth', bandwidth.toString(),
            '--bins', bins.toString(),
            '--rate', rate.toString(),
            '--address', opt.server_address,
            '--device', device,
            '--tag', tag,
            '--port', opt.spectrum_port,
            '--user_id', id
        ]);

        spew('running: ' + arg0 + ' ' + args.toString());

        var child = spawn(arg0, args, { stdio: 'inherit' });

        if(child.pid > 0) {
            // Save a copy of the pid.  They seem to change it
            // after the child exits.
            spectrumFeeds[id][tag] = {
                pid: child.pid,
                tcpSocket: false,
                parametersChanged: false,
                running: true,
                host: host,
                device: device,
                shutdown: function() {
                     spew('Shutting down spectrum child with pid ' +
                         this.pid);
                    if(spectrumFeeds[id] !== undefined &&
                        spectrumFeeds[id][tag] !== undefined)
                        delete spectrumFeeds[id][tag];

                    this.running = false;
                    // If there is a connected client TCP socket the
                    // socket will see that this object, spectrumFeeds[id][tag]
                    // no longer is valid and so when the next data is
                    // received it will destroy the connection which will
                    // make spectrumProgram exit.
                }
            };

            child.Cleanup = function() {
                if(spectrumFeeds[id] !== undefined &&
                    spectrumFeeds[id][tag] !== undefined) {
                    Emit('stopSpectrumFeed', tag);
                    spectrumFeeds[id][tag].shutdown();
                }
            };
        }

        // WTF: child loses the pid after it exits, though the child
        // object still exists.  For debugging we need the pid.
        child.Pid = child.pid;

        child.on('error', function(err) {
            console.log('Failed to start program: ' +
                arg0 + ' ' + args.toString() + ' ' + err);
            this.Cleanup();
        });

        child.on('close', function(code) {
            console.log('spectrumFeed process ' +
                this.Pid + ' exited with code ' + code);
            this.Cleanup();
        });

    });
}


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

        var spew = console.log.bind(console, 'spectrum feed from ' + address);

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


            //console.log("TCP socket from: " + address +
            //    ": Got message: " + message);
            //

            var id = obj.args[0];
            var tag = obj.args[1];
            if(spectrumFeeds[id] !== undefined &&
                    spectrumFeeds[id][tag] !== undefined &&
                    spectrumFeeds[id][tag].running &&
                    spectrumFeeds[id].webSocket !== undefined) {

                let sf = spectrumFeeds[id][tag];

                if(sf.tcpSocket === false) {
                    sf.tcpSocket = socket;
                    spew = console.log.bind(console,
                        spectrumFeeds[id].webSocket.Prefix +
                        'spectrum from ' + address);
                    socket.Spectrum = sf;
                    sf.tcpSocket = socket;
                    if(sf.parametersChanged !== false) {
                        spew('sending: ' + sf.parametersChanged);
                        socket.write(sf.parametersChanged);
                        sf.parametersChanged = false;
                    }
                }

                spectrumFeeds[id].webSocket.send(message);
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
                socket.Spectrum.tcpSocket = false;
                socket.Spectrum.shutdown();
                delete socket.Spectrum;
            }

            spew('closed');
        });

    }).listen(opt.spectrum_port, function() {

        console.log("listening for spectrum feed service on port " +
            opt.spectrum_port);
    });
}


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
