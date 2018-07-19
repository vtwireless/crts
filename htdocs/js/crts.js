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


function GetElementById(id) {

    var e = document.getElementById(id);
    assert(e, "GetElementById(id='" + id + "')=" + e);
    return e;
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
        var str = JSON.stringify({ name: name, args: args });

        dspew("sending: " + str);
        ws.send(str);
    }

    function On(name, func) {

        if(onCalls[name] !== undefined && func === null) {
            spew("removing web socket handler for '" + name + "'");
            delete onCalls[name];
            return;
        }

        if(onCalls[name] !== undefined)
            spew("replacing web socket handler for '" + name + "'");

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

        //dspew('handling message=\n   ' + e.data);

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



    // A list of spectrum feed objects.  Each one in this
    // list has a different "connection"/feed from a USRP.
    // This list has it's elements keyed off of a tag "string"
    // which is unique to this web socket client.
    //
    // TODO: Add the validating of the spectrum parameters.
    //
    var spectrumFeeds = {
    };

    // This is just a simple wrapper that helps in debugging.
    //
    function getSpectrumFeed(tag, funcName) {

        if(spectrumFeeds[tag] === undefined) {
            // This may be a late send, we can just ignore it.
            dspew(funcName + '(tag="' + tag + ', ...") bad spectrum tag');
            return false;
        }
        return spectrumFeeds[tag];
    }


    On('spectrumUpdate', function(id, tag, values) {

        var sf = getSpectrumFeed(tag, "On('spectrumUpdate')");
        if(sf === false) return;

        if(sf.onupdates.length > 0) {
            sf.onupdates.forEach(function(update) {
                // update the Displays
                update(values);
            });
        } else {
            // If there are no Displays we can at least spew.
            dspew('"spectrumUpdate": ' + tag + ': ' + values.toString());
        }

        sf.values = values;

        if(!sf.running)
            sf.running = true;
    });


    /* class SpectrumFeed
     *
     * There are the following SpectrumFeed client/server commands with
     * not exactly the same meaning at words in the client API:
     *
     *   startSpectrumFeed   Will create the feed and start sending
     *                       updates.  Also sets the parameters that
     *                       are sent with it.
     *
     *   setSpectrumFeed     Will set the parameters on the server.  Can
     *                       only be sent if the feed was created with
     *                       startSpectrumFeed.
     *                       
     *   stopSpectrumFeed    Will stop the feed updates and it can be
     *                       restarted.
     *
     *   destroySpectrumFeed Will stop the feed and remove the possibility
     *                       of restart.
     *
     */



    // tag is a string that is unique to this particular spectrum display
    // on this webSocket client.
    crts.SpectrumFeed_create = function(tag, host, port, uhd_args,
                onupdate, onset, ondestroy) {
        var sf;

        // Note: We do not bother contacting the server until we must,
        // that is when the spectrum data starts being sent.  For this
        // create thing we just take local client notes, creating an
        // object that keeps the state, parameters, and display callback
        // functions.

        if(spectrumFeeds[tag] === undefined) {

            dspew('Making spectrumFeed named "' + tag);

            sf = spectrumFeeds[tag] = {
                ////////////////////////////////
                // Default spectrum parameters
                ////////////////////////////////
                freq: 915.0/*MHz*/,
                bandwidth: 2.0/*MHz*/,
                bins: /*number of bins*/4,
                updateRate: 1.0/*Hz*/,
                tag: tag,
                uhd_args: uhd_args,
                // last requested state
                running: false,
                host: host,
                port: port,
                values: false,
                haveNewParameters: true,
                onupdates: [],
                onsets: [],
                ondestroys: []
            };
        } else
            sf = spectrumFeeds[tag];

        if(onset) {
            sf.onsets.push(onset);
            // update parameters in the new Display
            onset(sf.freq, sf.bandwidth, sf.bins, sf.updateRate);
        }

        if(onupdate) {
            sf.onupdates.push(onupdate);
            if(sf.values)
                // update the values to the new Display
                onupdate(sf.values);
        }

        if(ondestroy)
            sf.ondestroys.unshift(ondestroy);
    };


    // Interface to set spectrum parameters for spectrum with tag tag.
    crts.SpectrumFeed_set = function(tag, freq, bandwidth, bins, updateRate) {

        var sf = getSpectrumFeed(tag, "SpectrumFeed_set");
        if(sf === false) return;

        sf.freq = freq;
        sf.bandwidth = bandwidth;
        sf.bins = bins;
        sf.updateRate = updateRate;

        if(sf.running) {
            // There is no need to change the parameters on the server
            // unless we are getting data from the server now, running is
            // true.
            Emit('setSpectrumFeed', tag, sf.freq/*MHz*/, sf.bandwidth/*MHz*/,
                sf.bins/*number of bins*/, sf.updateRate/*Hz*/);
            // The server will know these settings soon.
            sf.haveNewParameters = false; // local flag.
        } else {
            // Flag that we need to set the parameters when we are running
            // again.
            sf.haveNewParameters = true;
        }

        sf.onsets.forEach(function(onset) {
            // update parameters in the spectrum Displays.  Even if the
            // spectrum is not being sent now.
            onset(freq, bandwidth, bins, updateRate);
        });
    };

    crts.SpectrumFeed_stop = function(tag) {

        var sf = getSpectrumFeed(tag, "SpectrumFeed_stop");
        if(sf === false) return;

        if(sf.running) {
            // No need to signal the server stop again and again.
            Emit('stopSpectrumFeed', tag);
            sf.running = false; // local flag.
        }
    };

    On('stopSpectrumFeed', function(tag) {

        var sf = getSpectrumFeed(tag, "SpectrumFeed_start");
        if(sf === false) return;

        spew('spectrumFeed with tag ' + tag + ' stopped');
        sf.running = false;
    });


    crts.SpectrumFeed_start = function(tag) {

        var sf = getSpectrumFeed(tag, "SpectrumFeed_start");
        if(sf === false) return;

        if(sf.haveNewParameters || sf.running === false) {
            // The underlying client/server protocol uses
            // 'startSpectrumFeed' to start, and set parameters while it
            // is running.
            Emit('startSpectrumFeed', sf.freq/*MHz*/, sf.bandwidth/*MHz*/,
                sf.bins/*number of bins*/, sf.updateRate/*Hz*/,
                sf.uhd_args/*uhd device on host*/,
                sf.host/*host or '' for no ssh to run spectrumSensing*/,
                sf.port/*ssh port*/,
                sf.tag/*the whatever tag string*/);
            sf.haveNewParameters = false;
        }
       
        // We wait until we get a 'spectrumUpdate' to set the
        // running flag.
        //sf.running = true;
    };

    crts.SpectrumFeed_destroy = function(tag) {

        var sf = getSpectrumFeed(tag, "SpectrumFeed_destroy");
        if(sf === false) return;

        dspew('destroying SpectrumFeed "' + tag + '"');

        Emit('destroySpectrumFeed', tag);

        // We must delete the spectrumFeeds[tag] so this tag can
        // be reused if the need happens.
        sf.ondestroys.forEach(function(destroy) {
            destroy();
        });
        delete sf.ondestroys;
        delete sf.onupdates;
        delete sf.onsets;
        delete spectrumFeeds[tag];
    };


    /////////////////////////////////////////////////////////////////////
    //   BEGIN: All the Launch stuff
    /////////////////////////////////////////////////////////////////////

    var launchCount = 0;
    var launches = {};

    crts.Launch = function(program, args, host, port, handler) {

        var launchId = launchCount++;

        Emit('launch', launchId, program, args, host, port, launchId);
        launches[launchId] = {
            handler: handler
        };
    };

    // Feedback from a launched program that finished.
    On('launch', function(launchId, status, feedbackStr) {

        if(launches[launchId] === undefined) {
            spew("got unknown launch reply");
            return;
        }
        launches[launchId].handler(status,feedbackStr);
        delete launches[launchId];
    });

    /////////////////////////////////////////////////////////////////////
    //   END: All the Launch stuff
    /////////////////////////////////////////////////////////////////////



    crts.USRPs_getList = function(hosts, handler) {

        // We stop the race of having crts.USRPs_getList()
        // called again before the last call returned.
        if(onCalls['getUSRPsList'] !== undefined) {
            spew("We are already waiting for the USRPs List");
            return;
        }

        Emit('getUSRPsList', hosts);

        // We need a wrapper web socket handler that removes
        // its self after it is called.
        On('getUSRPsList', function(usrps) {

            handler(usrps);
            // Remove this web socket handler
            On('getUSRPsList', null);
        });
    };


    crts.cleanupFunctions.unshift(function() {

        Object.keys(spectrumFeeds).forEach(function(tag) {
            var sf = getSpectrumFeed(tag, "cleanupFunctions spectrumFeeds");
            if(sf != false)
                crts.SpectrumFeed_destroy(tag);
        });
    });
}


////////////////////////////////////////////////////////////////////////////
//  app API (application programming interface)
////////////////////////////////////////////////////////////////////////////
//
// We wish to avoid letting the "app" have direct access to the
// server/client "communication channel", that is the webSockets and the
// Socket.IO like interfaces (and WebRTC stuff too).  We want to abstract
// that away from the code that is drawing and controlling stuff.  That
// will make it so that there can be a changing mix of "widgets" and
// "controllers" which are not inherently inter-connected.  Stuff can than
// be added and removed independently.  Like for example: we can have any
// number of spectrum displays, and USRP controller widgets that are not
// inter-connected.
//
// With this idea we can change the under-lying communication channel and
// seamlessly change it from WebSockets to WebRTC with peer to peer UDP
// based connections.  People using this API should never have access to
// the webSockets and/or WebRTC connections.  Users see this API as an
// event callback system where they request to have their callbacks called
// when events happen.  You may have a fancy parameter setting widget that
// just sets and gets spectrum parameters that seamlessly sets these
// parameters for all the connected spectrum displays, and it would not
// need to know what spectrum displays are present.
//
///////////////////////////////////////////////////////////////////////////
//
// The API of functions below are wrappers that delay the calling of the
// webSocket send until there is a webSocket connection.  Without this
// "delay" none of these functions could be called at start up, they would
// just fail due to trying to write to an invalid webSocket.  This beats
// the pain of using javaScript Promises.
//
///////////////////////////////////////////////////////////////////////////



function Launch(program, args, host='', port='', closeHandler=null) {

    var close = closeHandler;

    if(close === null) {
        close = function(status, feedback) {
            spew("Lanuching: " + program +
                " returned status: " + status +
                " : " + feedback);
        };
    }

    CRTSClient(function(crts) {
        crts.Launch(program, args, host, port, close);
    });
}




function USRPs_getList(hosts/*array of strings of hostnames*/,
            usrpsHandler=null) {

    var handler = usrpsHandler;

    if(handler === null) {
        handler = function(usrps) {
            spew("Got 'getUSRPsList' returned: " +
                    JSON.stringify(usrps));
        };
    }

    CRTSClient(function(crts) {
        crts.USRPs_getList(hosts, handler);
    });
}


function SpectrumDisplay_create(tag,
        onupdate=null, onset=null, ondestroy=null) {

    // We assume that the spectrum feed parameters are
    // already set, so host and port are set already.
    SpectrumFeed_create(tag, null/*host*/, null/*port*/,
        null/*uhd_args*/, onupdate, onset, ondestroy);
}


// For a given tag this adds another display for the spectrum data.  This
// does not build the display.  The user of this function uses this to
// make a particular spectrum display be it 2D or 3D, or whatever.  You
// can have any number of "displays" for a given spectra tag.
//
// Q: Why not make and return an object?
//
// 1. In javaScript returning an object is a problem given that the
//    functions in the returned object may not be valid until an
//    asynchronous initialization event happens.
//
// 2. We wish to not have to pass objects around between codes that are
//    otherwise independent.
//
// 3. The information that is shared between codes that use these shared
//    objects does not have a general object in it, and therefore can
//    be the same kind of information the is shared on the webSocket
//    client/server interface; it's JSON.  If we used an general object we
//    could not shared this object with the server, but JSON (not a
//    general object) can be shared between the client and the server.
//
// 4. Thinking about 3.  We don't want to go down the networked
//    javaScript/CORBA rabbit hole.  CORBA and RPC have failed to catch on
//    for so many reasons.
//
// reference:
//
//  https://stackoverflow.com/questions/3835785/why-has-corba-lost-popularity
//  https://en.wikipedia.org/wiki/Common_Object_Request_Broker_Architecture
//
function SpectrumFeed_create(tag, host="", port="", uhd_args="",
        // Different event handler functions that a "display implementation"
        // may set:
            onupdate=null, // called when spectrum data comes
            onset=null,    // called when spectrum parameters change
            ondestroy=null // called when the spectrum feed is gone
            ) {

    CRTSClient(function(crts) {
        crts.SpectrumFeed_create(tag, host, port, uhd_args,
                    onupdate, onset, ondestroy);
    });
}


function SpectrumFeed_set(tag, freq, bandwidth, bins, updateRate) {
    
    CRTSClient(function(crts) {
        crts.SpectrumFeed_set(tag, freq, bandwidth, bins, updateRate);
    });
}


// This only gets the local javaScript parameter values, it does
// not query the server.
function SpectrumFeed_get(tag, obj) {

    CRTSClient(function(crts) {

        crts.SpectrumFeed_get(tag, obj);
    });
}


function SpectrumFeed_stop(tag) {

    CRTSClient(function(crts) {
        crts.SpectrumFeed_stop(tag);
    });
}


function SpectrumFeed_start(tag, onstart=null) {

    CRTSClient(function(crts) {
        crts.SpectrumFeed_start(tag, onstart);
    });
}


function SpectrumFeed_destroy(tag) {

    CRTSClient(function(crts) {
        crts.SpectrumFeed_destroy(tag);
    });
}
