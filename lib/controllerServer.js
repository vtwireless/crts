// This part of the server reads and writes a TCP/IP connection
// from the controller in plugins/Controller/tcpClient.so

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


// Each connection has an associated controller.
var controllers = {};


function onConnect(socket) {

    // socket is a client TCP/IP socket and not a WebSocket.
    //
    // TODO: use a TLS connection.  That will add keys and
    // the C++ code can link with libgnutls or libopenssl.
    //
    // TEST with something like:
    //
    //    echo "{\"msg\":\"hello\"}" | nc localhost PORT
    //
    //    echo -ne  "{\"msg\":\"hello\"}" | nc localhost 9383
    //

    // totalMessage is a cache of the incoming messages in case we need to
    // append messages to make a complete JSON expression.  You see, TCP
    // does not guarantee complete messages are sent in a given read or
    // write, but TCP does guarantee sequence.  So totalMessage will
    // collect message frames.
    //
    // WebSockets does not have this problem.  WebSockets sends and
    // receives framed messages.
    //
    var totalMessage = '';

    var address = socket.remoteAddress + ' :' +
        socket.remotePort;

    socket.setEncoding('utf8');

    // A unique connection ID good for the time this server is running
    // and is associated with this particular TCP connection.
    var id = connectionCount++;

    // The TCP socket will send use this controller list that will
    // be associated with this particular TCP connection.
    var controller = controllers[id]
        = {
            id: id,
            programName: 'program_' + id,
            set: {},
            get: {},
            image_src: ""
        };

    var spew = console.log.bind(console,
        'crts controller ' + id + ' from ' + address + ': ');

    spew('connected');

    function destroy() {

        socket.destroy();
        // This will cause socket.on('close', function() to be called.
    }

    // Setup socketIO like interfaces adding io.On() and io.Emit()
    // functions using the socket.

    var io = {};

    require('./socketIO')(io, function(msg) { socket.write(msg);}, spew);


    io.On('controlList', function(set, get, image) {

        spew('got controlList: set=' +
            JSON.stringify(set) +
            ', get=' + JSON.stringify(get));
            //',  image=' + image);

        // Replace the set and get lists for this controller.
        controller.set = set;
        controller.get = get;

        // image is a base64 encoded PNG.
        // We generate a PNG image from this string called image

        var imageFilename = 'program_' + id.toString();

        // This controller.image with be the URL seen by the browser
        // so path.join() is not used and '/' is stuck in there.
        //
        // /contest_data/ is the path as seen from the web for files in
        // opt.contest_dir in the file system on this server.
        //
        controller.image = '/contest_data/' + imageFilename + '.png';

        // The full file system path of the image that we will
        // write without the ending, .svg or .png.
        var filepath = path.join(opt.contest_dir, imageFilename);

        // We now write this image as a binary file.  We assume
        // that image was a base64 encoded SVG file.
        
        var png = filepath + '.png';
        var svg = filepath + '.svg';

        fs.writeFileSync(filepath + '.svg', Buffer.from(image, 'base64'));

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

            if(command === 'P' && obj.stream_png64 !== undefined) {

                // We define this command "P" as a PNG images with base 64
                // encoding.  Of the form:
                //   { stream_png64: "ZGlncmFwaCB7Cglnc..." }

                spew("Got a stream dot image");

                // TODO: Decode and save this image to a tmp file and
                // serve it to the admin user as a png file.

            } else {
            
                // TODO: turn off this spew.
                //
                spew("got valid JSON message: " +
                    command + JSON.stringify(obj));
            }

            // TODO: MORE CODE HERE...


            // Goto next: See if there is another JSON plus EOT in the message.
            eotIndex = totalMessage.search(eotChar);
        }

    });

    socket.on('close', function() {

        removeControllerCB(controller);

        if(controllers[id] !== undefined)
            delete controllers[id];

        spew('closed');
    });
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

    // We return the object list of all controllers that are make
    // for each connection.
    //
    return controllers;
}


module.exports = init;
