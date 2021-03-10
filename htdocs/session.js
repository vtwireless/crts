require('/socketIO.js');



// opts.addAdminPanel
//      true to add admin panel
//      false to not add admin pannel
//
// opts.showHeader
//      true to add HTML page body top header and CSS
//      false to not add HTML page body top header and CSS
//
// opts.loadCRTScss
//      true to add main.css to the page
//      false to not add main.css to the page
//
function createSession(connectCallback=null, opts = {}) {

    // We changed this interface, it used to be:
    // createSession(connectCallback=null, addAdminPanel=true) {}
    // This if(){} will make it compatible with the old interface.
    if(typeof opts === 'boolean') {
        let addAdminPanel = opts;
        opts = {};
        opts.addAdminPanel = addAdminPanel;
    } else {
        if(opts.addAdminPanel === undefined)
            // This is the default; bad as it was and is.
            opts.addAdminPanel = true;
    }

    // Set opts defaults if they have not be overridden yet.
    //
    if(opts.showHeader === undefined)
        opts.showHeader = true; // true is the old default.
    if(opts.loadCRTScss === undefined)
        opts.loadCRTScss = true; // true is the old default.


    function _showUser(user) {

        let sessionStatusDiv = document.getElementById('sessionStatus');
        let userDiv = document.getElementById('user');

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

    if(opts.loadCRTScss)
        require('/main.css');

    if(opts.showHeader) {
        // Now that we know who the user is we can setup the session header
        require('/sessionHeader.css');
        require('/sessionHeader.htm', function(htm) {
            // We pull this html at the top of the body:
            var header = document.createElement('div');
            header.innerHTML = htm;
            document.body.insertBefore(header, document.body.firstChild);
            _showUser(user);
        });
    }


    if(user.name !== 'admin' || !opts.addAdminPanel) {
        _client(user, connectCallback);
        return;
    }



    /////////////////////////////////////////////////////////////////
    // At this point we are an "admin" so load contestAdmin.js
    // and other stuff for the admin.
    /////////////////////////////////////////////////////////////////

    // TODO: It would be hard to compile these require() calls out.
    // Currently the server will not get files from /admin/ for
    // non-admin users.
    //
    require('/admin/contestAdmin.js', function() {

        // This gets called after /admin/contestAdmin.css and /admin/contestAdmin.js
        // and all that they require() are loaded:
        //
        _client(user, function(client) {

            // contestAdminInit() is from '/admin/contestAdmin.js'
            // and it needs the client webSocket.  "This code here"
            // is being called before the webSocket receives any data,
            // but after the webSocket connection is open.
            contestAdminInit(client);

            if(connectCallback)
                connectCallback(client);
        });
    });
}
