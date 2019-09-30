require('/showHide.js');
require('/admin/contestAdmin.css');


function launcherInit(io, parentElement=null) {

    //  _addLauncherPanel() makes HTML that is a clickable list of programs
    //  that we can launch on the server by clicking on the client browser.
    //
    function _addLauncherPanel(io, contestPanel) {


        var programs = null;

        function displayLauncher(launcher) {

            // TODO: this text node could be made into
            // a table row.
            //
            launcher.runCountText.data = launcher.runCount.toString();

            launcher.numRunningText.data = launcher.numRunning.toString();
        }

        var div = document.createElement('div');
        div.className = 'launcher';

        contestPanel.appendChild(div);
        // Make this a show/hide clickable thing.
        makeShowHide(div, { header: 'Launch' });

        var table = null;

        function makeTable() {

            if(table) div.removeChild(table);

            table = document.createElement('table');
            table.className = 'launcher';

            var header_tr = document.createElement('tr');
            header_tr.className = 'launcher';


            let th = document.createElement('th');
            th.className = 'launcher';
            th.appendChild(document.createTextNode('program'));
            header_tr.appendChild(th);

            th = document.createElement('th');
            th.className = 'launcher';
            th.appendChild(document.createTextNode('run with arguments'));
            header_tr.appendChild(th);

            th = document.createElement('th');
            th.className = 'launcher';
            th.appendChild(document.createTextNode('number running'));
            header_tr.appendChild(th);

            th = document.createElement('th');
            th.className = 'launcher';
            th.appendChild(document.createTextNode('run count'));
            header_tr.appendChild(th);

            table.appendChild(header_tr);
            div.appendChild(table);
        }

        makeTable();
 

        io.Emit('getLauncherPrograms');

        io.On('receiveLauncherPrograms', function(programs_in) {

            var programNames = Object.keys(programs_in);

            if(programNames.length < 1) {
                console.log('got NO receiveLauncherPrograms');
                return;
            }

 
            if(programs) {
                // remove the old list
                makeTable();
                delete programs;
            }
            programs = {};

            programNames.forEach(function(key) {

                var path = key;
                var program = programs_in[key];

                assert(path === programs_in[key].path, "program != path");

                var tr = document.createElement('tr');
                tr.className = 'launcher';

                // All paths will start with '/' so they will not conflict
                // with objects keys in programs that do not start with '/'.
                //
                var launcher = programs[path] = { path: path};
                Object.keys(program).forEach(function(key) {
                    launcher[key] = program[key];
                });


                var argsInput = document.createElement('input');
                argsInput.type = 'text';
                argsInput.value = '';


                var td = document.createElement('td');
                td.className = 'launcher';
                td.appendChild(document.createTextNode(path));
                tr.appendChild(td);
                td.onclick = function() {
                    console.log("launch program=" + path + ' ' +
                        argsInput.value);
                    io.Emit('launch', path, argsInput.value);
                };

                td = document.createElement('td');
                td.className = 'args';
                td.appendChild(argsInput);
                tr.appendChild(td);

                td = document.createElement('td');
                td.className = 'numRunning';
                launcher.numRunningText = document.createTextNode('');
                td.appendChild(launcher.numRunningText);
                tr.appendChild(td);

                td = document.createElement('td');
                td.className = 'runCount';
                launcher.runCountText = document.createTextNode('');
                td.appendChild(launcher.runCountText);
                tr.appendChild(td);

                table.appendChild(tr);
                displayLauncher(launcher);
            });
        });

        io.On('programTally', function(path, program) {

            // This updates the table of programs giving a count
            // of the number of launched programs and the number
            // of running programs.

            if(undefined === programs[path]) return;
            var launcher = programs[path];
            Object.keys(program).forEach(function(key) {
                launcher[key] = program[key];
            });

            displayLauncher(launcher);
        });
    }


    // Make a panel for all the currently running programs.
    //
    // Programs may be run more than once so we need a list
    // of them with a unique id for each program, the PID.
    //
    // We can preform actions on the running programs with
    // this panel.
    //
    function _addRunningProgramsPanel(io, contestPanel) {

        var programs = {};
        var table;

        var topDiv = document.createElement('div');
        topDiv.className = 'programs';
        contestPanel.appendChild(topDiv);

        var table = document.createElement('table');
        table.className = 'programs';

        {
            let tr = document.createElement('tr');

            let th = document.createElement('th');
            th.appendChild(document.createTextNode('program'));
            th.className = 'programs';
            tr.appendChild(th);

            th = document.createElement('th');
            th.appendChild(document.createTextNode('args'));
            th.className = 'programs';
            tr.appendChild(th);

            th = document.createElement('th');
            th.appendChild(document.createTextNode('PID'));
            th.className = 'programs';
            tr.appendChild(th);

            th = document.createElement('th');
            th.appendChild(document.createTextNode('signal'));
            th.className = 'programs';
            tr.appendChild(th);

            table.appendChild(tr);

            topDiv.appendChild(table);
            makeShowHide(topDiv, { header: 'Running Programs' });
        }


        function addProgram(path, pid, args) {

            let tr = document.createElement('tr');

            programs[pid] = {
                path: path,
                pid: pid,
                args: args,
                tr: tr
            };


            let td = document.createElement('td');
            td.className = 'programs';
            td.appendChild(document.createTextNode(path));
            tr.appendChild(td);

            td = document.createElement('td');
            td.className = 'programs';
            td.appendChild(document.createTextNode(args));
            tr.appendChild(td);

            td = document.createElement('td');
            td.className = 'programs';
            td.appendChild(document.createTextNode(pid.toString()));
            tr.appendChild(td);

            td = document.createElement('td');
            td.className = 'programs';
            let input = document.createElement('input');
            input.type = 'text';
            input.value = 'SIGTERM';
            let span = document.createElement('span');
            span.className = 'program_signal';
            span.appendChild(document.createTextNode('signal'));
            span.onclick = function() {
                io.Emit('signalProgram', pid, input.value);
            };
            td.appendChild(span);
            td.appendChild(input);
            tr.appendChild(td);
            table.appendChild(tr);
        }


        io.On('programRunStatus', function(path, pid, args, state) {

            if(programs[pid] === undefined && state !== 'exited') {

                addProgram(path, pid, args);

            } else if(programs[pid] !== undefined && state === 'exited') {

                // Remove this from the panel.
                table.removeChild(programs[pid].tr);
                delete programs[pid];
            }
        });


        io.Emit('runningPrograms');

        io.On('runningPrograms', function(runningPrograms) {

            var keys = Object.keys(runningPrograms);
            keys.forEach(function(pid) {
                var rp = runningPrograms[pid];
                addProgram(rp.path, rp.pid, rp.args);
            });
        });

    }


    /////////////////////////////////////////////////////////////////////
    // That's it for the functions in this function.  Now do stuff.

    var contestPanel = document.createElement('div');
    contestPanel.className = "contestPanel";

    var h = document.createElement('h3');
    h.appendChild(document.createTextNode('CRTS ' +
            'Launcher Panel'));
    h.className = 'contestPanel';
    contestPanel.appendChild(h);

    if(typeof(WTApp) === 'function' && false/* this does not work yet*/) {

        // It's just not compatible yet.
        new WTApp('Control Panel', contestPanel);

    } else {
        if(parentElement) parentElement.appendChild(contestPanel);
        else {
            parentElement = document.getElementById('contestAdminPanel');
            if(parentElement)
                parentElement.appendChild(contestPanel);
            else
                document.body.appendChild(contestPanel);
        }
    }

    // We user could add this  contestPanel <div> as a child to whatever
    // they like.
    this.getElement = function() { return contestPanel; };


    let showHide = makeShowHide(contestPanel, {header: h, startShow: false});

    console.log('created contest panel');


    /******************* Make three more panels ************/
    // We add panels in this order.
    //
    _addLauncherPanel(io, contestPanel);
    _addRunningProgramsPanel(io, contestPanel);

    // This could not be hidden until all the child widgets where
    // added.  Now that we are done we can hide it.
    //showHide.hide();
}
