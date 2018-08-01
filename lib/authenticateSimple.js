// Very simple crude authentication.  If you want more just export
// a authenticate() function from a different module/file.


var opt = require('./opt'),
    querystring = require('querystring'),
    url = require('url'),
    passcode = opt.passcode;


if(passcode === 'auto_generated')
    // If 'crypto' is good stuff, then we will be secure.
    passcode = require('crypto').randomBytes(256).toString('hex').substr(0, 4);


// We export passcode too.
module.exports.passcode = passcode;
console.log('The passcode is: ' + passcode);


function parseCookies(request) {
    var list = {},
    rc = request.headers.cookie;

    rc && rc.split(';').forEach(function( cookie ) {
        var parts = cookie.split('=');
        list[parts.shift().trim()] = decodeURI(parts.join('='));
    });

    return list;
}



//
// This module returns the authenticate() function
//
//  authenticate() returns false on success
//  else returns true on failure.
//
module.exports.authenticate = function(req, res) {

    ////////////////////////////////////////////////////////////////////
    // Restrict access to this server.
    // We do this by:
    //
    //      Checking that a valid session cookie (passcode) was sent
    //
    //                 or
    //
    //      A valid query with the passcode
    //
    //

    // TODO: Note this does not ID the client.  All valid clients get the
    // same cookie.  The webSocket connection will ID (session) the client
    // later.

console.log('  cookies=' + req.headers.cookie);
    

    var cookies = parseCookies(req);

    if(cookies.passcode !== undefined && cookies.passcode === passcode)
        // success due to good cookie.
        // They must be accepting cookies and they sent a passcode query
        // in a past request.
        return { success: true, method: 'cookie' };

    // If the browser does not set cookies they could instead
    // send the passcode on every query.

    var obj = querystring.parse(url.parse(req.url).query);

    if(obj.passcode === passcode) {

        res.setHeader('Set-Cookie', 'passcode=' + passcode + '; expires=0; path=/;');

        return { success: true, method: 'query passcode'};
    }

    console.log('rejected request from address:  ' +
        req.connection.remoteAddress +
        ' invalid passcode="' + obj.passcode +
        '" != ' + passcode + '  url=' + req.url + '  cookies=' + req.headers.cookie);

    return { success: false, method: 'query passcode' };

};
