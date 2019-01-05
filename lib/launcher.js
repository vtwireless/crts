
var path = require('path'),
    fs = require('fs');

var opt = require('./opt');

var recurseSpewed = false;


// List of all available programs, and their corresponding data; that can
// be launched or could have been launched at some time when this server
// was running.  They are all the executables from in opt.launcher_dir.
var launcherPrograms = { };

// List of running programs and their corresponding data.
var runningPrograms = { };


// Async recursion -- it's crazy.
//
// Find all executable programs in dir.
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


    io.Emit('runningPrograms', runningPrograms);


    function broadcastProgramTally(lProgram) {

        assert(io.userName !== undefined);

        // We send a subset of the launcher program data, because we may
        // have data in the launcher program that only makes sense to this
        // server.
        //
        var data = {
            runCount: lProgram.runCount,
            numRunning: lProgram.numRunning,
        };

        BroadcastUser('programTally', 'admin',  lProgram.path, data);

        if(io.userName !== 'admin')
            BroadcastUser('programTally', io.userName, 
                lProgram.path, data);
    }

    function broadcastProgramRunStatus(rp, state) {

        // rp = running program data

        BroadcastUser('programRunStatus', 'admin',
            rp.path,  rp.pid, rp.args, state);

        if(io.userName !== 'admin')
            BroadcastUser('programRunStatus', io.userName,
                rp.path,  rp.pid, rp.args, state);
    }

    io.On('launch', function(path_in, args) {

        if(launcherPrograms[path_in] === undefined) {
            spew('requested to run unknow program: ' +
                    path_in);
            return;
        }

        var lProgram = launcherPrograms[path_in];

        spew('spawning: ' + path_in + ' ' + args);

        var fullPath = path.join(opt.launcher_dir, path_in);

        // We use a shell (shell: true) to parse the args for us.
        //
        if(args.length > 0)
            fullPath += ' ' + args;

        var child = spawn(fullPath, {
                cwd: process.cwd(),
                env: process.env,
                shell: true});

        var rp = runningPrograms[child.pid] = {

            pid: child.pid,
            path: path_in,
            args: args
        };
        broadcastProgramRunStatus(rp, 'running');

        // There is no ON SUCCESS event only an on error event.

        // Increase the program run count.
        //
        ++(lProgram.runCount);
        ++(lProgram.numRunning);
        broadcastProgramTally(lProgram);

        child.on('exit', function(statusCode) {

            --(launcherPrograms[path_in].numRunning);
            spew('program: "' + fullPath + '" exited with code ' + statusCode);
            broadcastProgramTally(lProgram);
            broadcastProgramRunStatus(rp, 'exited');
            delete runningPrograms[child.pid];
        });

        child.on('error', function(err) {

            spew('program: ' + fullPath + ' failed to start: ' + err);
            // TODO: The 'exit' event may handle sending the launcher
            // tally; well maybe not in all error cases...
        });
    });



    // MORE CODE HERE....

}


console.log('loaded ' + path.basename(__filename));

module.exports = initConnection;
