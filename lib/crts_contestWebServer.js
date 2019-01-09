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


var controllers = {};


var express = require('express'),
    webSocket = require('ws'),
    path = require('path'),
    fs = require('fs'),
    process = require('process'),
    url = require('url'),
    app = express();


const { spawn } = require('child_process');

const doc_root = path.normalize(path.join(__dirname, "..", "htdocs"));


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
app.use('/contest_data', express.static(opt.contest_dir))

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


***********************************************************************
*              resource access control
***********************************************************************

access = {
    joe: { 
        {
            "set":
                { "filterName0": [ "freq",  "gain" ]
                },
            "get":
                { "filterName0": [ "foo",  "bar", "baz" ]
                },
            "usrps":
                {
            }
        }
    }
}


*********************************************************************/

// Add a callback that is called when a controller changes.  For the
// most part a controller is a list of controls and their parameters.
// The controller represents a plugins/Controller/tcpClient.so
// connection (TODO: TLS controller connections).
//
function addControllerCB(controller) {

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



if(opt.radio_port > 0)
    // This adds the service that connects crts_radio filter streams
    // parameter controllers via a TCP/IP (or TLS) socket connection
    // to the tcpClient.so Controller module.
    controllers = require('./controllerServer')(addControllerCB,
        removeControllerCB);



function onWebSocketConnect(ws) {

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

        if(userName === 'admin')
            addAdminConnection(io, wsId, spew);
    });
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

    io.Emit('addUsers', userNames);

    //spew('non-admin users=' + userNames);

    Object.keys(controllers).forEach(function(controllerId) {
        var controller = controllers[controllerId];

        io.Emit('addController', controller.programName,
                controller.set, controller.get, controller.image);

        // Subscribe to all "get" parameter values for this controller.
        controller.addBrowserGetSubscription(io, wsId);
    });


    io.On('setParameter', function(programName, controlName,
        parameterName, value) {

        if(controllers[programName] === undefined) {
            spew("GOT 'setParameter' (" + [].slice.call(arguments) + ') with no controller');
            return;
        }

        spew("GOT 'setParameter' (" + [].slice.call(arguments) + ')');

        controllers[programName].setParameter(controlName, parameterName, value);
    });


    require('./launcher')(io, spew);
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

        var hostname = require('os').hostname();

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

        var hostname = require('os').hostname();

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
