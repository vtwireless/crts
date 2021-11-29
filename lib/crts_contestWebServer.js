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


var checkEmitSaved = require('./checkEmitSaved');


// Each launched program may have an object in this controllers[programName]
//
//
var controllers = {};
//
// loadedState has the same structure as controllers above and maybe more
// stuff in it.  loadedState is the saved state that is loaded at startup,
// and is used as default server state for things like the controllers
// data.  It may have data in it that is not used until a particular
// program it launched; because controllers do not exist until the
// programs that have the controls in them are running.
//
// loadedState = {
//     controllers: {  },
//     launcherPermissions: { }
// }
//
var loadedState = {};


// programs that a user can launch:
// launcherPermissions = {
//       john: {
//           "/tx1": true,
//           "/rx2": false
//      },
//      ...
// }
//
var launcherPermissions = {};



var spectrumFeeds = null;



// Get User Permission to access a particular Parameter
//
function GetUserPermData(funcName, user, programName, setGet,
                controlName, parameterName) {


    function barf(upperArgs) {
        var args = [].slice.call(upperArgs);
        args.shift();
        console.log(funcName + "(" + args + ') failed, ' +
            'data not consisent with current controllers=' +
            JSON.stringify(controllers));
        return null;
    }

    console.log("controllers=" + JSON.stringify(controllers));

    // There are 2 requirements: the data must exist (user too), and the
    // user must have permission.

    // controllers[programName][setGet][controlName][parameterName][access][user]
    //
    // returns
    // controller[programName][setGet][controlName][parameterName][access]
    //
    // If we just tried to returned
    // controller[programName][setGet][controlName][parameterName][access]
    // without all this "try" checking a simple webSocket send callback
    // could crash the server.
    //

    let access;

    try {
        access = controllers[programName][setGet][controlName]
                [parameterName].access;
        // The try will check that this user is a valid user.
        let userAccessBool = access[user];
    } catch {
        return barf(arguments);
    }

    return access;
}


function checkControllerPermission(funcName, programName, setOrGet,
            controlName, parameterName, user) {

    if(user === 'admin') return false; // success

    var access = GetUserPermData(funcName, user, programName,
            setGet, controlName, parameterName);
    if(!access) return false;
    return access[userName];
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

    require('./authenticate')(req, res, function(user) {

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
// will generate all the files that are in this directory for this
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


if(opt.contest_dir !== '')
    app.use('/contest', express.static(opt.contest_dir));




// This counter, wsCount, only increases.
//
var wsCount = 0;


// Sessions may have any number of websocket connections,
// so each webSocket connection shares the session data
// for a given user.
//
// If people share the same user than they share this data.
//
// sessions is some of the user state of this server.  We don't
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
        // the most part, a controller is a list of controls and their
        // parameters.  The controller represents a
        // plugins/Controller/tcpClient.so connection.
        //
        assert(controller !== null);

        assert(sessions.admin.ios !== undefined);

        assert(undefined === controllers[controller.programName]);
        // Copy this controller object.
        controllers[controller.programName] = controller;


        //
        // CAUTION: Butt-ugly code below.  It's just the nature of things.
        //
        // Now override any "setters" that values and user permissions
        // that we can using the opt.state_file data that the server
        // loaded at start up.
        //
        // Put another way: We copy what we can from loadedState to
        // controllers and push values to setters in the running programs
        // if we can.
        //
        // The loadedState does not have to be consistent with the current
        // state of the server, so we just use loadedState "if we can".
        try {

            var lgetValues = loadedState.controllers[controller.programName].
                    getValues;
            var lset = loadedState.controllers[controller.programName].set;
            var lget = loadedState.controllers[controller.programName].get;


            Object.keys(controller.set).forEach(function(controlName) {
                Object.keys(controller.set[controlName]).forEach(
                        function(parameterName) {

                    // Set any value we can set with the "getValues" from
                    // loadedState:
                    if(lgetValues[controlName] !== undefined &&
                            lgetValues[controlName][parameterName] !== undefined) {
                        try {
                            // Set a value from loadedState, if we can:
                            controller.setParameter(controlName, parameterName,
                                        lgetValues[controlName][parameterName]);
                        } catch(ee) {
                        }
                    }

                    // Set any value we can set with the set min and max from
                    // loadedState:
                    if(lset[controlName] !== undefined &&
                            lset[controlName][parameterName] !== undefined) {
                        try {
                            // Set a set min from loadedState, if we can:
                            controllers[controller.programName].set
                                [controlName][parameterName].min =
                                lset[controlName][parameterName].min;
                        } catch(ee) {
                        }
                        try {
                            // Set a set max from loadedState, if we can:
                            controllers[controller.programName].set
                                [controlName][parameterName].max =
                                lset[controlName][parameterName].max;
                        } catch(ee) {
                        }
                        try {
                            // Set a set max from loadedState, if we can:
                            controllers[controller.programName].set
                                [controlName][parameterName].init =
                                lset[controlName][parameterName].init;

                            // Initialize the setter if it was set by an
                            // 'admin' and was saved as an initial value.
                            if(controllers[controller.programName].set
                                    [controlName][parameterName].init !== null)
                                controllers[controller.programName].
                                        setParameter(controlName, parameterName,
                                        lset[controlName][parameterName].init);
                        } catch(ee) {
                        }
                    }

                    // Set a setters user access from loadedState, if we
                    // can.
                    Object.keys(controller.set[controlName]
                            [parameterName].access).forEach(
                                function(userName) {
                        try {
                            let laccess = lset[controlName][parameterName].
                                    access[userName];
                            controllers[controller.programName].set
                                [controlName][parameterName].access[userName] = laccess;
                        } catch(ee) {
                        }
                    });
                });

                // loop on getters.
                Object.keys(controller.get[controlName]).forEach(
                        function(parameterName) {

                    // Set a getters user access from loadedState, if we
                    // can.
                    Object.keys(controller.get[controlName]
                            [parameterName].access).forEach(
                                function(userName) {
                        try {
                            let laccess = lget[controlName][parameterName].
                                    access[userName];
                            controllers[controller.programName].get
                                [controlName][parameterName].access[userName] = laccess;
                        } catch(ee) {
                        }
                    });

                });
            });

        } catch(e) {
            // We do not have that particular state and we do not care.
        }


        Object.keys(sessions.admin.ios).forEach(function(wsId) {
            var wsIo = sessions.admin.ios[wsId];

            // Tell the client about this controller; and all its setters
            // and getters.
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
        assert(undefined !== controllers[controller.programName]);
        delete controllers[controller.programName];
    }

    // This adds the service that connects crts_radio filter streams
    // parameter controllers via a TCP/IP socket connection to the
    // tcpClient.so Controller module.
    require('./controllerServer')(addControllerCB, removeControllerCB);

} // if(opt.radio_port > 0)


var serviceTimeout = false;
var killAllPrograms = false;
var numWebSockets = 0;
// This is to stop all spectrum feed and kill all launched programs if
// there are no webSocket connects for longer than some time.
//
function checkLauncherServicesTimeout() {

    // disable this for now.
    return;

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
        if(numWebSockets == 0 && killAllPrograms) {
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

    function closeFail(arg) {

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

        // The name and password are received and correct.

        pre = ws.Prefix = 'User: ' + user.name + ' WebSocket[' + wsId +
            '](' + ws._socket.remoteAddress + ' :' +
            ws._socket.remotePort + '):';
        spew = console.log.bind(console, pre);

        userName = user.name;

        // The user name is needed to be seen by the io object in
        // socketIO.js, for BroadcastUser() which emits to all that users,
        // with user name userName, webSocket connections.  Also it is
        // used for io.Broadcast() to know that this is a io object that
        // has a user, and not a simple program, on the other end of the
        // socket.
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


        if(userName === 'admin') {

            // We have this 'addAdminConnection' event triggered by the
            // web page when the page is ready to receive the contest
            // state data from this server.  This alleviates race
            // conditions, so the admin client have IO receiver callbacks
            // ready now.
            io.On('addAdminConnection', function() {
                addAdminConnection(io, wsId, spew);
            });
        }

        let ret = require('./launcher')(io, spew, userName,
                launcherPermissions, loadedState);
        if(!killAllPrograms)
            killAllPrograms = ret;
    });



    // A client is polling for the "get" value.
    //
    io.On('getParameter', function(programName, controlName,
        parameterName, requestId/* the client IDs' this request*/) {

        // This is a client polling for a parameter value.
        if(checkControllerPermission("On('getParameter')",
                programName, 'get', controlName, parameterName,
                userName)) {
            spew("GOT 'getParameter' (" + [].slice.call(arguments) +
                    ') permission denied');
            return;
        }

        spew("GOT 'getParameter' (" + [].slice.call(arguments) + ')');

        io.Emit('getParameter', programName, controlName,
                parameterName, requestId,
                controllers[programName].getParameter(controlName,
                    parameterName));
    });


    io.On('setParameter', function(programName, controlName,
            parameterName, value) {

        if(checkControllerPermission("On('setParameter')",
                    programName, 'set', controlName, parameterName,
                    userName)) {
            spew("GOT 'setParameter' (" + [].slice.call(arguments) +
                    ') permission denied');
            return;
        }

        if(userName === 'admin') {
            // We save this value as the initializing value for the
            // 'admin'
            controllers[programName].set[controlName][parameterName].init =
                value;
        }

        spew("GOT 'setParameter' (" + [].slice.call(arguments) + ')');

        controllers[programName].setParameter(controlName,
                    parameterName, value);

        checkEmitSaved.markUnsaved(io)
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

    var urls = [];
    if(opt.https_port > 0)
        urls.push("https://" + opt.https_hostname + ":" + opt.https_port);
    if(opt.http_port > 0)
         urls.push("http://localhost:" + opt.http_port);


    io.Emit('addUsers', opt.users, urls);
    //spew('non-admin users=' + userNames);

    checkEmitSaved.sendSavedState(io);


    Object.keys(controllers).forEach(function(controllerId) {
        var controller = controllers[controllerId];

        io.Emit('addController', controller.programName,
                controller.set, controller.get, controller.image);

        // Subscribe to all "get" parameter values for this controller.
        controller.addBrowserGetSubscription(io, wsId);
    });

    io.Emit('stateFileName', opt.state_file);

    io.On('saveState', function(fileName) {

        let org_state_file = opt.state_file;

        if(fileName.length > 0) {
            if(fileName.substr(0,1) !== '/') // '/' Code not portable.
                opt.state_file = path.join(opt.launcher_dir, fileName);
            else
                opt.state_file = fileName;
        }

        try {
            fs.writeFileSync(opt.state_file,
                // We can add more state objects to this:
                '{ "__comment": "Generated ' + (new Date).toISOString() +
                '",\n\n' +
                '"controllers": ' +
                JSON.stringify(controllers) +
                ',\n' +
                '"launcherPermissions": ' +
                JSON.stringify(launcherPermissions) +
                '\n}\n');
            checkEmitSaved.markSaved(io);
            spew("Saved state to file: " + opt.state_file);
            //file written successfully
        } catch (err) {
            console.error(err)
        }

        if(org_state_file !== opt.state_file)
            io.Broadcast('stateFileName', opt.state_file);
    });


    io.On('changePermission', function(user, programName, setGet,
            controlName, parameterName, boolValue) {

        var access = GetUserPermData("On 'changePermission'",
                user, programName, setGet,
                controlName, parameterName);
        if(!access) return;

        // Change our local controllers object.  The above
        // GetUserPermData() will have verified that this chain of 6
        // keys [][][][][][] to this controllers object is valid.

        controllers[programName][setGet][controlName]
                [parameterName].access[user] = boolValue;
        spew("On 'changePermission' (" + [].slice.call(arguments) +
            ') changed access=' + access[user]);

        // Broadcast to all admins but this one.
        io.BroadcastUser('changePermission', 'admin', user,
            programName, setGet, controlName, parameterName, boolValue);
        // Tell all admin connections that this may not be saved
        // in the saved state file now, but only if it's not known
        // already.
        checkEmitSaved.markUnsaved(io);
    });

    function setParameterLimit(Type, type, programName, controlName,
        parameterName, value) {

        // Note: user 'admin' does not need an access checked but it
        // does need to have the arguments verified.
        if(checkControllerPermission("On('setParameter" + Type + "')",
                programName, 'set', controlName, parameterName,
                'admin')) {
            spew("GOT 'setParameter" + Type + "' (" + [].
                slice.call(arguments) +
                ') permission denied');
            return;
        }

        spew("GOT 'setParameter" + Type + "' (" + [].
                slice.call(arguments) + ')');

        controllers[programName].set[controlName]
                [parameterName][type] = value;

        io.Broadcast('setParameter' + Type, programName, controlName,
                parameterName, value);
    }

    io.On('setParameterMin', function(programName, controlName,
        parameterName, value) {

        setParameterLimit('Min', 'min', programName, controlName,
            parameterName, value);
        // Tell all admin connections that this may not be saved in the
        // saved state file now, but only if it's not known already.
        checkEmitSaved.markUnsaved(io);
    });

    io.On('setParameterMax', function(programName, controlName,
        parameterName, value) {

        setParameterLimit('Max', 'max', programName, controlName,
            parameterName, value);
        // Tell all admin connections that this may not be saved in the
        // saved state file now, but only if it's not known already.
        checkEmitSaved.markUnsaved(io);
    });
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
            if(userName === "admin")
                console.log('   https://' + hostname + ':' +
                    opt.https_port + '/admin/?user=' + userName +
                    '&password=' + user.password);
            else
                console.log('   https://' + hostname + ':' +
                    opt.https_port + '/contest/?user=' + userName +
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
            if(userName === "admin")
                console.log('   http://localhost:' +
                    opt.http_port + '/admin/?user=' + userName +
                    '&password=' + user.password);
            else
                console.log('   http://localhost:' +
                    opt.http_port + '/contest/?user=' + userName +
                    '&password=' + user.password);
        });
    });
}


// At startup try to read in state data:
try {
    loadedState = JSON.parse(fs.readFileSync(opt.state_file,
                {encoding:'utf8', flag:'r'}));
    console.log("Loaded service state file: " + opt.state_file +
        ": " + JSON.stringify(loadedState));
} catch(e) {
    // It's okay to fail.  Maybe there was no file to read yet.
    console.log("Failed to load state file: \"" + opt.state_file +
        "\": " + e);
    loadedState = {
        controllers: {},
        launcherPermissions: {}
    };
}
