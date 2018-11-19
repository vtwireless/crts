// opt.js: parse command line options and spew help

// You can add command line options here in options.  They
// will be made into the exported opt object that will have
// the default_value by default.  For example:
/*

  var opt = require('./opt');

  console.log( "http port = " + opt.http_port );

*/

console.log('loading "opt.js"');




var options = {

    // Put these options in the order which you want them to appear in the
    // --help.  So maybe alphabetical order by key name?

    debug: {

        type: 'bool',
        help: "Ran with more debugging spew."
    },

    help: {

        type: 'bool',
        default_value: false,
        help: "Print this help and then exit.",
        export_value: false, // do not expose in the opt object
        action: usage
    },

    http_port: {

        type: 'integer',
        default_value: 8180,
        argument: 'PORT',
        help: 'set the server HTTP port to PORT. ' +
            'The HTTP (non-secure) service is only available ' +
            'to localhost. Setting the PORT to "0" will ' +
            'disable this HTTP service.'
    },
    
    https_port: {

        type: 'integer',
        default_value: 9190,
        argument: 'SPORT',
        help: 'set the server HTTPS port to SPORT. ' +
            ' Setting the HTTPS port to "0" will ' +
            'disable this HTTPS service.'
    },


    server_address: {

        type: 'string',
        default_value: 'localhost',
        argument: 'ADDRESS',
        help: 'set the address or domain name that refers to this ' +
            'server from computers that access the stream programs ' +
            ' the access USRPs.' + 
            ' This address will be used to let the CRTS spectrum' +
            ' feed program and the CRTS Controllers connect to this server.'
    },


    spectrum_port: {

        type: 'integer',
        default_value: 9282,
        argument: 'PORT',
        help: 'set the spectrum reciever lessoning port to PORT.' +
            ' Setting the spectrum reciever server to "0" will ' +
            'disable this spectrum reciever service.  ' +
            'TODO: This serivce should have the client ' +
            'and this server running together behind the same firewall and ' +
            'otherwise this code should ride on a TLS layer.  ' +
            'TODO: Really this communication channel should be using ' +
            'WebRTC, but building the WebRTC C++ API is a total pain ' +
            'in the ass; time is money.'

    },


    radio_port: {

        type: 'integer',
        default_value: 9383,
        argument: 'PORT',
        help: 
            'set the CRTS_radio server port to PORT. ' +
            ' Yet another stupid lessoning port, for connecting to' +
            ' crts_radio programs. ' +
            'TODO: This serivce should have the client ' +
            'and this server running together behind the same firewall and ' +
            'otherwise this code should ride on a TLS layer. ' +
            'TODO: Really this communication channel should be using ' +
            'WebRTC, but building the WebRTC C++ API is a total pain ' +
            'in the ass; time is money.'
    },

    user: {

        argument: 'USERNAME',
        parse: function(user) {
            // Example:  user = 'teamA'
            
            assert(users[user] === undefined, "We alread have user " + user);

            users[user] = {
                        password: require('crypto').randomBytes(256).
                            toString('hex').substr(0, passwordLen)
            }
        },
        help:
            'set the name of a user in the contest.  Example:' +
            ' --user=hokies --user=eagles' +
            ' will make the two users hokies and eagles.  In addition' +
            ' to users added with this option there will always be a' +
            ' user named admin which will have super user control.'
    }
};


const passwordLen = 5; // number of hex chars

var users = {

    // We start with the admin user.
    admin: {
        password: require('crypto').randomBytes(256).
            toString('hex').substr(0, passwordLen)
    }
};


var keys =  Object.keys(options);

keys.forEach(function(key) {

    let option = options[key];

    if(option.type !== undefined && option.type !== 'bool') {
        if(option.default_value !== undefined)
            option.value = option.default_value;
    } else
        // bool options that are not set default to false.
        option.value = false;
});


var error = '';

function addError(str) {
    if(error.length > 0)
        error += ' ' + str;
    else
        error = '\n  bad option(s): ' + str;
}

var path = require('path'),
    process = require('process');

function usage() {

    var program_name = path.basename(require.main.filename);
    // strip off the .js suffix if there is one.
    if(program_name.substr(-3) === '.js')
        program_name = program_name.substr(0, program_name.length-3);
    var spaces = '                                                  ' +
        '                                                           ' +

        '                           ',
        right0 = 4,  // chars left margin
        right1 = 25, // position to left side of help
        min_width = 50, // minimum width
        width = 78;  // width of text

    if(error.length)
        Write(error + "\n\n");

    function Write(str) {
        process.stdout.write(str);
    }

    function print(str, right, next_right) {

        var i;
        var out = '';

        while(str.length > 0 && str.substr(0,1) == ' ') str = str.substr(1);

        out = spaces.substr(0,right) + str;
        if(arguments.length > 2)
            right = next_right;

        while(out.length > width) {
            for(i=width; i>min_width && out.substr(i,1) != ' '; --i);
            if(i == min_width)
                // We failed to find a space so show it all.
                i = width;

            Write(out.substr(0,i) + "\n");
            out = out.substr(i);
            while(out.length > 0 && out.substr(0,1) == ' ') out = out.substr(1);
            if(out.length > 0)
                out = spaces.substr(0,right) + out;
        }
        Write(out + "\n");
    }

    Write('\n  Usage: ' + program_name + " [OPTIONS]\n\n");
    print('Run a ' + program_name + ' HTTP(S)/webSocket server.', 2);
    Write("\n");
    Write("  ----------------------------------------------------------------------------\n");
    Write("  ---------------------------       OPTIONS        ---------------------------\n");
    Write("  ----------------------------------------------------------------------------\n\n");



    keys.forEach(function(key) {
        var pre = '--' + key;
        var opt = options[key];
        if(opt.argument !== undefined)
            pre += ' ' + opt.argument;

        if(pre.length < right1 - right0)
            pre += spaces.substr(0, right1 - right0 - pre.length);
        else
            pre += ' ';

        if(typeof(opt.default_print) === 'undefined' && opt.default_value) {
            var default_value = ' The default value of ' +
                    opt.argument + ' for this option is ';
            if(opt.type === 'string')
                default_value += '"' +
                    opt.default_value + '".';
            else if(opt.type === 'integer')
                default_value +=
                    opt.default_value + '.';
            else if(opt.type === 'regexp')
                default_value += '/' +
                    opt.default_value + '/.';
            else if(opt.type === 'bool') {
                // bool argument options just turn thing on unless they
                // are set to being on already which beats the point of it
                // being an option.

                assert(opt.type === 'bool');
                assert(opt.default_value == undefined);

                default_value = ' This is not set by default.';
            }
        } else {
            var default_value = '';
        }

        print(pre + opt.help + default_value, right0, right1);
        Write("\n");
    });

    printOptions(options);

    process.exit(1);
}

function printOptions(obj) {

    var keys = Object.keys(obj);

    console.log('\n  ===================== ' + keys.length +
        ' option values found ===================\n');

    i=0;
    keys.forEach(function(key) {

        var o = obj[key];
        if(o.value === undefined)
            var value = o.toString();
        else
            var value = o.value.toString();
        console.log('     ' + (i++) + '  ' + key + '=' +
                value);
    });

    console.log(
        '\n  =================================' +
        '===============================\n');
}


var alen = process.argv.length;

// Get command-line arguments into options value strings or value bool.
//
for(var i=2; i < alen; ++i) {
    var arg = process.argv[i];
    for(var k=0; k<keys.length; ++k) {
        var name = keys[k];
        var opt = options[name];
        var type = opt.type;
        var value = null;

        //console.log(name);
        if(type !== 'bool') {
            var optlen = arg.indexOf('=') + 1;
            if(('--'+name === arg || '-'+name === arg) && alen > i+1) {
                // --option val   -option val
                arg = process.argv[++i];
                value = arg;
            } else if(optlen > 0 && ('--'+name+'=' === arg.substr(0,optlen) ||
                        '-'+name+'=' === arg.substr(0, optlen)) &&
                        arg.length > optlen) {
                // --option=val
                value = arg.substr(optlen);
            }

            // TODO: add short options like the list command 'ls -al'
        }

        else if(type && type == 'bool') {
            if('--'+name === arg || '-'+name === arg) {
                // --option  -option
                value = true;
                if(opt.action !== undefined &&
                        typeof(opt.action) === 'function')
                    opt.action();
            }
        }

        if(value !== null) {
            if(typeof(opt.parse) === 'function')
                opt.parse(value);
            else
                opt.value = value;
            break;
        }
    }

    if(k === keys.length) 
        addError(arg);
}




if(error.length)
    usage();




// Get what the user needs from options.  The rest of the objects in this
// file will disappear.
//
opt = new Object;

keys.forEach(function(key) {
    var option = options[key];
    if(option.export_value === undefined || option.export_value === true) {
        var type = option.type;
        
        if(typeof(option.parse) === 'function')
            ;// ignore it
        else if(type === 'string' || type === 'regexp' || type === 'bool')
            opt[key] = option.value;
        else if(type === 'integer')
            opt[key] = parseInt(option.value);
        else {
            console.log("Missing type switch for option: " + key);
            process.exit(1);
        }
    }
});


printOptions(opt);

// Print the users

console.log("The users are:");
Object.keys(users).forEach(function(key) {
    console.log("    " + key + " with password: " + users[key].password);
});
console.log("");



// TODO: Change this if you add TLS key setting options.
//
// Let’s Encrypt is a free, automated, and open Certificate Authority.
// See:  https://letsencrypt.org/
//
opt.tls = {
    key : path.join(path.join(__dirname, "..", "etc"), 'private_key.pem'),
    cert: path.join(path.join(__dirname, "..", "etc"), 'public_cert.pem')
};

opt.users = users;


// Check that options are consistent and add some optional flags in the
// environment.
//
if(opt.debug && process.env.DEBUG === undefined)
    process.env.DEBUG = "express:*";


if(opt.http_port === opt.https_port) {
    console.log("Failed start: both the http and the https ports are " +
            opt.https_port);
    process.exit(1);
}




// Let the module user see opt and that is all.
//
// None of the code in this file is seen in other files except opt.
//
module.exports = opt;
