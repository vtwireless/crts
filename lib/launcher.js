
var path = require('path'),
    fs = require('fs');

var opt = require('./opt');

var recurseSpewed = false;


// List of all available programs, and their corresponding data; that can
// be launched or could have been launched at some time when this server
// was running.  They are all the executables from in the directory:
// opt.launcher_dir.
//
var launcherPrograms = { };

// List of running programs and their corresponding data.  This,
// runningPrograms, data is shared as is with the browser clients.
//
var runningPrograms = { };

var launchCount = 0;


// This is a globally accessible function:
// This returns a unique decsription.
launcher_getDescription = function(pid) {

    if(runningPrograms[pid] === undefined)
        return pid.toString(); // Not a great answer.

    // We must make this a unique description, so we
    // prepend the pid.
    //
    let ret = pid.toString() + ' ' + runningPrograms[pid].path;
    if(runningPrograms[pid].args.length > 0)
        ret += ' ' + runningPrograms[pid].args;

    return ret;
}


// Async recursion -- it's crazy.
//
// Find all executable programs in dir.  This is used to find all the
// launcherPrograms at the time of a client request.
//
// TODO: Push launcherPrograms to the connected clients any time a change
// is found.
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
                console.log("WARNING: " +
                    "launcher programs has too many subdirectories, " +
                    depth + " directories deep in " + opt.launcher_dir);
                // Warn just once.
                recurseSpewed = true;
            }
            return;
        }

        function addProgram(fullPath) {
            // It is a file and it is executable by
            // anyone.  We send the path relative to the
            // launcher_dir.  So / is the launcher_dir as
            // seen from the web browsers.
            //
            let servicePath = fullPath.substr(opt.launcher_dir.length);
            //console.log('servicePath=' + servicePath);
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
        }

        function checkPath(fullPath, realPath=null, depth=0) {

            ++depth;

            if(depth > 20) {
                console.log("Too many sym links at " + fullPath);
                return;
            }

            if(realPath === null) realPath = fullPath;

            ++waitCount;
            //
            fs.lstat(realPath, function(err, stat) {

                if(stat.isDirectory())
                    // Recurse
                    _getProgramsList(depth, fullPath);
                else if(stat.isFile() && stat.mode & 0001)
                    addProgram(fullPath);
                else if(stat.isSymbolicLink()) {

                     ++waitCount;
                    //
                    fs.realpath(fullPath, function(err, realPath) {
                        // Recurse checkPath().
                        checkPath(fullPath, realPath, depth);
                        checkReturn(); // fs.realpath()
                    });
                }
                checkReturn(); // fs.lstat()
            });
        }

        //console.log("dir=" + dir + " waitCount=" + waitCount);

        ++waitCount;
        //
        fs.readdir(dir, { withFileTypes: true }, function(err, files) {
            for(var i in files) {
                // Resolve symlinks and re-get what this file is;
                // so that this server follows symlinks.
                checkPath(path.join(dir, files[i].name));
            }
            checkReturn(); // fs.readdir()
        });
    }

    _getProgramsList(0, dir);
}

const { spawn } = require('child_process');


// We initialize this code:
//
// We are passed in io - a SocketIO like object
// that has On and Emit methods that are associated
// with the webSocket connection.
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

        var env = process.env;
        env.LAUNCH_ID = ++launchCount;

        var child = spawn(fullPath, {
                cwd: process.cwd(),
                env: env,
                shell: '/bin/bash', 
                stdio: 'inherit'});

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

        child.on('exit', function(statusCode, signal) {

            --(launcherPrograms[path_in].numRunning);
            if(signal)
                spew('program: "' + fullPath +
                    '" exited due to ' +
                    signal + ' signal');
            else
                spew('program: "' + fullPath +
                    '" exited with code ' + statusCode);
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


    io.On('signalProgram', function(pid, signalType) {

        if(runningPrograms[pid] !== undefined) {

            spew('signaling ' + pid + ' (' + signalType +
                ') ' + runningPrograms[pid].path);

            var sig = signalType;
            if(signalType.substr(0,3) !== 'SIG')
                sig = 'SIG' + signalType;

            try {
                process.kill(pid, sig);
            } catch(err) {
                spew('signaling (' + sig +
                    ') ' + runningPrograms[pid].path + ' Failed: ' +
                    err);
            }
        }
    });


    // MORE CODE HERE....

}


console.log('loaded ' + path.basename(__filename));

module.exports = initConnection;
