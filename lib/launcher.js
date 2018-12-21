
var path = require('path'),
    fs = require('fs');

var opt = require('./opt');



// We initialize this code:
//
// We are passed in io - a SocketIO like object
// that has On and Emit methods.
//
// This is called for each webSocket IO thingy
function initConnection(io, spew) {

    spew('initializing ' + path.basename(__filename));


    io.On('getLauncherPrograms', function() {
        
        programs = [];

        






    });



    // MORE CODE HERE....

}


console.log('loaded ' + path.basename(__filename));

module.exports = initConnection;
