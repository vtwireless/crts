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


    var contestPanel = document.createElement('div');
    contestPanel.className = "userPanel";

    var h = document.createElement('h3');
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

    io.Emit('getLauncherPrograms');


    ///////////////////////////////////////////////////////////////////
    //   Add panels
    ///////////////////////////////////////////////////////////////////

    AddLauncherPanel();



    console.log("Made user panel");
}


