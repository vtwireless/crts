require('/socketIO.js');



// connectCallback() is a function that this called after the
//      connection/session is good to go.  When called it is passed the io
//      object.  See socketIO.js for the io object.
//
// opts.showHeader
//      true to add HTML page body top header and CSS
//      false to not add HTML page body top header and CSS.
//      By default we do not show header.
//
// opts.loadCRTScss
//      true to add main.css to the page
//      false to not add main.css to the page
//      By default this is false, and we do not load
//      main.css to the page.
//
function Session(connectCallback=null, opts = {}) {

    // Set opts defaults if they have not be overridden yet.
    //
    if(opts.showHeader === undefined)
        opts.showHeader = false; // true is the old default.
    if(opts.loadCRTScss === undefined)
        opts.loadCRTScss = false; // true is the old default.


    function _showUser(user) {

        let sessionStatusDiv = document.getElementById('crts_sessionStatus');
        let userDiv = document.getElementById('crts_user');

        if(sessionStatusDiv && userDiv) {
            if(user === null) {
                sessionStatusDiv.innerHTML = "Not logged in";
                userDiv.innerHTML = '';
            } else {
                sessionStatusDiv.innerHTML = "User: ";
                userDiv.innerHTML = user.name;
            }
        }
    }


    function _client(user, connectCallback=null) {

        var url = location.protocol.replace(/^http/, 'ws') +
            '//' + location.hostname + ':' + location.port + '/';

        var ws = new WebSocket(url);

        assert(typeof(createSocketIOObject) === 'function',
            'script /js/socketIO.js was not loaded yet');

        // This function call will add the, socketIO like, On() and Emit()
        // methods to ws.  Also adds ws.CheckForSocketIOMessage() which
        // eats messages if they of a form that we use to make this
        // socketIO like On() interfaces.  It's something like SocketIO
        // without the bloat of SocketIO.

        // io will be the exposed "public" object.
        var io = createSocketIOObject(function(msg) { ws.send(msg); });

        ws.onmessage = function(e) {

            var message = e.data;

            if(io.CheckForSocketIOMessage(message))
                // We got a socketIO like message and it was
                // handled.
                return;

            // Or the message was not handled; not that there's anything
            // wrong with that.
            console.log("got webSocket message=" + message);
        };


        ws.onopen = function(e) {

            console.log('created webSocket to: ' + url);

            // If this is an invalid attempt than the server will disconnect.

            ws.send(JSON.stringify({
                name: user.name, password: user.password
            }));

            // It is possible that a browser client can be trying to spoof
            // the web server with in-correct info, but in that case the
            // webSocket connection will be dropped unless they can guess
            // a correct password and ws.send() it correctly.  So at this
            // point we can say we are completely connected and ready to
            // go.  i.e. waiting for a reply to the send above is
            // pointless.

            if(connectCallback)
                connectCallback(io);
        };

        ws.onclose = function(e) {

            console.log('closed webSocket to: ' + url);

            _showUser(null);

            // Here is how we could erase the passcode cookie:
            //document.cookie = 'passcode=; Max-Age=-99999999;';
        };
    }


    // returns the user as an object and prepends a <div> the user header
    // (or footer) in the body of the page; or it asserts and fails in the
    // case not having a single valid user as we define it here.
    function getAuthicatedUser() {

        // At this point if this fails, we do not have a valid user so
        // asserting is just fine.
        //
        // The user should not get this page unless they have the passcode
        // cookie set.

        assert(document.cookie.length > 0,
            "No cookies. You must accept cookies to use this web site.");
        var cookies = {};
        document.cookie.split('; ').forEach(function(cookieStr) {
            var cookie = cookieStr.split('=');
            //assert(cookies.passcode === undefined,
            //    "You can't be more than one user");
            if(cookie.length === 2)
                cookies[cookie[0]] = cookie[1];
        });

        assert(cookies.passcode !== undefined);

        var user = cookies.passcode.split(',');
        assert(user.length === 2);
        user = { name: user[0], password: user[1] };
        
        assert(typeof(user.name) === 'string' && user.name.length > 0);
        assert(typeof(user.password) === 'string' && user.password.length > 0);

        _showUser(user);

        return user;
    }

    var user = getAuthicatedUser();

    console.log("We are authenticated as user: " + user.name);


    // We must call _client(user, connectCallback) only once after the
    // other files are loaded via require(); hence the weird if() stuff
    // below.  This way the users connectCallback() is called after
    // everything is setup.
    //

    if(opts.loadCRTScss)
        require('/main.css', function() {
            if(!opts.showHeader)
                _client(user, connectCallback);
        });

    if(opts.showHeader) {
        // Now that we know who the user is we can setup the session header
        require('/sessionHeader.css');
        require('/sessionHeader.htm', function(htm) {
            // We pull this html at the top of the body:
            var header = document.createElement('div');
            header.innerHTML = htm;
            document.body.insertBefore(header, document.body.firstChild);
            _showUser(user);

            _client(user, connectCallback);
        });
    } else if(!opts.loadCRTScss)
        _client(user, connectCallback);
}
