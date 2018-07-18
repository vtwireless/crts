#!/usr/bin/env node

// Launches requested programs and optionally connect a TCP/IP socket to
// the launched program.


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
// with the init() function/class object thingy.
//
sessions = {};


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
    var session = sessions[id] = {

        // list of TCP connections.
        //
        tcpConnections: {},

        programCount: 0,
        programs: {},

        // The webSocket that the tcp connections needs to
        // relay to and from.
        //
        ws: ws
    };

    function destroy() {
        spew('closing ' + modName);

        // Remove any tcp connections.
        Object.keys(session.tcpConnections).forEach(function(key) {
            session.tcpConnections[key].destroy();
        });

        // Try to make all the programs for this session exit.
        Object.keys(session.programs).forEach(function(key) {
            session.programs[key].destroy();
        });

        // Remove this global data.
        delete sessions[id];
    }

    // Add a webSocket close cleanup function.
    //
    ws.OnClose(function() {
        destroy();
    });

    spew('created ' + modName + ' session');

    function launch(program, args /* array of arguments */) {

        var arg0 = getProgramPath(program);

        var child = spawn(arg0, args, { stdio: 'inherit', shell: '/bin/bash' });

        // Append to the spew for this 'launch' thingy.
        //
        var spew = console.log.bind(spew, 'spawn("' + argv0 +
            '", ...) pid=' + child.pid + ':');


        // It would appear that node at least calls the system call
        // fork(2) before returning from above spawn(), because we now
        // have the child PID.
        //
        if(child.pid < 1) {
            spew('failed: fork failed');
            return;
        }

        var programId = session.programCount++;
        var program = session.programs[programId] = {

            arg0: arg0,
            args: args,
            pid: child.pid/*save the pid*/,
            destroy: function() { 
                if(session.programs[programId] !== undefined) {
                    delete session.programs[programId];
                    if(this.pid > 0) {
                        // This can do nothing if the signal is caught
                        // by the child.
                        child.kill('SIGTERM');

                        setTimeout(function() {
                            // Should we wait and than kill it the hard way.
                            // Give it time to exit.
                            
                            try {
                                child.kill('SIGKILL');
                            } catch {
                                // We tried. We can't do better than that.
                            };
                        }, 4000/*milliseconds*/);

                        this.pid = -1;
                    }
                    delete session.programs[programId];
                }
            }
        };

        child.on('error', function(err) {
            spew('failed err=' + err);
            delete session.programs[programId];
        });

        child.on('close', function(code) {
            console.log(' exited with code ' + code);
            delete session.programs[programId];
        });
    }

    On('launch', launch);  
}


// TODO: Get rid of this TCP crap and use WebRTC instead and let this
// service have a peer to peer connection where one peer is the browser
// and one is the crts_radio or other launched program.
//
// The program connected to the other side of this TCP/IP socket
// should be
//               ../bin/crts_radio ...
//
// or other program running on a computer behind the firewall.
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

    // For a given session we have a number of tcp connections.
    // We do not know what session this is using until the other
    // end tells us in the first message.
    var session = false;
    var connectionId = tcpConnectionCount++;

    var spew = console.log.bind(console,
        'crts controller from ' + address + ': ');

    spew('connected');

    function destroy() {

        if(session !== false) {
            // Remove the associated data for this connection from the
            // session.
            delete session.tcpConnections[connectionId];
            session = false;
        }

        socket.destroy();
    }

    socket.on('data', function(message) {

        // TODO: prepend old messages if they fail to JSON parse.

        function wspew(s) {
            spew('WARNING: ' + s + ': message="' + message + '"');
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

        if(obj.msg === undefined || obj.sessionId === undefined ||
            obj.name === undefined) {
            wspew('missing session ID or payload or whatever');
            destroy();
            return;
        }

        if(sessions[obj.sessionId] === undefined) {

            // The webSocket/session must exist or else there is no client
            // that we are working for.  In this case it does not, so:
            //
            wspew('session with ID: ' + obj.id + ' not found');
            // just drop the connection.
            //
            // This case may just be to a shutdown race, so it may be just
            // fine.  If there is no session then just close and cleanup.
            //
            destroy();
            return;
        }
 
        if(session === false) {
            // The sessions[].tcpConnections[] is seen by the above init()
            // function so that the WebSocket/session thingy can call
            // destroy() when the webSocket closes.
            //
            session = sessions[obj.id];
            // Now we have just learned what client/session this is
            // connected with, so now we make the spew more customized for
            // this connection 
            spew = session.ws.spew.bind(session.ws,
                'crts controller from ' + address + ': ');
            
            // and tell the init() function that we are here by letting
            // there be a tcpConnection object:
            //
            session.tcpConnections[connectionId] = { destroy: destroy };
        }

        // // MORE HERE ... MORE CODE ...
        //
        // TODO: remove this spew and take action.
        //
        spew("got message: " + message);

        // Forward the payload in 'msg' to the webSocket.  The receiving
        // browser client need to know what the associated TCP connection
        // is.  The associated TCP connection is mapped one to one with
        // the launched program.
        session.ws.Emit(obj.name, connectionId, obj.msg);

    });


    socket.on('close', function() {

        if(session !== false) {
            delete session.tcpConnections[connectionId];
            session = false;
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
