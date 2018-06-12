// Developer configuration
var debug = true;

var spew = console.log.bind(console);

if(debug)
    var dspew = spew;
else
    var dspew = function() {};


// Lets make crts a singleton.  Kind of like the application, there can
// only be one.
var crts = false;


function fail() {

    // TODO: add stack trace or is browser debugger enough?
    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    const line = '--------------------------------------------------------';
    text += '\n' + line + '\nCALL STACK\n' + line + '\n' +
        new Error().stack + '\n' + line + '\n';
    console.log(text);

    if(crts) crts.cleanup();


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


function CRTSClient(onInit=function(){}) {

    // The CRTS object we are making is a singleton.
    if(crts === false) {
        crts = {};
        crts.onInits = [];
        crts.onInits.unshift(onInit);
    } else {
        // we have a crts object already.
        if(crts.onInits !== undefined)
            // and the onInits have not been called yet,
            // so we add this one to be called later.
            crts.onInits.unshift(onInit);
        else
            // we can call it now.
            onInit(crts);

        return; // We already have the singleton crts object.
    }

    // We can call CRTSClient() many times but it only runs
    // here once and with one crts object.


    crts.cleanupFunctions = [];

    crts.cleanup = function() {

        this.cleanupFunctions.forEach(function(cleanup) {
            cleanup(crts);
        });
        delete crts.cleanupFunctions;
        delete crts;
        crts = false;
        // We are done diddly done with this crts object,
        // but we could make another...
    };


    // Why does this WebSocket() standard not have this as the default url
    // arg?  Stupid standards people.
    //
    var url = location.protocol.replace(/^http/, 'ws') +
        '//' + location.hostname + ':' + location.port + '/';

    // We'll make only one webSocket object.
    var ws = new WebSocket(url);
    var pre = 'WebSocket(' + url + '):';

    // spew() is just a object local console.log() wrapper to keep prints
    // starting the same prefix for a given websocket connection.
    spew = console.log.bind(console, pre);

    if(debug) dspew = spew;

    // Other functions may call spew().
    crts.spew = spew;

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

        spew("close"); // developer feedback...
    };

    ws.onopen = function() {

        spew("connected");
    };

    On('init', function(id) {
        pre = 'WebSocket[' + id + '](' + url + '):';
        spew = console.log.bind(console, pre);
        if(debug) dspew = spew;
        spew("got \"init\" client id=" + id);

        Emit('init', 'hi server');

        // This is where we know that the server is ready for this
        // client to send commands to server.  If we need another
        // transaction to startup we can put this if block in a
        // different On() call.
        if(crts.onInits !== undefined) {
            crts.onInits.forEach(function(init) {
                init(crts);
            });
            delete crts.onInits;
        }
    });


    crts.cleanupFunctions.unshift(function() {
        ws.close();
    });

    var spectrumDisplays = {};


    On('spectrum', function(id, tag, values) {

        if(spectrumDisplays[tag] === undefined) {
            // This may be a late send, we can ignore it.
            dspew('bad spectrum array');
            return;
        }

        if(spectrumDisplays[tag].spectrumHandler)
            spectrumDisplays[tag].spectrumHandler(tag, values);
        else if(debug)
            spew('"spectrum" : ' + tag + ': ' + values.toString());
    });

    // tag is a string that is unique to this particular spectrum display
    // on this webSocket client.
    crts.createSpectrumDisplay = function(freq, bandwidth, 
                bins, updateRate, tag, uhd_args, host,
                spectrumHandler) {

        spew("createSpectrumDisplay(freq=" + freq +
            " MHz, bandwidth=" + bandwidth +
            " MHz, bins=" + bins +
            ", updateRate=" + updateRate +
            ", tag=" + tag +
            ", uhd_args=" + uhd_args +
            ', host="' + host,
            '", spectrumHandler=' + spectrumHandler);

        if(spectrumDisplays[tag] !== undefined) {

            let str = 'You have a spectrum display named "' +
                tag + '" already.';
            spew(str);
            alert(str);
            return;
        }

        spectrumDisplays[tag] = {
            spectrumHandler: spectrumHandler,
            freq: freq,
            bandwidth: bandwidth,
            bins: bins,
            updateRate: updateRate,
            tag: tag,
            uhd_args: uhd_args,
            host: host
        };

        Emit('launchSpectrumSensing', freq/*MHz*/, bandwidth/*MHz*/,
            bins/*number of bins*/, updateRate, uhd_args/*uhd device on host*/,
            host/*host or '' for no ssh to run spectrumSensing*/,
            tag/*the whatever tag string*/);
    };

    crts.stopSpectrum = function(tag) {

        if(spectrumDisplays[tag] === undefined) return;
        delete spectrumDisplays[tag];

        spew('requesting "stopSpectrumSensing" for tag: ' + tag);
        Emit('stopSpectrumSensing', tag);
    };

}

function CRTSCreateSpectrumDisplay(freq, bandwidth, bins, updateRate,
            tag="tag", uhd_args="", host="", spectrumHandler=null) {

    CRTSClient(function(crts) {

        crts.createSpectrumDisplay(freq, bandwidth, bins, updateRate,
                    tag, uhd_args, host, spectrumHandler);
    });
}

function CRTSStopSpectrum(tag) {

    CRTSClient(function(crts) {

        crts.stopSpectrum(tag);
    });
}

