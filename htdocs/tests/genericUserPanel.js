// TODO: A lot of code in his file is replicated in
// ../admin/contestAdmin.js

require('/session.js');
require('/showHide.js');
require('/tests/genericUserPanel.css');


function GenricUserPanel(io, parentElement=null) {


    function AddLauncherPanel(programs) {

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
            th.appendChild(document.createTextNode('unique name'));
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

        io.On('receiveLauncherPrograms', function(programs_in, permissions) {

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

                var programName = document.createElement('input');
                programName.type = 'text';
                programName.value = '';


                var td = document.createElement('td');
                td.title = 'launch';
                td.className = 'launcher';
                td.appendChild(document.createTextNode(path));
                tr.appendChild(td);
                td.onclick = function() {
                    console.log("launch program=\"" + path + ' ' +
                        argsInput.value + '" name= "' +
                        programName.value + '"');
                    io.Emit('launch', path, argsInput.value, programName.value);
                };

                td = document.createElement('td');
                td.className = 'args';
                // The user may or may not set a program name
                // The program name must be unique to this server.
                td.appendChild(programName);
                tr.appendChild(td);

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
    function AddRunningProgramsPanel() {

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
            th.appendChild(document.createTextNode('unique name'));
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


        function addProgram(path, name, pid, args) {

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
            td.appendChild(document.createTextNode(name));
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
            span.title = 'send signal';
            td.appendChild(span);
            td.appendChild(input);
            tr.appendChild(td);
            table.appendChild(tr);
        }


        io.On('programRunStatus', function(path, name, pid, args, state) {

            if(programs[pid] === undefined && state !== 'exited')
                addProgram(path, name, pid, args);
            else if(programs[pid] !== undefined && state === 'exited') {

                // Remove this from the panel.
                table.removeChild(programs[pid].tr);
                delete programs[pid];
            }
        });


        io.On('runningPrograms', function(runningPrograms) {

            console.log("got 'runningprograms': " +
                JSON.stringify(runningPrograms));

            var keys = Object.keys(runningPrograms);
            keys.forEach(function(pid) {
                var rp = runningPrograms[pid];
                addProgram(rp.path, rp.name, rp.pid, rp.args);
            });
        });

    }


    function AddControllerPanels() {

        var controllers = {};

        function setParameterMinMax(type, programName, controlName,
                    parameterName, value) {
            try {
                // Set the value if we can; if not then don't.
                controllers[programName].
                        set[controlName][parameterName]
                        [type + 'Input'].value = value;
            } catch {
                console.log("Cannot set controllers[" +
                        [].slice.call(arguments) + "]." +
                        type + 'Input.value')
            }
        }


        io.On('setParameterMin', function(programName, controlName,
                    parameterName, value) {

            setParameterMinMax('min', programName, controlName,
                    parameterName, value);
        });


        io.On('setParameterMax', function(programName, controlName,
                    parameterName, value) {

            setParameterMinMax('max', programName, controlName,
                    parameterName, value);
        });


        function getParameter(programName, controlName, parameter, id) {

            console.log('getParameter ' + parameter + " id=" + id);

            io.Emit('getParameter', programName, controlName,
                parameter, id);
        }

        function makeId(elementType, programName, controlName, parameter) {

            var ret = "";
            for(var i=0; i < arguments.length; ++i) {
                ret += arguments[i];
                ret += '_';
            }
            return ret;
        }

        // This will keep adding controller panels to the ContestPanel as
        // they come up.

        // makeControllerTable() makes a set or get table for a controller.
        //
        // type is "set" or "get"
        //
        function makeControllerTable(type, setOrGet, parentNode, programName) {

            var div = document.createElement('div');
            div.className = type;
            {
                let h3 = document.createElement('h3');
                h3.className = type;
                h3.appendChild(document.createTextNode(type));
                div.appendChild(h3);
            }
            var table = document.createElement('table');
            table.className = type;
            div.appendChild(table);
            var tr = document.createElement('tr');
            table.appendChild(tr);
            {
                // Content of tr is table headers
                //
                var th = document.createElement('th');
                th.className = type;
                th.appendChild(document.createTextNode('Filter:Parameter'));
                tr.appendChild(th);

                if(type === 'get') {
                    th = document.createElement('th');
                    th.className = type;
                    th.appendChild(document.createTextNode('value'));
                    tr.appendChild(th);
                } else {
                    // type === 'set'
                    th = document.createElement('th');
                    th.className = type;
                    th.appendChild(document.createTextNode('set to:'));
                    tr.appendChild(th);

                    th = document.createElement('th');
                    th.className = type;
                    th.appendChild(document.createTextNode('min'));
                    tr.appendChild(th);

                    th = document.createElement('th');
                    th.className = type;
                    th.appendChild(document.createTextNode('max'));
                    tr.appendChild(th);
                }
            }

            Object.keys(setOrGet).forEach(function(controlName) {

                var parameters = Object.keys(setOrGet[controlName]);

                function makeSetterInput(setWhat, programName, controlName,
                                parameterName, initValue,
                                type=null/*or 'min' or 'max'*/) {

                    td = document.createElement('td');
                    td.className = 'setValue';
                    let input = document.createElement('input');
                    input.size = 8; // makes it smaller than default length
                    input.type = 'text';
                    input.onchange = function() {
                        console.log('io.Emit(' + "'" + setWhat + "'," + [
                            programName, controlName,
                            parameterName, parseFloat(input.value)] + ')');

                        io.Emit(setWhat, programName, controlName,
                            parameterName, parseFloat(input.value));
                    };

                    let button = document.createElement('button');
                    button.type = 'button';
                    button.appendChild(document.createTextNode('submit'));
                    button.onclick = function() {
                        input.onchange();
                    };
                    if(initValue !== null)
                        input.value = initValue;
                    td.appendChild(button);
                    td.appendChild(input);
                    return td;
                }

                parameters.forEach(function(parameterName) {
                    // Make a row for this parameter
                    //
                    var tr = document.createElement('tr');
                    table.appendChild(tr);

                    var td = document.createElement('td');
                    td.className = type;
                    td.appendChild(document.createTextNode(
                        controlName + ":" + parameterName));
                    tr.appendChild(td);

                    if(type === "get") {
                        td = document.createElement('td');
                        td.className = 'getvalue';
                        td.id = makeId('getTD', programName, controlName, parameterName);
                        td.appendChild(document.createTextNode('value'));
                        tr.appendChild(td);
                        io.Emit('subscribeToGetter', programName, controlName, parameterName)
                    } else {
                        // type === 'set'
                        td = makeSetterInput('setParameter', programName, controlName,
                                parameterName, null);
                        tr.appendChild(td);

                        td = document.createElement('td');
                        td.className = 'minMax';
                        td.innerHTML = setOrGet[controlName][parameterName].min.toString();
                        tr.appendChild(td);

                        td = document.createElement('td');
                        td.className = 'minMax';
                        td.innerHTML = setOrGet[controlName][parameterName].max.toString();
                        tr.appendChild(td);
                      }
                });
            });

            parentNode.appendChild(div);
        }


        function appendContestTable(controller, programName,
            set, get, image) {

            // Add a new controller <div>
            var controllerDiv = controller.div =
                document.createElement('div');
            controllerDiv.className = "controller";
            let h3 = document.createElement('h3');
            let span = document.createElement('span');
            h3.appendChild(document.createTextNode('Controllers for '));
            span.appendChild(document.createTextNode(programName));
            span.className = 'controller';
            h3.appendChild(span);
            controllerDiv.appendChild(h3);

            contestPanel.appendChild(controllerDiv);

            var img = document.createElement('img');
            console.log('adding image: ' + image);
            img.src = image;

            img.className = 'controller';
            controllerDiv.appendChild(img);

            makeControllerTable("set", set, controllerDiv, programName);
            makeControllerTable("get", get, controllerDiv, programName);
            makeShowHide(controllerDiv, { header: h3 });

            return controllerDiv;
        }


        io.On('addController', function(programName, set, get, image) {

            console.log('Got On("addController",) program="' +
                programName + '"' +
                '\n    set=' + JSON.stringify(set) +
                '\n    get=' + JSON.stringify(get) +
                '\n  image=' + image);

            var controller = controllers[programName] = {
                programName: programName,
                set: set,
                get: get
            };

            var div = appendContestTable(controller, programName, set, get, image);

            controller.destroy = function() {
                console.log('removing controller for ' + programName);
                // remove widget
                div.parentNode.removeChild(div);
                // delete controller from list
                delete controllers[this.programName];
            }
        });


        io.On('getParameter', function(programName,
                controlName, parameter, value) {

            //console.log("GOT socketIO event 'getParameter'," +
            //        "args=" + [].slice.call(arguments));

            if(controllers[programName] === undefined) return;


            if(true) {
                var node = document.getElementById(makeId(
                    'getTD', programName, controlName, parameter));
                if(node) {
                    node.innerHTML = value.toString();
                }
            }
        });


        io.On('removeController', function(programName, set, get) {

            console.log('Got On("removeController",) program="' +
                programName);

            if(controllers[programName] === undefined) return;
            controllers[programName].destroy();
        });
    }


    var contestPanel = document.createElement('div');
    contestPanel.className = "userPanel";

    let h = document.createElement('h3');
    h.appendChild(document.createTextNode('CRTS ' +
            'Generic User Test Panel'));
    h.className = 'userPanel';
    contestPanel.appendChild(h);


    if(typeof(parentElement) !== 'string' && parentElement !== null)
        parentElement.appendChild(contestPanel);
    else if(typeof(parentElement) === 'string')
        parentElement = document.getElementById(parentElement);
    else
        parentElement = document.body;

    parentElement.appendChild(contestPanel);

    // We user could add this  userPanel <div> as a child to whatever
    // they like.
    this.getElement = function() { return contestPanel; };

    makeShowHide(contestPanel, {header: h, startShow: true});


    ///////////////////////////////////////////////////////////////////
    //   Add panels
    ///////////////////////////////////////////////////////////////////

    AddLauncherPanel();
    AddRunningProgramsPanel();
    AddControllerPanels();


    // Tell the server more of the stuff we want.
    io.Emit('runningPrograms');
    io.Emit('getLauncherPrograms');

    console.log("Made user panel");
}


