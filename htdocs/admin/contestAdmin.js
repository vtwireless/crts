require('/showHide.js');
require('/admin/contestAdmin.css');


function contestAdminInit(io, parentElement=null) {

    function _addControllerPanels(io, contestPanel, users) {

        var controllers = {};

        io.On('changePermission', function(userName, programName, setOrGet,
                controlName, parameterName, boolValue) {

                console.log("On 'changePermission' (" +
                    [].slice.call(arguments) + ')');

            var access = controllers[programName][setOrGet][controlName]
                [parameterName][userName];
            if(boolValue !== access.checkbox.checked) {
                access.checkbox.checked = boolValue;
            }
        });


        function getParameter(programName, controlName, parameter, id) {

            console.log('getParameter ' + parameter + " id=" + id);

            _contest.io.Emit('getParameter', programName, controlName,
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

        // makeActionTable() makes a set or get table for a controller.
        //
        // type is "set" or "get"
        //
        function makeActionTable(type, setOrGet, parentNode, programName) {

            function checkbox(userName, controlName, parameterName, value) {

                var input = document.createElement('input');
                input.type = 'checkbox';
                input.checked = value;
                input.onchange = function() {

                    // TODO: We need to group commands sent like this ...

                    io.Emit('changePermission', userName, programName, type,
                        controlName, parameterName, input.checked);
                    console.log('changePermission ("' +  userName +
                            '","' + controlName +
                            '","' + parameterName +
                            '","' + type + '")');
                };

                return input;
            }

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
                }

                users.forEach(function(userName) {
                    th = document.createElement('th');
                    th.className = type;
                    th.appendChild(document.createTextNode(userName));
                    tr.appendChild(th);
                });
            }

            Object.keys(setOrGet).forEach(function(controlName) {

                var parameters = Object.keys(setOrGet[controlName]);

                parameters.forEach(function(parameterName) {
                    // Make a row for this parameter
                    //
                    var access = setOrGet[controlName][parameterName];
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
                    } else {
                        // type === 'set'
                        td = document.createElement('td');
                        td.className = 'setValue';
                        let input = document.createElement('input');
                        input.type = 'text';
                        input.onchange = function() {
                            console.log('io.Emit(' + ['setParameter',
                                programName, controlName,
                                parameterName, parseFloat(input.value)] + ')');

                            io.Emit('setParameter', programName, controlName,
                                parameterName, parseFloat(input.value));
                        };
                        let button = document.createElement('button');
                        button.type = 'button';
                        button.appendChild(document.createTextNode('submit'));
                        button.onclick = function() {
                            input.onchange();
                        };
                        td.appendChild(button);
                        td.appendChild(input);
                        tr.appendChild(td);
                    }

                    users.forEach(function(userName) {
                        td = document.createElement('td');
                        td.className = type;
                        var boolValue = access[userName];
                        access[userName] = {
                            checkbox:
                                checkbox(userName, controlName,
                                    parameterName, boolValue),
                            permission: boolValue
                        };
                        td.appendChild(access[userName].checkbox);
                        tr.appendChild(td);
                    });
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
            h3.appendChild(document.createTextNode('Controller '));
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

            makeActionTable("set", set, controllerDiv, programName);
            makeActionTable("get", get, controllerDiv, programName);
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
            //    "args=" + [].slice.call(arguments));

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
            'Administrator Control Panel'));
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

    io.Emit('addUsers');

    io.On('addUsers', function(users, urls) {

        /************* Make "Contestant User URLs" panel ***********/
        var div = document.createElement('div');
        div.className = 'userLinks';
        contestPanel.appendChild(div);
        // Make this a show/hide clickable thing.
        makeShowHide(div, { header: "Contestant User URLs" });

        var userNames = [];
        Object.keys(users).forEach(function(userName) {
            if(userName === 'admin') return;
            userNames.push(userName);
        });

        let p = document.createElement('p');
        p.className = 'userLinks';
        p.appendChild(document.createTextNode(
            'Contest participants  may login and access the ' +
            'contest with the following URLs:'));
        div.appendChild(p);

        urls.forEach(function(url) {

            var h3 = document.createElement('h3');
            h3.className = 'userLinks';
            h3.appendChild(document.createTextNode(url));
            div.appendChild(h3);

            var table = document.createElement('table');
            table.className = 'userLinks';
            div.appendChild(table);

            userNames.forEach(function(userName) {

                let tr = document.createElement('tr');
                tr.className = 'userLinks';
                table.appendChild(tr);
                let td = document.createElement('td');
                td.className = 'userLinks';
                let a = document.createElement('a');
                a.className = 'userLinks';
                let link = a.href = url + '/?user=' + userName +
                    '&password=' + users[userName].password;
                a.appendChild(document.createTextNode(userName));
                td.appendChild(a);
                tr.appendChild(td);


                if(url.match(/^https\:/) == null)
                    return;

                // this is a https server url.
                // so add email to user option.

                td = document.createElement('td');
                td.className = 'userLinks';
                a = document.createElement('a');
                a.href='mailto:' + userName +
                    '?to&subject=CRTS%20Participant%20Link&body=Hello%20' +
                    userName + '%2C%0A%0AYour%20user%20CRTS%20Participant%20Link%20' +
                    'is%3A%20' + link.replace(/\&/,'%26') + '%0A%0A';
                a.appendChild(document.createTextNode(
                    'Email participant link to ' + userName));
                td.appendChild(a);
                tr.appendChild(td);
            });
        });

        /******************* Make three more panels ************/
        // We add panels in this order.
        //
        _addLauncherPanel(io, contestPanel);
        _addRunningProgramsPanel(io, contestPanel);
        _addControllerPanels(io, contestPanel, userNames);

        // This could not be hidden until all the child widgets where
        // added.  Now that we are done we can hide it.
        showHide.hide();
    });
}
