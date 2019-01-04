
// Local file object.
//
var _contest = { controllers: {} };


function _getContestPanel(){

    if(_contest.panel === undefined) {

        var panelDiv = _contest.panel = document.createElement('div');
        panelDiv.className = "contestPanel";

        var h = document.createElement('h3');
        h.appendChild(document.createTextNode('CRTS ' +
                'Contest Access Control Panel'));
        h.className = 'contestPanel';
        panelDiv.appendChild(h);

        getElementById('bottom').appendChild(panelDiv);

        console.log('created contest panel');

        makeShowHide(panelDiv, {header:  h});

    } else {
        var panelDiv = _contest.panel;
    }

    return panelDiv;
}



function _appendContestTable(controller, programName,
    set, get, image) {

    function getParameter(programName, controlName, parameter, id) {

        console.log('getParameter ' + parameter + " id=" + id);

        _contest.io.Emit('getParameter', programName, controlName,
            parameter, id);
    }

    function checkboxOnChange(userName, programName, type,
        controlName, parameter, input) {

        console.log(parameter + " checkbox changed:  value= " + input.checked);

        _contest.io.Emit('changePermission', userName, programName, type,
                controlName, parameter, input.checked);
    }

    function _makeId(elementType, programName, controlName, parameter) {

        var ret = "";
        for(var i=0; i < arguments.length; ++i) {
            ret += arguments[i];
            ret += '_';
        }
        return ret;
    }


    // type is "set" or "get"
    // obj is set or get
    function makeActionTable(type, obj, parentNode) {

        function checkbox(userName, controlName, parameter) {
            return '<input type=checkbox onchange="checkboxOnChange(\'' + 
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
            _contest.users.length.toString() +
            ">Users</th>" + 
            ((type==="get")?"<th></th>":"") +
            "</tr>" +
            "<tr class=" +
            type + "><th class=" + type +
            ">parameter</th>";

        _contest.users.forEach(function(userName) {
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
                _contest.users.forEach(function(userName) {
                    div_innerHTML += '<td class=' + type +
                        '>' + checkbox(userName, controlName, parameter) +
                        '</td>';
                });
                if(type==="get") {
                    let id = _makeId('getTD', programName,
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

    var panelDiv = _getContestPanel();

    // Add the controller <div>
    var controllerDiv = controller.div = document.createElement('div');
    controllerDiv.className = "controller";
    controllerDiv.innerHTML = "<h3 class=controller>Controller " +
        "<span class=controller>" + programName + "</span></h3>";
    panelDiv.appendChild(controllerDiv);

    var img = document.createElement('img');
    console.log('adding image: ' + image);
    img.src = image;

    img.className = 'controller';
    controllerDiv.appendChild(img);

    // TODO: add the table required attributes.  I forget what it was
    // called.  Like description of something.  Tidy complains about it
    // not being there.

    makeActionTable("set", set, controllerDiv);
    makeActionTable("get", get, controllerDiv);
}



//  _addLauncherPanel() makes HTML that is a clickable list of programs
//  that we can launch on the server.
//
function _addLauncherPanel(io) {


    var programs = null;

    function displayLauncher(launcher) {

        // TODO: this text node could be made into
        // a table row.
        //
        launcher.runCountText.data = launcher.runCount.toString();

        launcher.numRunningText.data = launcher.numRunning.toString();
    }

    function createPrograms() {
        var contestPanel = _getContestPanel();

        if(programs) {
            // remove the old list
            assert(programs.div, "");
            while(programs.div.firstChild)
                programs.div.removeChild(launcher.firstChild);
            delete programs.div; // Is this delete needed??
            delete programs;
        }

        programs = {};
        var div = programs.div = document.createElement('div');

        div.className = 'launcher';
        return div;
    }


    io.Emit('getLauncherPrograms');

    io.On('receiveLauncherPrograms', function(programs_in) {

        var programNames = Object.keys(programs_in);

        if(programNames.length < 1) {
            console.log('got NO receiveLauncherPrograms');
            return;
        }

        var contestPanel = _getContestPanel();

        // TODO: This html could be prettied up a lot.
        //

        if(programs) {
            // remove the old list
            assert(programs.div, "");
            while(programs.div.firstChild)
                programs.div.removeChild(launcher.firstChild);
            delete programs.div; // Is this delete needed??
            delete programs;
        }

        var div = createPrograms();

        div.className = 'launcher';

        let table = document.createElement('table');
        table.className = 'launcher';

        let tr = document.createElement('tr');
        tr.className = 'launcher';


        let th = document.createElement('th');
        th.className = 'launcher';
        th.appendChild(document.createTextNode('program'));
        tr.appendChild(th);

        th = document.createElement('th');
        th.className = 'launcher';
        th.appendChild(document.createTextNode('run with arguments'));
        tr.appendChild(th);

        th = document.createElement('th');
        th.className = 'launcher';
        th.appendChild(document.createTextNode('number running'));
        tr.appendChild(th);

        th = document.createElement('th');
        th.className = 'launcher';
        th.appendChild(document.createTextNode('run count'));
        tr.appendChild(th);

        table.appendChild(tr);


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


        div.appendChild(table);
        contestPanel.appendChild(div);
        // Make this a show/hide clickable thing.
        makeShowHide(div, { header: 'launch' });
    });

    io.On('updateRunState', function(path, program) {

        if(undefined === programs[path]) return;
        var launcher = programs[path];
        Object.keys(program).forEach(function(key) {
            launcher[key] = program[key];
        });

        displayLauncher(launcher);
    });
}


// Make a panel for all the currently running programs
//
function _addRunningProgramsPannel(io) {

    var programs = null;

    function getRunningProgramsPanel() {

        if(programs) return programs.div;

        programs = {};
        var contestPanel = _getContestPanel();
        programs.div = document.createElement('div');
        programs.div.className = 'programs';
        var header = document.createElement('h3');
        header.appendChild(document.createTextNode('Running Programs'));
        programs.div.appendChild(header);
        contestPanel.appendChild(programs.div);
        makeShowHide(programs.div, { header: header });
        return programs.div;
    }


    var div = getRunningProgramsPanel();

    io.On('updateRunState', function(path, program_in, pid, isRunning) {

        if(programs[pid] === undefined && isRunning) {

            var program = programs[pid] = program_in;
        }
    });


}


function contestAdminInit(io) {

    console.log('contestAdminInit()');

    assert(_contest.io === undefined,
        "You cannot load/call this function more than once.");

    _contest.io = io;

    _addLauncherPanel(io);
    _addRunningProgramsPannel(io);

    io.On('addUsers', function(users) {

        if(_contest.users === undefined)
            _contest.users = [];
        _contest.users = _contest.users.concat(users);

        console.log('added users=' + users);
    });

    io.On('addController', function(programName, set, get, image) {

        console.log('Got On("addController",) program="' +
            programName + '"' +
            '\n    set=' + JSON.stringify(set) +
            '\n    get=' + JSON.stringify(get) +
            '\n  image=' + image);

        var controller = _contest.controllers[programName] = {
            programName: programName,
            set: set,
            get: get
        };

        _appendContestTable(controller, programName, set, get, image);

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

