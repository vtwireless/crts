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
function createSocketIOObject(obj, sendFunc, warnFunc=null) {

    // Funcs is the list of "On" callback functions with their
    // corresponding scope.
    var funcs = {};

    if(warnFunc)
        var warn = warnFunc;
    else
        var warn = console.log;

    assert(obj.On === undefined);
    assert(obj.CheckForSocketIOMessage === undefined);
    assert(obj.Emit === undefined);

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

        // TODO: remove this spew:
        warn('handling (' + funcs[json.name].length +
            ') socketIO On "' + json.name + '" message');

        funcs[json.name].forEach(function(func) {

            // Call the callback function with the args:
            func(...json.args);
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
        sendFunc('I' + JSON.stringify({ name: name, args: args }));
    };
}

if(typeof(window) === 'undefined') {

    // Based on typeof(window) === 'undefined' we assume that this is node
    // JS code and not browser javaScript.  We don't need browerify if we
    // actually test the code.
    //
    module.exports = createSocketIOObject;
}
