
var path = require('path'),
    fs = require('fs');

var opt = require('./opt');

var recurseSpewed = false;


// List of all programs, and their corresponding data, that can be
// launched or could have been launched at some time when this server was
// running.
var launcherPrograms = { };


// Async recursion -- it's crazy.
//
function getProgramsList(dir, userFunc) {

    // This is the number of async calls we are waiting for:
    var waitCount = 0;
    
    // Directory recursion depth:
    var depth = 0;

    var programs = {};

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
                fs.realpath(path.join(dir, files[i].name),
                        function(err, fullPath) {
                    ++waitCount
                    fs.lstat(fullPath, function(err, stat) {
                        if(stat.isDirectory()) {
                            // Recurse
                            _getProgramsList(depth, fullPath);
                        } else if(stat.isFile() && stat.mode & 0001) {
                            // It is a file and it is executable by
                            // anyone.  We send the path relative to the
                            // launcher_dir.  So / is the launcher_dir.
                            var servicePath = fullPath.substr(opt.launcher_dir.length);
                            if(undefined === launcherPrograms[servicePath]) {
                                programs[servicePath] =
                                launcherPrograms[servicePath] = {
                                    path: servicePath,
                                    // number of times this program
                                    // launched for server run time.
                                    // This number only increases.
                                    runCount: 0,
                                    numRunning: 0 // currently running
                                };
                            } else {
                                programs[servicePath] = launcherPrograms[servicePath];
                            }
                            // TODO: We need to add a method that changes
                            // programs from the clients when the files
                            // are added or removed.

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

const { spawn } = require('child_process');


// List of all programs that are running.
var running = [];


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
            io.Emit('receiveLauncherPrograms', programs);
        });
    });

    io.On('launch', function(path_in) {

        if(launcherPrograms[path_in] === undefined)
            return;

        spew('spawning: ' + path_in);

        var fullPath = path.join(opt.launcher_dir, path_in);

        var child = spawn(fullPath, { cwd: process.cwd(),  env: process.env });

        // There is no ON SUCCESS event only an on error event.

        // Increase the program run count.
        //
        ++(launcherPrograms[path_in].runCount);
        ++(launcherPrograms[path_in].numRunning);

        Broadcast('updateRunState', path_in, launcherPrograms[path_in]);

        child.on('exit', function(statusCode) {

            --(launcherPrograms[path_in].numRunning);
            spew('program: ' + fullPath + ' exited with code ' + statusCode);
            Broadcast('updateRunState', path_in, {
                // We only need to send what changed:
                numRunning: launcherPrograms[path_in].numRunning
            });

        });

        child.on('error', function(err) {

            spew('program: ' + fullPath + ' failed to start: ' + err);
        });


    });



    // MORE CODE HERE....

}


console.log('loaded ' + path.basename(__filename));

module.exports = initConnection;
