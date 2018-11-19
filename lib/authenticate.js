// Very simple crude authentication.  If you want more just export
// a authenticate() function from a different module/file.


var opt = require('./opt'),
    querystring = require('querystring'),
    url = require('url'),
    users = opt.users;


function checkPassword(user, password) {

    return users[user] !== undefined &&
         users[user].password === password;
}


function checkPasscodeCookie(request) {
    var list = {},
    rc = request.headers.cookie;

    // A valid cookie should contain:
    //   passcode=user,password
    //
    //   like:  passcode=joe,621b9
    //

    rc && rc.split(';').forEach(function(cookie) {
        var parts = cookie.split('=');
        list[parts.shift().trim()] = decodeURI(parts.join('='));
    });

    if(list.passcode !== undefined) {
        var a = list.passcode.split(',');
        if(a.length === 2 && checkPassword(a[0], a[1]))
            return a[0]; // a[0] is user
    }

    return false; // fail
}



//
// This module returns the authenticate() function
//
// This calls successCallback() on success and
// res.send("\nBad user and/or password\n\n"); res.end();
// on failure.
//
// Calls successCallback(user) on success or else it
// sends "Bad user and/or password"
//
module.exports = function(req, res, successCallback) {

    ////////////////////////////////////////////////////////////////////
    // Restrict access to this server.
    // We do this by:
    //
    //      Checking that a valid session cookie was sent
    //
    //                 or
    //
    //      A valid query with the user and password
    //
    //

 
    //console.log('  cookies=' + req.headers.cookie);

    var user;

    if((user = checkPasscodeCookie(req)) !== false) {
        // success due to good cookie.
        //
        // They must be accepting cookies and they sent a valid user and
        // password query in a past request.
        successCallback(user); // success
        return;
    }

    var success = false;
    var obj = querystring.parse(url.parse(req.url).query);
 
    if(obj.user !== undefined && obj.user !== undefined &&
        checkPassword(obj.user, obj.password)) {
        success = true;
    } else {
        console.log('rejected request from address:  ' +
            req.connection.remoteAddress +
            ' invalid user (' + obj.user + ') and password (' + obj.password +
            ') url=' + req.url + '  cookies=' + req.headers.cookie);
    }

    setTimeout(function() {

        // We delay response so that they cannot keep guessing quickly.
        // By not responding quickly to either a correct password or an
        // incorrect password they also cannot close the connection
        // assuming that the guess was incorrect due to the delay.  In
        // order to get good client server response they must except
        // cookies.

        if(success) {
            // The slow successful case.
            res.setHeader('Set-Cookie', 'passcode=' + obj.user +
                ',' + obj.password + '; expires=0; path=/;');
            successCallback(obj.user);
        } else {
            // The slow failure case.
            // Unset the cookie with expires=1.
            res.setHeader('Set-Cookie', 'passcode=user,password' +
                '; expires=1; path=/;');
            res.send("\nBad user and/or password\n\n");
            res.end();
        }

        // This means the time to guess the correct user and password will
        // be this time times the number of possible passwords, divided by
        // 2, divided by the number of simultaneous connections.  That
        // should be a long time, like more than a thousand years.

    }, 300/*milliseconds*/);
};
