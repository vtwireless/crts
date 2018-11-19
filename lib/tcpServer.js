// opt contains the configuration parsed from the command line.
//
const opt = require('./opt');

const maxMessageLength = 1024*512;

const eotChar = String.fromCharCode(4);


require('net').createServer(function(socket) {

    // socket is a client TCP/IP socket and not a WebSocket.
    //
    //
    // TEST with something like:
    //
    //    echo "{\"msg\":\"hello\"}" | nc localhost PORT
    //
    //    echo -ne  "{\"msg\":\"hello\"}" | nc localhost 9383
    //

    // totalMessage is a cache of the incoming message in case we need to
    // append messages to make a complete JSON expression.
    //
    var totalMessage = '';

    var address = socket.remoteAddress + ' :' +
        socket.remotePort;

    socket.setEncoding('utf8');

    var spew = console.log.bind(console,
        'crts controller from ' + address + ': ');

    spew('connected');

    function destroy() {

        socket.destroy();
        // This will cause socket.on('close', function() to be called.
    }

    socket.on('data', function(message) {

        // TODO: prepend old messages if they fail to JSON parse.

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

        if(totalMessage.substr(0,1) !== '{') {
            // A JSON must start with '{'.
            wspew('missing leading "{"');
            destroy();
            return;
        }

        // Find 'EOT' break the total Message with
        // {json}EOT{json}EOT{json}EOT, which would be from many commands
        // getting pushed together into the one TCP read().  The message
        // can be any number (zero or more) of JSONs, {JSON_STUFF},
        // separated by ascii char EOT.

        var eotIndex = totalMessage.search(eotChar);

        while(eotIndex > -1) {

            var msg = totalMessage.substr(0, eotIndex);

            // Chop off the current message/JSON thingy
            // which should be terminated by a EOT ascii char
            //
            totalMessage = totalMessage.substr(eotIndex+1);

            try {
                var obj = JSON.parse(msg);
            } catch {
                wspew('JSON.parse() failed');
                // We may get this message appended to later.  So it
                // may get used later, so this may be fine.
                return;
            };

            // TODO: turn off this spew.
            //
            spew("got valid JSON message: " + JSON.stringify(obj));

            // TODO: add protocol JSON object verifying here:
            //
            if(obj.STUFFFFFFFFF !== undefined) {
                wspew('missing launch ID or payload or whatever obj=' +
                    JSON.stringify(obj));
                // TODO: keep it the connection for now.
                //destroy();
                return;
            }

            // DEBUG TEST:
            socket.write(JSON.stringify({
                control: "tx",
                command: "get",
                parameter: "freq",
                value: 5.0e+6
            }) + eotChar);


            // TODO: MORE CODE HERE...


            // Goto next: See if there is another JSON plus EOT in the message.
            eotIndex = totalMessage.search(eotChar);
        }

    });


    socket.on('close', function() {

        spew('closed');
    });


}).listen(opt.radio_port, function() {

    console.log("listening for crts_radio controller" +
        " service on TCP/IP port " +
        opt.radio_port);
});


