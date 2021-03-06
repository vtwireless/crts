#!/usr/bin/env node

"use strict";

// This util.js creates the global functions:
//
//   fail('', '', '', ..), assert(bool, '', '', ...), and
//   rmDirRecursive()
//
//
require('./util');

// opt contains the configuration parsed from the command line
// and maybe other interfaces.
//
const opt = require('./opt');


var controllers = null;
var spectrumFeeds = null;

// Get User Permission Parameter Data
//
function GetUserPermData(funcName, user, programName, setGet,
                controlName, parameterName) {

    var args = [].slice.call(arguments);

    function barf() {  
        args.shift();
        spew(funcName + "(" + args + ') failed, data not consisent');
        return null;
    }

    // There are 2 requirements: the data must exist (user too), and the
    // user much have permission.

    // controller[programName][setGet][controlName][parameterName][user]
    //
    // returns controller[programName][setGet][controlName][parameterName]
    //
    // If we just tried to returned
    // controller[programName][setGet][controlName][parameterName] without
    // all this "if" checking a simple webSocket send could crash the
    // server.
    //
    var controller = controllers[programName];
    if(controller === undefined) return barf();
    var setOrGet = controller[setGet];
    if(setOrGet === undefined) return barf();
    var control = setOrGet[controlName];
    if(control === undefined) return barf();
    var access = control[parameterName];
    if(access === undefined) return barf();
    if(access[user] === undefined) return barf();

    // We return this object so that the user may do whatever
    // they need to: change a value or get a value.
    return access;
}



var express = require('express'),
    webSocket = require('ws'),
    path = require('path'),
    fs = require('fs'),
    process = require('process'),
    url = require('url'),
    app = express(),
    serveIndex = require('serve-index');

const { spawn } = require('child_process');

const doc_root = opt.doc_root;


console.log("Set document root to: " + doc_root);


function sendMimeType(req, res) {

    var extname = path.extname(url.parse(req.url).pathname);

    // reference:
    // https://developer.mozilla.org/en-US/docs/Learn/Server-side/Node_server_without_framework

    // Why does express not do this mimeTypes for me.  I'm a little put
    // off by how little express does.
    //
    var mimeTypes = {
        '.html': 'text/html', // HTML page file
        '.htm': 'text/html',  // HTML fragment
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

    //console.log("filePath='" + url.parse(req.url).pathname +
    //    "'  ext=" + extname + ' contentType=' + contentType);

    res.setHeader('Content-Type', contentType);
    //console.log("set mime type to: " + contentType);
}


// This is the express catch all callback...??
//
// All requests must pass through this, so I think.
//
app.get('*', function(req, res, next) {

    var authenticate = require('./authenticate');

    authenticate(req, res, function(user) {

        assert(req.User === undefined);
        req.User = user;

        // Goto next express parser.
        next();
    });

    // and if we did not authenticate the authenticate() call will handle
    // it for us.
});

app.get('/admin/*', function(req, res, next) {

    if(req.User !== 'admin') {

        // This is another authentication failure case it's a regular user
        // accessing a admin page.
        //
        res.send("\nNot authorized.\n\n");
        res.end();
        return;
    }

    next();
});

app.get('*', function(req, res, next) {

    // On successful authentication we do this.
    sendMimeType(req, res);
    next();
});

app.get('/', function (req, res) {

    res.sendFile(path.join(doc_root, 'index.html'));
});

// ref:
//   https://expressjs.com/en/starter/static-files.html
//
// Running this server will generate some files that are served via HTTP.
// These generated files will be put in one directory for this particular
// running instance of this web server.  These files are served like they
// are static files from the perspective of the express service, but we
// will generate all the file that are in this directory for this
// particular running instance of this web server.
//
// To access a file in this directory the URL is something like for
// example:
//
//   https://foobar.com:9999/contest_data/foo.png
//
app.use('/contest_data', serveIndex(opt.upload_dir));
app.use('/contest_data', express.static(opt.upload_dir));


// To access a file in this directory the URL is something like for
// example:
//
//   https://foobar.com:9999/foo.png
//
app.use(express.static(doc_root));



// This counter, wsCount, only increases.
//
var wsCount = 0;


// Sessions may have any number of websocket connections,
// so each webSocket connection shares the session data
// for a given user.
//
// If people share the same user than they share this data.
//
// sessions is basically the state of this server.  We don't
// let this server run very long, so we don't worry about putting
// this data in long term storage, like a file or database.
// Each user gets an object in sessions.
//
var sessions = { };

/*********************************************************************
 *              sessions format
 *********************************************************************


sessions = {
    joe: {
        ios: {
            wsId0: io,
            wsId1: io
        }
    },

    admin: { // admin gets access to everything
        ios: {
            wsId0: io,
            wsId1: io
        }
    }
}

*********************************************************************/

if(opt.spectrum_port > 0) {

    // Add the spectrum TCP listener (serivce) which connects a GNU radio
    // python script TCP client called ../bin/crts_spectrumFeed to the
    // spectrum listening port.  The ./launcher.js will handle running the
    // crts_spectrumFeed programs assuming that there is a program that
    // runs crts_spectrumFeed in the launcher directory,
    // opt.launcher_dir.
    //
    spectrumFeeds = require('./spectrumRelayer')();
}



if(opt.radio_port > 0) {

    // Add the streams controller TCP listener (serivce) which connects
    // the stream via the webClient filter to this "radio listening
    // port".

    function addControllerCB(controller) {

        // Add a callback that is called when a controller changes.  For
        // the most part a controller is a list of controls and their
        // parameters.  The controller represents a
        // plugins/Controller/tcpClient.so connection (TODO: TLS
        // controller connections).
        //

        assert(controller !== null);

        if(sessions.admin === undefined)
            return;

        assert(sessions.admin.ios !== undefined);

        Object.keys(sessions.admin.ios).forEach(function(wsId) {
            var wsIo = sessions.admin.ios[wsId];

            // Tell the client about this controller.
            wsIo.Emit('addController',
                controller.programName,
                controller.set, controller.get,
                controller.image
            );

            // Add this webSocket IO thingy to the list of "get"
            // subscribers for all the parameters in this controller.
            controller.addBrowserGetSubscription(wsIo, wsId);
        });
    }

    function removeControllerCB(controller) {

        assert(controller !== null);

        if(sessions.admin === undefined)
            return;

        assert(sessions.admin.ios !== undefined);

        Object.keys(sessions.admin.ios).forEach(function(wsId) {
            sessions.admin.ios[wsId].Emit('removeController',
                controller.programName);
        });
    }


    // This adds the service that connects crts_radio filter streams
    // parameter controllers via a TCP/IP (or TLS) socket connection
    // to the tcpClient.so Controller module.
    controllers = require('./controllerServer')(addControllerCB,
        removeControllerCB);

} // if(opt.radio_port > 0)


var serviceTimeout = false;
var killAllPrograms = function() {};
var numWebSockets = 0;
// This is to stop all spectrum feed and kill all launched programs if
// there are no webSocket connects for longer than some time.
//
function checkLauncherServicesTimeout() {

    if(numWebSockets) {
        // There are webSocket connections, so remove the timeout if there
        // is one.
        if(serviceTimeout !== false) {
            clearTimeout(serviceTimeout);
            serviceTimeout = false;
        }
        return;
    }

    if(serviceTimeout !== false)
        // We already have a timeout so just return.
        return;

    // Set a timeout.
    serviceTimeout = setTimeout(function() {
        if(numWebSockets == 0) {
            // We still have no webSocket connections.
            // TODO: kill all launched programs.
            killAllPrograms();
        }
        serviceTimeout = false;
    }, 1000*60*3 /*milliseocnds delay*/);
}


function onWebSocketConnect(ws) {

    ++numWebSockets;
    checkLauncherServicesTimeout();

    var wsId = (wsCount++).toString();
    var pre = ws.Prefix = 'WebSocket[' +
        wsId + '](' + ws._socket.remoteAddress + ' :' +
        ws._socket.remotePort + '):';
    // spew() is just a object local console.log() wrapper to keep prints
    // starting the same prefix for a given websocket connection.
    var spew = console.log.bind(console, pre);

    var userName = null; // local function copy key to session data.

    // Setup socketIO like interfaces adding ws.On() and ws.Emit()
    // functions to ws.
    
    var io = // io will be a socketIO like thingy
        require('./socketIO')(function(msg) { ws.send(msg); }, spew);

    ws.on('close', function() {

        // TODO: change users login status display at the admin pages.

        if(userName) {
            assert(sessions[userName] !== undefined);
            delete sessions[userName].ios[wsId];
            userName = null;
        }

        if(typeof(io.Cleanup) === 'function') {
            io.Cleanup();
        }

        if(spectrumFeeds)
            spectrumFeeds.removeWebSocket(ws, wsId);

        --numWebSockets;
        checkLauncherServicesTimeout();

        spew('closed');
    });

    function closeFail(arg=null) {

        spew(arg);
        ws.close();

        return 0;
    }



    ws.on('message', function(message) {

        // First check if it's a socketIO like message.
        if(userName && io.CheckForSocketIOMessage(message))
            // If it was a socketIO like message than it was handled in
            // CheckForSocketIOMessage().
            return;

        // Only valid case HERE we have is setting the user name.

        if(userName)
            return closeFail('We have a user name already.');

        // Next is it json with user name.
        try {
            var user = JSON.parse(message);
        } catch {
            return closeFail('message was not JSON.  Closing connection');
        }

        // USER
        if(user.name === undefined || user.password === undefined ||
            opt.users[user.name] === undefined ||
            user.password !== opt.users[user.name].password)
            return closeFail('invalid user (' + user.name+
                ') and/or password (' + user.password + ')');

        pre = ws.Prefix = 'User: ' + user.name + ' WebSocket[' + wsId +
            '](' + ws._socket.remoteAddress + ' :' +
            ws._socket.remotePort + '):';
        spew = console.log.bind(console, pre);

        userName = user.name;

        // The user name is needed to be seen by the io object in
        // socketIO.js, for BroadcastUser() which emits to all that users,
        // with user name userName, webSocket connections.
        //
        io.SetUserName(userName);

        if(sessions[userName] === undefined)
            sessions[userName] = {
                ios: {
                }
            };

        sessions[userName].ios[wsId] = io;

        pre = ws.Prefix = 'User: ' + userName +
            ' WebSocket' +
            '[' + wsId +
            '](' + ws._socket.remoteAddress + ' :' +
            ws._socket.remotePort + '):';
        spew = console.log.bind(console, pre);

        spew('validated user');


        if(spectrumFeeds)
            // Let any spectrum feeds be used by this webSocket.
            spectrumFeeds.addWebSocket(io, wsId, spew);

        if(userName === 'admin')
            addAdminConnection(io, wsId, spew);
    });




    if(controllers) {

        function checkControllerPermission(funcName, programName, setOrGet,
                controlName, parameterName) {

            if(userName === 'admin') return false; // success

            var access = GetUserPermData(funcName, user, programName, setGet,
                    controlName, parameterName);
            return access[userName];
        }


        io.On('getParameter', function(programName, controlName,
            parameterName, requestId/* the client IDs' this request*/) {

            // This is a client polling for a parameter value.

            if(checkControllerPermission("On('getParameter')",
                    programName, 'set', controlName, parameterName)) {
                spew("GOT 'getParameter' (" + [].slice.call(arguments) +
                    ') permission denied');
                return;
            }

            spew("GOT 'getParameter' (" + [].slice.call(arguments) + ')');

            io.Emit('getParameter', programName, controlName, parameterName, requestId,
                controllers[programName].getParameter(controlName, parameterName));
        });


        io.On('setParameter', function(programName, controlName,
            parameterName, value) {

            if(checkControllerPermission("On('setParameter')",
                    programName, 'set', controlName, parameterName)) {
                spew("GOT 'setParameter' (" + [].slice.call(arguments) +
                    ') permission denied');
                return;
            }

            spew("GOT 'setParameter' (" + [].slice.call(arguments) + ')');

            controllers[programName].setParameter(controlName, parameterName, value);
    });

    }


}


function addAdminConnection(io, wsId, spew) {

    /////////////////////////////////////////////////////////////////////
    //
    //  Here: This webSocket connection is from an admin.
    //
    //  io   - is the socketIO-like thingy for this webSocket connection.
    //
    //  spew - is a spew print function that adds a text prefix
    //
    /////////////////////////////////////////////////////////////////////

    var userNames = [];
    Object.keys(opt.users).forEach(function(userName) {
        if(userName !== 'admin')
            userNames.push(userName);
    });

    var urls = [];
    if(opt.https_port > 0)
        urls.push("https://" + opt.https_hostname + ":" + opt.https_port);
    if(opt.http_port > 0)
         urls.push("http://localhost:" + opt.http_port);


    io.On('addUsers', function() {
        io.Emit('addUsers', opt.users, urls);
    });

    //spew('non-admin users=' + userNames);


    if(controllers) {

        Object.keys(controllers).forEach(function(controllerId) {
            var controller = controllers[controllerId];

            io.Emit('addController', controller.programName,
                    controller.set, controller.get, controller.image);

            // Subscribe to all "get" parameter values for this controller.
            controller.addBrowserGetSubscription(io, wsId);
        });


        io.On('changePermission', function(user, programName, setGet,
                controlName, parameterName, boolValue) {

            var access = GetUserPermData("On 'changePermission'",
                    user, programName, setGet,
                    controlName, parameterName);
            if(!access) return;

            access[user] = boolValue;
            spew("On 'changePermission' (" + [].slice.call(arguments) +
                ') changed access=' + access[user]);

            // Broadcast to all admins but this one.
            io.BroadcastUser('changePermission', 'admin', user,
                programName, setGet, controlName, parameterName, boolValue);
        });
    }

    killAllPrograms = require('./launcher')(io, spew);
}


if(opt.https_port !== 0) {

    var https = require('https');
    var server = https.createServer({
        key: fs.readFileSync(opt.tls.key),  // private_key
        cert: fs.readFileSync(opt.tls.cert) // public_cert
    }, app);


    var ws = new webSocket.Server({server: server});
    ws.on('connection', onWebSocketConnect);

    server.listen(opt.https_port, function() {

        var hostname = opt.https_hostname;

        console.log("https server listening on port " +
            opt.https_port);

        console.log("The following URLs may work:");

        Object.keys(opt.users).forEach(function(userName) {
            var user = opt.users[userName];
            console.log('   https://' + hostname + ':' +
                opt.https_port + '/?user=' + userName +
                '&password=' + user.password);
        });
    });
}


if(opt.http_port !== 0) {

    var http = require('http');
    var server = http.createServer(app);

    var ws = new webSocket.Server({server: server});
    ws.on('connection', onWebSocketConnect);

    server.listen(opt.http_port, 'localhost', function() {

        console.log("http server listening on localhost port " +
            opt.http_port);

        console.log("The following URLs may work:");

        Object.keys(opt.users).forEach(function(userName) {
            var user = opt.users[userName];
            console.log('   http://localhost:' +
                opt.http_port + '/?user=' + userName +
                '&password=' + user.password);
        });
    });
}
