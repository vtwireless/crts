
// Developer friendly utility functions fail() and assert() are not so
// user friendly.
//
function fail() {

    // TODO: add stack trace or is browser debugger enough?
    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    line = '\n--------------------------------------------------------';
    text += line + '\nCALL STACK' + line + '\n' +
        new Error().stack + line;
    console.log(text);
    alert(text);
    window.stop();
    throw text;
}

function assert(val, msg=null) {

    if(!val) {
        if(msg)
            fail(msg);
        else
            fail("JavaScript failed");
    }
}

function getElementById(id) {

    var element = document.getElementById(id);
    if(!element) fail("document.getElementById(" + id + ") failed");
    return element;
}


// We did not want to expose this function so we put an underscore
// in the start of it's name.
//
function _client(user, connectCallback=null) {

    var url = location.protocol.replace(/^http/, 'ws') +
        '//' + location.hostname + ':' + location.port + '/';

    var ws = new WebSocket(url);

    assert(typeof(createSocketIOObject) === 'function',
        'script /js/socketIO.js was not loaded yet');

    // This function call will add the, socketIO like, On() and Emit()
    // methods to ws.  Also adds ws.CheckForSocketIOMessage() which eats
    // messages if they of a form that we use to make this socketIO like
    // On() interfaces.

    // io will be the exposed "public" object.
    var io = {};

    createSocketIOObject(io, function(msg) { ws.send(msg); });

    ws.onmessage = function(e) {

        var message = e.data;

        if(io.CheckForSocketIOMessage(message))
            // We got a socketIO like message and it was
            // handled.
            return;

        console.log("got webSocket message=" + message);
    };


    ws.onopen = function(e) {

        console.log('got webSocket to: ' + url);

        // If this is an invalid attempt than the server will disconnect.

        ws.send(JSON.stringify({
            name: user.name, password: user.password
        }));

        // It is possible that a browser client can be trying to spoof the
        // web server with in-correct info, but in that case the webSocket
        // connection will be dropped unless they can guess a correct
        // password and ws.send() it correctly.  So at this point we can
        // say we are completely connected and ready to go.  I.E. waiting
        // for a reply to the send above is pointless.

        getElementById('sessionStatus').innerHTML = "Connected as user: ";

        if(connectCallback) {

            connectCallback(io);
        }
    };

    ws.onclose = function(e) {

        console.log('closed webSocket to: ' + url);

        getElementById('sessionStatus').innerHTML = "Not logged in";
        getElementById('user').innerHTML = '';


        // Here is how we could erase the passcode cookie:
        //document.cookie = 'passcode=; Max-Age=-99999999;';
    };
}


function createClient(connectCallback=null) {


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

        getElementById('sessionStatus').innerHTML = "Authenticated as user: ";
        getElementById('user').innerHTML = user.name;

        return user;
    }

    var user = getAuthicatedUser();

    console.log("We are authenticated as user: " + user.name);

    if(user.name !== 'admin') {

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


(function() {

    // If the script src ends in "run", like:
    //
    //   <script src="/js/session.js?run"></script>,
    //
    // then this will create a client, otherwise this will leave it up to
    // the rest of the page code to do what is coded.
    //
    let scripts = document.getElementsByTagName('script');
    let lastScript = scripts[scripts.length-1];
    let doRun = ((lastScript.src.search(/run$/))===(-1))?false:true;

    if(doRun)
        onload = function() { createClient(); };

})();

