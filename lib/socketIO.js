// This is not socketIO, it's just a simplified, subset of the socketIO
// "on" and "emit" like interfaces.  The real SocketIO has some nice
// interfaces, but it's very dated and inefficient.  It's got too much
// crufty code.  I replace it with this much more flexible and 100 line
// piece of code.  This is not meant to be compatible with the real
// socketIO.
//
// This takes an object and a corresponding send function and appends the
// two socketIO like methods "On" and "Emit".  We could not use "on" and
// "emit" because "on" is usually already set.  In addition this adds the
// CheckForSocketIOMessage() function to the object.
//
// By having the CheckForSocketIOMessage() function we do not restrict how
// the underlying communication layer is used.  The use can still just
// send and receive messages of any kind, unlike the real socketIO which
// forces the user to use only the "emit" and "on" interfaces to send and
// receive data.  The same (and more so) can be said about MQTT; it eats
// the whole webSocket, not letting the user even look at it.
//
// The user must call obj.CheckForSocketIOMessage(msg) for each message
// that they think may be a socketIO like message.  The passed in msg
// message must be one framed message.  This does not do the framing of
// messages.  If the msg starts with 'I' we assume that it was intended to
// be consumed by this message parser.
//
//
// We can use this with webSockets, TCP/IP sockets, and likely TLS/TCP/IP
// sockets.
//


//
function createSocketIOObject(sendFunc, warnFunc=null) {

    // Funcs is the list of "On" callback functions with their
    // corresponding scope.
    var funcs = {};
    var obj = {};

    if(warnFunc)
        var warn = warnFunc;
    else
        var warn = console.log;


    // Like: socketIO.on('name', function(arg0, arg1, arg2, ...) {})
    //
    obj.On = function(name, func) {

        // Make it be able to be set more than one callback for a given
        // name.  So if we have an array of functions for a given name
        // than there is more than one callback to call.
        //
        // javaScript is f-ing magic and will save the scope (context) of
        // the function, func, too.

        if(funcs[name] === undefined)
            funcs[name] = [];
        funcs[name].push(func);
    };

    // Return true if the message is consumed; that is it should have been
    // used up by this interface, even if it failed.
    obj.CheckForSocketIOMessage = function(msg) {

        // msg should be like:
        //
        //   msg = I{name: funcName, args: [ arg0, arg1, arg2 ] }

        if(msg.substr(0,1) !== 'I')
            return false; // The user may handle this message.

        try {
            var json = JSON.parse(msg.substr(1));
        } catch {
            warn('bad json parse in msg=' + msg);
            return true; // fail but handled
        }

        if(json.name === undefined) {
            warn('No IO name in msg=' + msg);
            return true; // fail but handled
        }


        if(json.args === undefined || typeof(json.args) !== 'object' ||
                json.args.length === undefined) {
            warn('Bad IO json args in: ' + msg);
            return true; // fail but handled
        }

        if(funcs[json.name] === undefined) {
            warn('NO socketIO On "' + json.name + '" for message=' + msg);
            return true;
        }

        if(json.name === "launch")
                warn('handling (' + funcs[json.name].length +
            ') socketIO On "' + json.name + '" message=' + msg);


        funcs[json.name].forEach(function(func) {

            // Call the callback function with the args:
            func(...json.args);

            if(json.name === "launch")
                warn('handled');

        });

        return true; // success and handled
    }

    // Like socketIO.emit()
    //
    // Usage: obj.Emit(name, arg0, arg1, arg2, ...)
    //
    obj.Emit = function(x) {

        var args = [].slice.call(arguments);
        var name = args.shift();

        // Call the users send function which may add EOT char at the end
        // of the message in the case that the send protocol does not have
        // framing built it.  Like for example TCP/IP does not frame
        // messages, but webSockets does frame massages.
        //
        try {
            sendFunc('I' + JSON.stringify({ name: name, args: args }));
        } catch(e) {
            console.log('FAILED to send(name="' + name + '"): ' + e);
        }
    };

    if(typeof(window) !== 'undefined')
        // This is not node JS.  We do not need additional
        // code for the client browser.
        return obj;


    // This is node JS:
    
    obj.SetUserName = function(userName) {
        obj.userName = userName;
    }

    var count = ++createCount;
    // Add to list of objects.  Needed for Broadcast() and
    // BroadcastUser().
    sends[count] = sendFunc;
    objs[count] = obj;

    // These methods will Broadcast to all but this obj.
    obj.BroadcastUser = BroadcastUser;
    obj.Broadcast = Broadcast;



    obj.Cleanup = function() {

        // Replace this functions with dummies so
        // that events that trigger them will not
        // really call them any more, and they don't
        // try to send() with a closed webSocket.

        delete obj.Cleanup;
        obj.CheckForSocketIOMessage = function() {};
        obj.Emit = function() {};
        obj.On = function() {};
        delete sends[count];
    };

    return obj;
}


if(typeof(window) === 'undefined' 
    //&& importScripts === 'undefined'
    ) {

    // Based on typeof(window) === 'undefined' we assume that this is node
    // JS code and not browser javaScript.
    //
    // We don't need node's browerify if we actually test run the code.
    //

    // File scope variables are accessed above:
    //
    var sends = {}; // list of send functions
    var objs = {}; // list of IO objects
    var createCount = 0;

    if(typeof(importScripts) === 'undefined')
        module.exports = createSocketIOObject;

    // global Broadcast() function
    //
    Broadcast = function(/* name, args ... */) {

        var myObj = this;
        var keys = Object.keys(sends);

        if(keys.length < 1) return;

        var args = [].slice.call(arguments);
        var name = args.shift();
        var msg = 'I' + JSON.stringify({ name: name, args: args });
        
        // Call the all send functions that it makes sense to call.
        keys.forEach(function(key) {
            // send for only io object that have client users.
            if(typeof(objs[key].userName) !== 'string') return;
            // if this is part of IO object method than this will
            // not emit for this IO object.
            if(myObj === objs[key]) return;
            sends[key](msg);
        });
    };

    // global BroadcastUser() function
    //
    BroadcastUser = function(/* name, userName, args ...*/) {

        var myObj = this;
        var keys = Object.keys(sends);
        if(keys.length < 1) return;

        var args = [].slice.call(arguments);
        var name = args.shift();
        var userName = args.shift();
        // The message does not need the user name.
        var msg = 'I' + JSON.stringify({ name: name, args: args });

        // Call the users, with this userName, send functions.
        keys.forEach(function(key) {
            // if this is part of IO object method than this will
            // not emit for this IO object.
            if(objs[key].userName === userName && myObj !== objs[key])
                sends[key](msg);
        });
    };

}
