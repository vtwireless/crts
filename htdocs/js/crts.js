var debug = true;

var cleanup = false;

function fail() {

    // TODO: add stack trace or is browser debugger enough?
    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    const line = '--------------------------------------------------------';
    text += '\n' + line + '\nCALL STACK\n' + line + '\n' +
        new Error().stack + '\n' + line + '\n';
    console.log(text);
    if(cleanup) cleanup();
    alert(text);
    window.stop();
    throw "javascript error"
}

function assert(val, msg=null) {

    if(!val) {
        if(msg)
            fail(msg);
        else
            fail("JavaScript failed");
    }
}


function CRTSClient() {

    // Why does this WebSocket() standard not have this as the default url
    // arg?
    var url = location.protocol.replace(/^http/, 'ws') +
        '//' + location.hostname + ':' + location.port + '/';

    var ws = new WebSocket(url);
    var pre = 'WebSocket(' + url + '):';

    // spew() is just a object local console.log() wrapper to keep prints
    // starting the same prefix for a given websocket connection.
    var spew = console.log.bind(console, pre);


    /////////////////////////////////////////////////////////////
    //  BEGIN: Making Socket.IO like interfaces: On() Emit()
    /////////////////////////////////////////////////////////////

    // The list of action functions for this websocket.
    var onCalls = {};

    function Emit() {

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
    }

    ws.onerror = function (error) {
        spew('Error: ' + error);
    };

    ws.onmessage = function(e) {

        //spew("got message: " + e.data);

        var obj = JSON.parse(e.data);
        var name = obj.name; // callback name (not subscription name)

        // We should have this form:
        // e.data = { name: eventName, args:  [ {}, {}, {}, ... ] }
        if(name === undefined || obj.args === undefined ||
                !(obj.args instanceof Array)) {
            fail(pre + 'Bad "on" message from server\n  ' +
                e.data);
        }

        if(onCalls[name] === undefined)
            fail(pre + 'message callback "' + name +
                    '" not found for message: ' +
                    '\n  ' + e.data);

        //spew('handling message=\n   ' + e.data);

        // Call the on callback function using array spread syntax.
        //https://developer.mozilla.org/en/docs/Web/JavaScript/Reference/Operators/Spread_operator
        (onCalls[name])(...obj.args);
    };

    /////////////////////////////////////////////////////////////
    //  END: Making Socket.IO like interfaces:  On() Emit()
    /////////////////////////////////////////////////////////////


    ws.onclose = function() {

        spew("close");
    };

    ws.onopen = function() {

        spew("connected");
    };

    On('init', function(id) {
        pre = 'WebSocket[' + id + '](' + url + '):';
        spew = console.log.bind(console, pre);
        spew("got \"init\" client id=" + id);
        Emit('init', 'hi server');

        Emit('launchSpectrumSensing', 915.0/*freq MHz*/, 2.0/*bandwidth MHz*/,
            3/*bins*/, 10/*update rate*/, "args=192.168.10.3"/*uhd device on host*/,
            'localhost'/*host or '' for no ssh to run spectrumSensing*/,
            'apple4'/*the whatever tag*/);
    });

    On('spectrum', function(tag, values) {
        console.log('"spectrum" : ' + tag + ': ' + values.toString());
    });


    cleanup = function() {
        ws.close();
    }

    return {}; // TODO: add interfaces?
}

CRTSClient();
