
// Local file object.
//
var _contest = { controllers: {} };

function _makeId(elementType, programName, controlName, parameter) {

    var ret = "";
    for(var i=0; i < arguments.length; ++i) {
        ret += arguments[i];
        ret += '_';
    }
    return ret;
}

function _checkboxOnChange(userName, programName, type,
    controlName, parameter, input) {

    console.log(parameter + " checkbox changed:  value= " + input.checked);

    _contest.io.Emit('changePermission', userName, programName, type,
            controlName, parameter, input.checked);
}

function _getParameter(programName, controlName, parameter, id) {

    console.log('getParameter ' + parameter + " id=" + id);

    _contest.io.Emit('getParameter', programName, controlName,
        parameter, id);
}




function _appendContestTable(controller, programName,
    set, get, image) {

    // type is "set" or "get"
    // obj is set or get
    function makeActionTable(type, obj, parentNode) {

        function checkbox(userName, controlName, parameter) {
            return '<input type=checkbox onchange="_checkboxOnChange(\'' + 
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
                    ' onclick="_getParameter(\'' +
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

    if(_contest.panel === undefined) {
        var panelDiv = _contest.panel = document.createElement('div');
        panelDiv.className = "contestPanel";
        panelDiv.innerHTML = "<h3 class=contestPanel>CRTS " +
            "Contest Access Control Panel</h3>";

        getElementById('bottom').appendChild(panelDiv);

        console.log('appended contest panel');
    } else {
        var panelDiv = _contest.panel;
    }

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



function contestAdminInit(io) {

    console.log('contestAdminInit()');

    assert(_contest.io === undefined,
        "You cannot load/call this function more than once.");
    _contest.io = io;

    io.Emit('getLauncherPrograms');


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

