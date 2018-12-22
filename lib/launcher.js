
var path = require('path'),
    fs = require('fs');

var opt = require('./opt');

var recurseSpewed = false;


// Async recursion -- it's crazy.
//
function getProgramsList(dir, userFunc) {

    // This is the number of async calls we are waiting for:
    var waitCount = 0;
    
    // Directory recursion depth:
    var depth = 0;

    var programs = [];

    // We need to call this at the end of every callback, because we
    // do not know the order of the callback calls.
    function checkReturn() {
        --waitCount;
        if(waitCount === 0)
            userFunc(programs);
    }

    // Recursing function.
    function _getProgramsList(depth, dir) {

        ++depth;

        if(depth > 40) {
            // There are too many subdirectories.  There must be a limit.
            // We can't let this ill configured server keep recursing.
            if(!recurseSpewed) {
                console.log("launcher programs has too many subdirectories, " +
                    depth + " directories deep in " + opt.launcher_dir);
                // Warn just once.
                recurseSpewed = true;
            }
            return;
        }

        //console.log("dir=" + dir + " waitCount=" + waitCount);

        ++waitCount;
        //
        fs.readdir(dir, { withFileTypes: true }, function(err, files) {
            for(var i in files) {
                // Resolve symlinks and re-get what this file is;
                // so that this server follows symlinks.
                ++waitCount
                //
                fs.realpath(path.join(dir, files[i].name), function(err, fullPath) {
                    ++waitCount
                    fs.lstat(fullPath, function(err, stat) {
                        if(stat.isDirectory()) {
                            // Recurse
                            _getProgramsList(depth, fullPath);
                        } else if(stat.isFile() && stat.mode & 0001) {
                            // it is a file and it is executable by
                            // anyone.
                            programs.push(fullPath);
                            //console.log("programs=" + programs);
                        }
                        checkReturn();
                    });
                    checkReturn();
                });
            }
            checkReturn();
        });
    }

    _getProgramsList(0, dir);
}



// We initialize this code:
//
// We are passed in io - a SocketIO like object
// that has On and Emit methods.
//
// This is called for each webSocket IO thingy
function initConnection(io, spew) {

    spew('initializing ' + path.basename(__filename));

    io.On('getLauncherPrograms', function() {

        getProgramsList(opt.launcher_dir, function(programs) {
            spew("got programs: " + programs);
            io.Emit('launcherPrograms', programs);
        });
    });



    // MORE CODE HERE....

}


console.log('loaded ' + path.basename(__filename));

module.exports = initConnection;
