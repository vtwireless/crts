require('/js/socketIO.js');

function createSession(connectCallback=null) {

    function _showUser(user) {

        let sessionStatusDiv = document.getElementById('sessionStatus');
        let userDiv = document.getElementById('user');

        if(sessionStatusDiv && userDiv) {
            if(user === null) {
                sessionStatusDiv.innerHTML = "Not logged in";
                userDiv.innerHTML = '';
            } else {
                sessionStatusDiv.innerHTML = "Authenticated as user: ";
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
        // socketIO like On() interfaces.

        // io will be the exposed "public" object.
        var io = createSocketIOObject(function(msg) { ws.send(msg); });

        ws.onmessage = function(e) {

            var message = e.data;

            if(io.CheckForSocketIOMessage(message))
                // We got a socketIO like message and it was
                // handled.
                return;

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

            if(connectCallback) {

                connectCallback(io);
            }
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
            assert(cookies.passcode === undefined,
                "You can't be more than one user");
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

    if(user.name !== 'admin' ||
        !document.getElementById('contestAdminPanel')) {

        _client(user, connectCallback);
        return;
    }

    /////////////////////////////////////////////////////////////////
    // At this point we are an "admin" so load /js/contestAdmin.js
    // and other stuff for the admin.
    /////////////////////////////////////////////////////////////////

    var head = document.getElementsByTagName('head')[0];

    var link = document.createElement('link');
    link.href = '/admin/css/contestAdmin.css';
    link.rel = "stylesheet";
    link.type = 'text/css';
    head.appendChild(link);
    // Note how we nest the onload callbacks to avoid file loading and
    // function calling race conditions.
    link.onload = function() { 
        var script= document.createElement('script');
        script.src= '/admin/js/contestAdmin.js';
        head.appendChild(script);
        script.onload = function() { 
            
            _client(user, function(client) {

                // contestAdminInit() is from '/admin/js/contestAdmin.js'
                // and it needs the client webSocket.  "This code here"
                // is being called before the webSocket receives any data,
                // but after the webSocket connection is open.
                contestAdminInit(client);

                if(connectCallback)
                    connectCallback(client);
            });
        };
    };
}
