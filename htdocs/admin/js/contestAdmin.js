
function _addControllerPanel(io, contestPanel) {

    var controllers = {};
    var users = false;


    io.On('addUsers', function(userNames) {

        assert(users === false, "We only add the users once.");
        users = userNames;
    });


    function getParameter(programName, controlName, parameter, id) {

        console.log('getParameter ' + parameter + " id=" + id);

        _contest.io.Emit('getParameter', programName, controlName,
            parameter, id);
    }

    function userCheckboxOnChange(userName, programName, type,
        controlName, parameter, input) {

        console.log(parameter + " checkbox changed:  value= " + input.checked);

        _contest.io.Emit('changePermission', userName, programName, type,
                controlName, parameter, input.checked);
    }

    function makeId(elementType, programName, controlName, parameter) {

        var ret = "";
        for(var i=0; i < arguments.length; ++i) {
            ret += arguments[i];
            ret += '_';
        }
        return ret;
    }

    // This will keep adding controller panels to the ContestPanel as they
    // come up.

    // makeActionTable() makes a set or get table for a controller.
    //
    // type is "set" or "get"
    // obj is set or get
    function makeActionTable(type, obj, parentNode, programName) {

        function checkbox(userName, controlName, parameter) {
            return '<input type=checkbox onchange="userCheckboxOnChange(\'' + 
                userName + "','" +
                programName + "','" +
                type + "','" +
                controlName + "','" +
                parameter + "',this);\"" +
                "></input>";
        }

        var div = document.createElement('div');
        div.className = type;
        var div_innerHTML = "<h3 class=" + type + ">" +
            type + "</h3>" +
            "<table class=" + type + ">" + 
            "<tr><td></td><th class=" + type + " colspan=" +
            users.length; +
            ">Users</th>" +
            ((type==="get")?"<th></th>":"") +
            "</tr>" +
            "<tr class=" +
            type + "><th class=" + type +
            ">parameter</th>";

        users.forEach(function(userName) {
            div_innerHTML += '<th class=' + type + '>' +
                userName + '</th>';
        });
        if(type==="get") div_innerHTML += "<th>value</th>";
        div_innerHTML += '</tr>';

        Object.keys(obj).forEach(function(controlName) {

            var parameters = obj[controlName];

            parameters.forEach(function(parameter) {
                // Make a row for this parameter
                div_innerHTML += "<tr class=" + type +
                    "><td class=" + type + ">" + controlName +
                    ":" + parameter + '</th>';
                users.forEach(function(userName) {
                    div_innerHTML += '<td class=' + type +
                        '>' + checkbox(userName, controlName, parameter) +
                        '</td>';
                });
                if(type==="get") {
                    let id = makeId('getTD', programName,
                        controlName, parameter);
                    div_innerHTML +=
                    '<td class=getvalue id=' + id +
                    ' onclick="getParameter(\'' +
                        programName + "','" +
                        controlName + "','" +
                        parameter + "','" +
                        id + "');\"" +
                        '>click</td>';
                }
                div_innerHTML += '</tr>';
            });
        });

        div_innerHTML += '</table>';
        div.innerHTML = div_innerHTML;
        parentNode.appendChild(div);
    }

    function appendContestTable(controller, programName,
        set, get, image) {

        // Add a new controller <div>
        var controllerDiv = controller.div = document.createElement('div');
        controllerDiv.className = "controller";
        controllerDiv.innerHTML = "<h3 class=controller>Controller " +
            "<span class=controller>" + programName + "</span></h3>";
        contestPanel.appendChild(controllerDiv);

        var img = document.createElement('img');
        console.log('adding image: ' + image);
        img.src = image;

        img.className = 'controller';
        controllerDiv.appendChild(img);

        makeActionTable("set", set, controllerDiv, programName);
        makeActionTable("get", get, controllerDiv, programName);
    }


    io.On('addController', function(programName, set, get, image) {

        assert(users !== false, "We have not gotten 'addUsers' yet");

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

        appendContestTable(controller, programName, set, get, image);
    });


    io.On('getParameter', function(value, programName, controlName,
        parameter, id) {

        var node = document.getElementById(id);
        if(node) {
            node.innerHTML = value.toString();
        }
    });


    io.On('removeController', function(programName, set, get) {

        console.log('Got On("removeController",) program="' +
            programName);

        // TODO: HERE more code.
        //
        //


        delete _contest.controllers[programName];
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
        let span = document.createElement('span');
        span.className = 'program_signal';
        span.appendChild(document.createTextNode('signal'));
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


    io.On('runningPrograms', function(runningPrograms) {

        var keys = Object.keys(runningPrograms);
        keys.forEach(function(pid) {
            var rp = runningPrograms[pid];
            addProgram(rp.path, rp.pid, rp.args);
        });
    });

}


function contestAdminInit(io) {

    var contestPanel = document.createElement('div');
    contestPanel.className = "contestPanel";

    var h = document.createElement('h3');
    h.appendChild(document.createTextNode('CRTS ' +
            'Contest Access Control Panel'));
    h.className = 'contestPanel';
    contestPanel.appendChild(h);

    getElementById('bottom').appendChild(contestPanel);
    makeShowHide(contestPanel, {header:  h});

    console.log('created contest panel');

    _addLauncherPanel(io, contestPanel);
    _addRunningProgramsPanel(io, contestPanel);
    _addControllerPanel(io, contestPanel);
}

