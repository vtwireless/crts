
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
// Looks like this:  top key is PID:
// runningPrograms[pid] {
//     pid: pidNum,
//     path: pathString,
//     args: argsArray,
//     name: uniqueNameString
// };
//
var runningPrograms = { };


// programNameToRunningPrograms maps a running programName to
// runningPrograms[] entry, just so we do not need to search
// runningPrograms.
//
var programNameToRunningPrograms = { };



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

    //////////////////////////////////////////////////////////
    // The user could make a launch directory at any time.
    // If there is not a launch directory at this time we
    // just do not return any programs.
    //
    try {
        opt.launcher_dir = fs.realpathSync(opt.launcher_dir);
        if(!fs.lstatSync(opt.launcher_dir).isDirectory()) {
            console.log("The launcher directory \"" +
                opt.launcher_dir + "\" is not a directory.");
            userFunc(programs);
            return;
        }
    } catch(e) {
        console.log("The launcher directory \"" +
            opt.launcher_dir + "\" is not accessible yet: " + e);
        // The user is screwing with this server, but we just keep
        // it running with no launch-able programs.
        userFunc(programs);
        return;
    };
    //////////////////////////////////////////////////////////


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


    io.On('runningPrograms', function() {
        io.Emit('runningPrograms', runningPrograms);
    });

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
            rp.path,  rp.name, rp.pid, rp.args, state);

        if(io.userName !== 'admin')
            BroadcastUser('programRunStatus', io.userName,
                rp.path, rp.name,  rp.pid, rp.args, state);
    }

    // ARGUMENTS:
    //
    // programName is a unique ID for the launch of the program.  If
    //          A program with program name is running already we do
    //          not launch another.
    //
    // OPTIONS arg opts
    //
    // opts.runOne  bool
    //   if true and the path_in program is running already do not
    //   run it again.
    //
    io.On('launch', function(path_in, args, programName='', opts = {}) {


        if(programName !== '')
            if(undefined !== programNameToRunningPrograms[programName]) {
                spew('requested program path: "' + path_in +
                        '" with name: "' + programName + '" is running already');
                return true; // Found running program with name already.
           }

        // Set defaults for opts = {}
        if(opts.runOne === undefined)
            opts.runOne = false;


        if(opts.runOne) {
            // In this case we run only one copy of this program at a
            // time.
            if(undefined !== Object.keys(runningPrograms).find(function(pid) {
                // Look at all running programs and see if this path
                // is running now.
                if(runningPrograms[pid].path === path_in) {
                    spew('requested program (opts.runOne) "' + path_in +
                        '" is running already');
                    return true; // Found running program.
                }
            })) return; // We do not launch.
            
            // Else we try to launch/run it.
        }


        if(launcherPrograms[path_in] === undefined) {
            spew('requested to run unknow program: ' +
                    path_in);
            return;
        }



        var lProgram = launcherPrograms[path_in];

        spew('--------------------spawning: ' + path_in + ' ' + args);

        var fullPath = path.join(opt.launcher_dir, path_in);

        // We use a shell (shell: true) to parse the args for us.
        //
        if(args.length > 0)
            fullPath += ' ' + args;

        var env = process.env;
        ///////////////////////////////////////////////////////////////////
        // We pass information to the launched program through the
        // environment variables.  If you need to send more information to
        // your launched program from this web server consider adding an
        // environment variable here:
        //////////////////////////////////////////////////////////////////

        // Each launched program gets a unique name.
        if(programName === '') {
            programName = path_in;
            while(undefined !== programNameToRunningPrograms[programName])
                programName = '' + (env.LAUNCH_ID = launchCount++);
            while(undefined !== programNameToRunningPrograms[programName]);
        }

        env.PROGRAM_NAME = programName;

        // We tell the spectrum feed and all other programs the address
        // the they may connect to so that this server can relay spectrum
        // data.
        env.SERVER_ADDRESS = opt.server_address;


        // The port the this server is listening to for connections
        // that we replay spectrum data with.  The spectrum program is
        // crts_spectrumFeed.
        env.SPECTRUM_PORT = opt.spectrum_port;

        // It may good for a launched program to know:
        env.HTTPS_HOSTNAME = opt.https_hostname;
        env.HTTPS_PORT = opt.https_port;


        var child = spawn(fullPath, {
                cwd: process.cwd(),
                env: env,
                shell: '/bin/bash', 
                stdio: 'inherit'});

        var rp = runningPrograms[child.pid] = {

            pid: child.pid,
            path: path_in,
            args: args,
            name: programName
        };

        assert(undefined === programNameToRunningPrograms[programName]);
        programNameToRunningPrograms[programName] = rp;

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

            let name = runningPrograms[child.pid].name;
            if(name !== '') {
                assert(undefined !== programNameToRunningPrograms[name]);
                delete programNameToRunningPrograms[name];
            }

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


    // We return a function that can kill all the running programs.
    //
    var killAllPrograms = function() {

        Object.keys(runningPrograms).forEach(function(pid) {
            let sig = 'SIGTERM';
            try {
                process.kill(pid, sig);
                spew('signaled: ' + sig + ' to ' + runningPrograms[pid].path);
            } catch(err) {
                spew('signaling (' + sig +
                    ') ' + runningPrograms[pid].path + ' Failed: ' +
                    err);
            }
        });
    };

    return killAllPrograms;
}


// Note: this file is only loaded once, even if we have more than one
// require() that would load it.
console.log('loaded ' + path.basename(__filename));

module.exports = initConnection;
