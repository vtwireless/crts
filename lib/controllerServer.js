// This part of the server reads and writes a TCP/IP connection
// from the controller in plugins/Controller/tcpClient.so
//
// For the code in this file we assume that the web controller
// (tcpClient.so) is the only web controller for the program that was
// launched.  Note: This design lacks some foresight in that we needed a
// unique name for each tcpClient that connects, and we wound up using
// the launcher program name as the unique name for each tcpClient; so now
// we can't have more than one tcpClient for each program launched.  We
// need to rewrite a shit ton of code to fix that limitation; well not
// that much ..., but I do not have the time needed to fix it.

// opt contains the configuration parsed from the command line.
//
const opt = require('./opt');

const maxMessageLength = 1024*512;

const eotChar = String.fromCharCode(4);

var connectionCount = 0;

var addControllerCB = null;
var removeControllerCB = null;

const fs = require('fs'),
    path = require('path');



function onConnect(socket) {

    // socket is a client TCP/IP socket and not a WebSocket.
    //
    // TEST with something like:
    //
    //    echo "{\"msg\":\"hello\"}" | nc localhost PORT
    //
    //    echo -ne "{\"msg\":\"hello\"}" | nc localhost 9383
    //

    // totalMessage is a cache of the incoming messages in case we need to
    // append messages to make a complete JSON expression.  You see, TCP
    // does not guarantee complete messages are sent in a given read or
    // write, but TCP does guarantee sequence.  So totalMessage will
    // collect message frames.
    //
    // WebSockets does not have this problem.  WebSockets sends and
    // receives framed messages for each send (write) and receive
    // (read).
    //
    var totalMessage = '';

    var address = socket.remoteAddress + ' :' +
        socket.remotePort;

    socket.setEncoding('utf8');

    // A unique connection ID good for the time this server is running
    // and is associated with this particular TCP connection.
    var programName = null;

    // The TCP socket will send us this controller list that will
    // be associated with this particular TCP connection.
    var controller = {
        programName: programName,
        set: {},
        get: {},
        getValues: {}, // last value received
        getSubscribers: {} // list of webSocket IO thingys
    };

    var spew = console.log.bind(console,
        'crts controller from ' + address + ': ');

    spew('connected');

    function destroy() {

        socket.destroy();
        // This will cause socket.on('close', function() to be called.
    }

    // Setup socketIO like interfaces adding io.On() and io.Emit()
    // functions using the socket.  We do not use the io.Emit(); instead
    // we use socket.write(JSON.stringify(json_message) + '\004')
    // and the \004 char is the EOT terminator that we mark frames with.
    //
    var io = require('./socketIO')(function(msg) { fail(); }, spew);


    socket.on('data', function(message) {

        // TODO: prepend old messages if they fail to JSON parse.

        //spew("message===" + message);

        function wspew(s) {
            spew('WARNING: ' + s + ': message="' + message + '"');
        }

        if(totalMessage.length + message.length > maxMessageLength) {
            wspew('maxMassageLength (' +  maxMessageLength +
                ') was exceeded');
            destroy();
            return;
        }

        // TCP/IP does not guarantee that we have the whole message,
        // so there may be more added in later reads.

        totalMessage += message;

        if(totalMessage.substr(1,1) !== '{') {
            // A JSON must start with '{'.
            wspew('missing leading "{"');
            destroy();
            return;
        }

        // Find 'EOT' break the total Message with COMMAND{json}EOT
        // COMMAND{json}EOT COMMAND{json}EOT, where COMMAND is the one
        // char command (space is not there), which would be from many
        // commands getting pushed together into the one TCP read().  The
        // message can be any number (zero or more) of COMMAND{JSONs}
        // separated by ascii char EOT.
        //
        // Messages are this:
        //
        //   bytes         description
        //  -------    ---------------------
        //     1         COMMAND char
        //    ???        {serialized JSON}
        //     1         EOT char
        //
        //  repeat
        //
        //
        //  COMMAND chars so far:
        //
        //     'I' => socket IO like function call
        //

        var eotIndex = totalMessage.search(eotChar);

        while(eotIndex > -1) {

            var command = totalMessage.substr(1, 1);

            var msg = totalMessage.substr(0, eotIndex);

            // Chop off the current message/JSON thingy which should be
            // terminated by a EOT ascii char.
            //
            totalMessage = totalMessage.substr(eotIndex+1);

            if(io.CheckForSocketIOMessage(msg)) {
                eotIndex = totalMessage.search(eotChar);
                continue;
            }


            msg = msg.substr(1); // chop off the command char.

            try {
                var obj = JSON.parse(msg);
            } catch {
                wspew('JSON.parse() failed');
                // We may get this message appended to later.  So it
                // may get used later, so this may be fine.
                return;
            };


            // TODO: MORE CODE HERE...


            // Goto next: See if there is another JSON plus EOT in the
            // message.
            eotIndex = totalMessage.search(eotChar);
        }

    });

    socket.on('close', function() {

        removeControllerCB(controller);

        spew('closed');
    });

    controller.removeBrowserGetSubscription =
    function(wsId, filterName, parameterName) {

        try {
            var subscribers =
                controller.getSubscribers[filterName][parameterName];
            delete subscribers[wsId];
            console.log('removed subscription for ' + 
                programName + ":" +
                filterName + ":" +
                parameterName +
                ' connection num: ' + wsId);
        } catch(ee) {

        }
    }


    // If filterName = null we subscribe to all filters.
    // If parameterName = null we subscribe to all parameters in that
    // filter.
    //
    controller.addBrowserGetSubscription =
    function(wsIo, wsId, filterName=null, parameterName=null) {

        if(filterName === null) {
            // Recurse for all filters.
            Object.keys(controller.getSubscribers).forEach(function(fN) {
                controller.addBrowserGetSubscription(wsIo, wsId, fN);
            });
            return;
        }

        if(controller.getSubscribers[filterName] === undefined)
            controller.getSubscribers[filterName] = {};

        if(parameterName === null) {
            // Recurse for all parameters in this filter
            Object.keys(controller.getSubscribers[filterName]).
                    forEach(function(pN) {
                controller.addBrowserGetSubscription(wsIo, wsId, filterName, pN)
            });
            return;
        }

        // At this point we have filterName and parameterName.

        var value = false;

        if(controller.getSubscribers[filterName][parameterName] === undefined)
            controller.getSubscribers[filterName][parameterName] = {};


        if(controller.getValues[filterName] !== undefined &&
                controller.getValues[filterName][parameterName] !== undefined)
            value = controller.getValues[filterName][parameterName];


        var subscriptions = controller.getSubscribers[filterName][parameterName];

        if(subscriptions[wsId] !== undefined) {
            console.log("webSocket(" + wsId +
                ') already subscribing to: ' +
                filterName + ":" + parameterName);
            return;
        }

        console.log("webSocket(" + wsId + ') subscribing to: ' +
            filterName + ":" + parameterName);

        subscriptions[wsId] = wsIo;

        if(value !== false)
            // Send the current value if we have one yet.
            wsIo.Emit('getParameter', programName,
                filterName, parameterName, value);

    };


    // 'controlList' is the first message sent to this server from
    // webClient.so.
    io.On('controlList', function(programName_in, set, get, image) {

        programName = programName_in;
        controller.programName = programName;

        // Now that we have the programName we can adjust the spew
        // function to better describe this object.
        //
        spew = console.log.bind(console,
            'crts controller "' + programName + '" from ' + address + ': ');

        spew('got controlList: set=' +
            JSON.stringify(set) +
            ', get=' + JSON.stringify(get));
            //',  image=' + image);

        // Replace the set and get lists for this controller.
        //
        function setupUserAccess(accessObj) {
            Object.keys(opt.users).forEach(function(userName) {
                // { joe: true/*can access*/, betty: false }}
                // The 'admin' does not need permission, it is the
                // super user.
                if(userName === 'admin') 
                    // goto next.
                    return;
                accessObj[userName] = false;
            });
        }

        function setupAccessList(obj, setOrGet/*"get" or "set"*/) {
            var controlNames = Object.keys(obj);
            controlNames.forEach(function(controlName) {
                var control = controller[setOrGet][controlName] = {};
                // obj[controlName] is an array of parameter Names as a string.
                obj[controlName].forEach(function(parameterName) {
                    // parameterNames are unique over the filter.
                    assert(control[parameterName] === undefined);
                    // set default user access permissions
                    spew(setOrGet + ':' + controlName + ':' + parameterName);
                    control[parameterName] = {access:{}};
                    setupUserAccess(control[parameterName].access);
                    if(setOrGet === 'get') return;
                    // Now setOrGet === 'set'. Getters do not have a min
                    // or max, and many not have an initial value set.
                    control[parameterName].min = 0.1;
                    control[parameterName].max = 1.0;
                    control[parameterName].init = null;
                });
            });
        }

        setupAccessList(set, "set");
        setupAccessList(get, "get");

        // Create empty 'get' subscription lists if they do not exist
        // yet.  They will have webSocket subscribers that are written
        // as parameters change from 'get'- value like events from this
        // controller.
        //
        Object.keys(get).forEach(function(filterName) {
            if(controller.getSubscribers[filterName] !== undefined)
                return;
            controller.getSubscribers[filterName] = {};
            Object.keys(get[filterName]).forEach(function(i) {
                let parameterName = get[filterName][i];
                if(controller.getSubscribers[filterName][parameterName] !==
                        undefined)
                    return;

                spew("new getSubscribers for " + filterName + ':' + parameterName);
                controller.getSubscribers[filterName][parameterName] = {};
            });
        });

        // image is a base64 encoded PNG.
        // We generate a PNG image from this string called image

        var imageFilename = programName.replace(/\//g, '_');

        // This controller.image with be the URL seen by the browser
        // so path.join() is not used and '/' is stuck in there.
        //
        // /contest_data/ is the path as seen from the web for files in
        // opt.upload_dir in the file system on this server.
        //
        controller.image = '/contest_data/' + imageFilename + '.png';

        // The full file system path of the image that we will
        // write without the ending, .svg or .png.
        var filepath = path.join(opt.upload_dir, imageFilename);

        // We now write this image as a binary file.  We assume
        // that image was a base64 encoded SVG file.

        var png = filepath + '.png';
        var svg = filepath + '.svg';

        fs.writeFileSync(filepath + '.svg', Buffer.from(image, 'base64'));


        // Now push this control parameter list to any browser admin users
        // via the addControllerCB() callback.
        assert(typeof(addControllerCB) === 'function');
        addControllerCB(controller);

        // TODO: Rewrite this fucked-up code.
        //return;

        // Now create a PNG image from the SVG image.
        //
        // WHY: because we could not run: dot -Tpng|base64 -w 0
        // in controllerClient.so, it would just hang forever.
        // AND: firefox does not display this SVG file.
        //
        // A SVG is so much smaller than PNG, but program BUGS win out.
        //
        const execSync = require('child_process').execSync;
        execSync('convert ' + svg + ' ' + png);

        // TODO: Fix the bug here.
        return;

        // The return code returned from execSync() is not correct
        // so we just look to see if the SVG file is there.
        //
        assert(fs.statSync(png).size > 10,
            "convert " + svg + ' ' + png + " failed");

        spew('Created image files: ' + svg + ' and ' + png);

        // Now push this control parameter list to any browser admin users
        // via the addControllerCB() callback.
        assert(typeof(addControllerCB) === 'function');
        addControllerCB(controller);


        // DEBUG TEST:
        //socket.write(JSON.stringify({stuff}));
    });


    controller.getParameter = function(controlName, parameterName) {

        // return controller.getValues[filterName][parameterName]
        // and keep from crashing if the data is not present.
        //
        if(controller.getValues === undefined) return null;
        if(controller.getValues[filterName] === undefined) return null;
        if(controller.getValues[filterName][parameterName] === undefined) return null;

        return controller.getValues[filterName][parameterName];
    }


    io.On('updateParameter', function(filterName, parameterName, value) {

        //spew('updateParameter ' + programName + ':' + filterName +
        //    ':' + parameterName + ':' + value);

        // This is the parameter change event that receiving data from the
        // TCP socket.
        //
        // Push this value to the webSocket subscribers.
        if(controller.getSubscribers[filterName] === undefined)
            controller.getSubscribers[filterName] = {};
        if(controller.getSubscribers[filterName][parameterName] === undefined)
            controller.getSubscribers[filterName][parameterName] = {};

        if(controller.getValues[filterName] === undefined)
            controller.getValues[filterName] = {};
        if(controller.getValues[filterName][parameterName] === undefined)
            controller.getValues[filterName][parameterName] = {};

        // Record the value.
        controller.getValues[filterName][parameterName] = value;

        var subscriptions = controller.
            getSubscribers[filterName][parameterName];
        Object.keys(subscriptions).forEach(function(wsId) {
            // Sends data to the browsers webSocket.  They did not poll in
            // this case.  They registered a callback for the parameter
            // change event.
            subscriptions[wsId].Emit('getParameter',
                programName, filterName, parameterName, value);
        });
    });


    // TODO: we need to add a record of current standing "get parameter"
    // poll requests by using a counter to ID (replyId) for each get
    // parameter poll request.

    controller.setParameter = function(controlName, parameterName, value) {

        var obj = {};
        // We make an object for each control (in this case just one).
        obj[controlName] =  [
            // array of commands for this control
            // The set command takes two parameters:
            { 'set': [parameterName, value] }
            // ,{ 'get': [ parameterName, replyId ] }
            // ,{ 'set': [ parameterName, value ] }
            // etc ...  but in this case just one.
        ];

        socket.write(JSON.stringify(obj) + '\004');
    };
}


// Make this file/code a singleton.
var runCount = 0;

function init(addControllerCB_in, removeControllerCB_in) {

    assert(runCount === 0, "You can only initialize/run this code once.");
    ++runCount;

    addControllerCB = addControllerCB_in;
    assert(typeof(addControllerCB) === 'function');
    removeControllerCB = removeControllerCB_in;
    assert(typeof(removeControllerCB) === 'function');


    require('net').createServer(onConnect).
        listen(opt.radio_port, function() {

            console.log("listening for crts_radio controller" +
                " service on TCP/IP port " +
                opt.radio_port);
    });
}


module.exports = init;
