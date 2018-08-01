#!/usr/bin/env node

// In Brief:
//
// This code launches requested programs and optionally connect a TCP/IP
// socket to the launched program.
//
//
// More:
//
// It requires that there be WebSocket connection that will be associated
// with the launch requesting browser client.  We call that WebSocket
// connection a client session.
//
// For each launched program there may be zero or one TCP/IP connections
// from that launched program.  This code relays the messages to and from
// the TCP/IP connection to and from the WebSocket.  The relayed messages
// are tagged with the ID of the launched program, so that the messages
// are all associated with a corresponding launched program.
//
// The HTTP served file crts.js provides a client javaScript API for this
// module.
//
//
// TODO: Replace this webSocket and TCP/IP relay shit with webSockets and
// webRTC.
//
// RANT: It's kind-of obvious that webRTC is designed to play a part in
// this, removing this "relay" thing, but the webRTC C++ API is a total
// pain to work with.  We are of the school that says quality source code
// does not have a right of passage.  Software build systems should not
// be a barrier to entry for the software developer, they should be
// completely automated.  Building a software package should not be a
// summer project for the software user, but Google's webRTC API package
// is that bad.  No software package should be harder to build than the
// Linux kernel which is easy to build.  Building Google's webRTC API
// package is stupid hard, WTF.  Google is the crap software bully that
// MicroSoft is; forcing sub-par standards down your throat.  There are
// gitHub projects dedicated to just building WebRTC, which is a measure
// that proves this point.  You don't see any such projects for the
// NodeJS, Apache, and so on, for they are easy to build and install.
// RANT ...
//
//


var path = require('path'),
    fs = require('fs');

// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');

var modName = path.basename(__filename);
modName = modName.substr(0, modName.length - 3); // remove .js
console.log("loaded: '" + modName + '"');

const { spawn } = require('child_process');



// List of sessions.  So that the TCP server below can share data
// with the init() function/class object thingy.
//
wsSessions = {};


// We must have a very indirect way to find programs that we launch,
// otherwise we'd have a very insecure server which could run any program
// on the server computer.
//
// This is where we can add launcher based on the clients browser path
// part of the URL or like info.  Things like url https://foo.com/~joe/
// with program = "prog03"
// would map to running /home/joe/public_html/crts_programs/prog03
//
// We could also add ssh to a host to run programs.
//
function getProgramPath(program, ws=null, whatever=null) {

    // TODO: MORE CODE HERE:
    //
    return program;
}



// We initialize this code for each webSocket connection that calls
// this.  We are letting the webSocket connections be one-to-one
// with a what could be considered a web session.
//
// init() session per webSocket
//
function init(ws) {

    // This is the body of actions associated with a given browser
    // webSocket connection.  It sets up a "session" context/scope
    // (javaScript scope).
    //
    var id = ws.Id; // We use this as the session ID
    var Spew = ws.Spew;
    var On = ws.On;
    var Emit = ws.Emit;
    var assert = ws.assert;

    assert(wsSessions[id] === undefined,
        "A webSocket connection with ID=" + id + " already exists");

    // Add a webSocket session object so that we can share data with the
    // TCP server connections below.
    //
    var wsSession = wsSessions[id] = {

        // List of launched programs:
        programs: {},

        // The webSocket that the tcp connections needs to
        // relay to and from.
        //
        ws: ws
    };


    function destroy() {
        Spew('closing ' + modName);

        // Try to make all the programs and connections for this session
        // exit.
        Object.keys(wsSession.programs).forEach(function(key) {
            if(wsSession.programs[key].destroy !== undefined)
                wsSession.programs[key].destroy();
        });

        // Remove this global data for this functions object/context.
        delete wsSessions[id];
    }

    // Add a webSocket close cleanup function for this functions
    // object/context.
    //
    ws.OnClose(function() {
        destroy();
    });

    Spew('created ' + modName + ' session');

    function launch(clientLaunchId, program, args /* array of arguments */, host, port) {

        if(wsSession.programs[clientLaunchId] !== undefined) {

            // We have to have the client use this ID (clientLaunchId) in
            // the future so we let them tell us it's value and then we do
            // not need to tell them the value.  If the client is stupid
            // about it at the start, it would not work in the future
            // anyway; so that's makes this a failure mode, which we
            // recover from by ignoring it here:
            Spew('got launch without new clientLaunchId=' +
                clientLaunchId);
            return;
        }

        var arg0 = getProgramPath(program);

        var env = process.env;

        // We add the webSocket session ID and client launch ID to the
        // child's environment so that the launched program can know
        // itself when they talk to the TCP/IP connection.

        // ID for this webSocket session:
        env.wsSessionId = id.toString();

        // The browser gave this ID for this particular program launch:
        env.clientLaunchId = clientLaunchId.toString();

        var child = spawn(arg0, args, { stdio: 'inherit', env: env });

        // Append to the spew for this 'launch' thingy.
        //
        var Spew = ws.Spew.bind(ws, 'spawn("' + arg0 +
            '", ...) pid=' + child.pid + ':');

        Spew("running program=" + program + ' args=[' + args + ']');

        // It would appear that node at least calls the system call
        // fork(2) before returning from above spawn(), because we now
        // have the child PID, at least in a success case.
        //
        if(child.pid < 1) {
            Spew('failed: fork failed');
            return;
        }

        // Make a launch program object that is shared with the
        // TCP/IP connection, if there is a TCP/IP connection:
        var program = wsSession.programs[clientLaunchId] = {

            pid: child.pid/*save the pid*/,
        };

        child.on('error', function(err) {
            // Any TCP/IP connection will brake when the program is no
            // longer running, so we don't need to act on the TCP
            // connection here.
            Spew('failed err=' + err);
            Emit('launch', clientLaunchId, 1, 'failed to run at all');
            delete wsSession.programs[clientLaunchId];
        });

        child.on('close', function(code) {
            // Any TCP/IP connection will brake when the program is no
            // longer running, so we don't need to act on the TCP
            // connection here.
            Spew(' exited with code ' + code);
            // TODO: use the last Emit() arg for something better.
            Emit('launch', clientLaunchId, code, ' exited with code ' + code);
            delete wsSession.programs[clientLaunchId];
        });
    }

    On('launch', launch);

    On('launchMessage' /* a message that is relayed to the launched program */,
            function(clientLaunchId) {

                if(wsSession.programs[clientLaunchId] === undefined) {
                    Spew("launchMessage clientLaunchId=" + clientLaunchId +
                        " was not found");
                    // This could just be a race condition, so it may be
                    // fine.
                    return;
                }
                if(wsSession.programs[clientLaunchId].connection === undefined) {
                    Spew("launchMessage clientLaunchId=" + clientLaunchId +
                        " connection was not found");
                    // This could just be a race condition, so it may be
                    // fine.
                    return;
                }

                var connection = wsSession.programs[clientLaunchId].connection;
                // Forward to the TCP connection as a JSON string with all
                // the arguments, kind of like a very crude form of RPC.
                //
                connection.send(JSON.stringify([].slice.call(arguments)));
    });
}


// TODO: Get rid of this TCP crap and use WebRTC instead and let this
// service have a peer to peer connection where one peer is the browser
// and one is the crts_radio or other launched program.
//
// The program connected to the other side of this TCP/IP socket
// should be
//               ../bin/crts_radio ...
//
// or other program running on a computer behind the servers firewall.
//

var tcpConnectionCount = 0;

const maxMessageLength = 1024*512;



require('net').createServer(function(socket) {

    // socket is a client TCP/IP socket and not a WebSocket.
    //
    // This is a TCP connection.

    var address = socket.remoteAddress + ' :' +
        socket.remotePort;

    socket.setEncoding('utf8');

    var totalMessage = '';

    // We limit are limited to one TCP connection per launched
    // program, so we have just one launch ID which we get from
    // the first message from the launched program.  We do not
    // know it yet:
    var clientLaunchId = false,
    // And so, we do not know what wsSession this is using until the other
    // end tells us in the first message.
        wsSession = false,
        wsSessionId = false,
        programs = false;

    var Spew = console.log.bind(console,
        'crts controller from ' + address + ': ');

    Spew('connected');

    function destroy() {
        socket.destroy();

        if(clientLaunchId !== false && programs[clientLaunchId].destroy !== undefined) {
            delete programs[clientLaunchId].destroy;
            clientLaunchId = false;
        }
    }

    socket.on('data', function(message) {

        // TODO: prepend old messages if they fail to JSON parse.

        function wspew(s) {
            Spew('WARNING: ' + s + ': message="' + message + '"');
        }

        // TODO: find '}{' break the total Message with '}' and '{',
        // which would be from two commands getting pushed together
        // into the one TCP read().
        //

        if(totalMessage.length + message.length > maxMessageLength) {
            wspew('maxMassageLength (' +  maxMessageLength +
                ') was exceeded');
            destroy();
            return;
        }

        // TCP/IP does not guarantee that we have the whole message,
        // so there may be more added in later reads.

        totalMessage += message;

        if(totalMessage.substr(0,1) !== '{') {
            wspew('missing leading "{"');
            destroy();
            return;
        }

        try {
            var obj = JSON.parse(totalMessage);
        } catch {
            wspew('JSON.parse() failed');
            // We may get this message appended to later.  So it
            // may get used later.
            return;
        };

        // Now we have a JSON object so we will start looking for a new
        // message in the future reads, so we reset the total message so
        // far to a zero length string.
        //
        totalMessage = '';





        if(obj.msg === undefined || obj.clientLaunchId === undefined ||
            obj.wsSessionId === undefined) {
            wspew('missing launch ID or payload or whatever obj=' +
                JSON.stringify(obj));
            destroy();
            return;
        }


        if(clientLaunchId === false) {

            // This is the first message and it must have this obj
            // with these keys "msg", "clientLaunchId", and "wsSessionId".
            //
            if(obj.msg === undefined || obj.clientLaunchId === undefined ||
                obj.wsSessionId === undefined) {
                wspew('missing launch ID or payload or whatever obj=' +
                    JSON.stringify(obj));
                destroy();
                return;
            }

            wsSessionId = obj.wsSessionId;
            wsSession = wsSessions[wsSessionId];

            // The wsSessions[].tcpConnections[] is seen by the above init()
            // function so that the WebSocket/session thingy can call
            // destroy() when the webSocket closes.
            //

            programs = wsSession.programs;
            clientLaunchId = obj.clientLaunchId;

            // Now we have just learned what client/session this is
            // connected with, so now we make the spew more customized for
            // this connection 
            Spew = wsSession.ws.Spew.bind(wsSession.ws,
                'crts controller [' + obj.clientLaunchId + '] from ' + address + ': ');

            // and tell the init() function that we are here by letting
            // there be a tcpConnection object:
            //
            programs[clientLaunchId] = {};
            programs[clientLaunchId].destroy = destroy;

        } else if(wsSessions[wsSessionId] === undefined) {

            // This may not be a big deal.  It just a race condition
            // where the webSocket closed, and the TCP/IP socket has
            // not closed yet.

            Spew("After webSocket closed, got message: " + message);
            destroy();
            return;
        }

        Spew("got message: " + message);

        // Forward the payload in 'msg' to the webSocket.  The receiving
        // browser client need to know what the associated TCP connection
        // is.  The associated TCP connection is mapped one to one with
        // the launched program.
        //
        // obj.msg is the payload of the message that the client
        // javaScript can get.
        //
        wsSession.ws.Emit('launchMessage', clientLaunchId, message);
    });


    socket.on('close', function() {

        if(clientLaunchId !== false && programs[clientLaunchId].destroy !== undefined)
            delete programs[clientLaunchId].destroy;

        Spew('closed');
    });


}).listen(opt.radio_port, function() {

        console.log("listening for crts_radio controller service on port " +
            opt.radio_port);
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
