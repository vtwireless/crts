require('/session.js');
require('/showHide.js');
require('/tests/genericUserPanel.css');


function GenricUserPanel(io, parentElement=null) {


    function AddLauncherPanel(programs) {

        var div = document.createElement('div');
        div.className = 'launcher';

        userPanel.appendChild(div);
        // Make this a show/hide clickable thing.
        makeShowHide(div, { header: 'Launch' });

        console.log('launcher programs=' + JSON.stringify(programs));

        var paths = Object.keys(programs);
        if(paths.length === 0)
            return;
    }


    var userPanel = document.createElement('div');
    userPanel.className = "userPanel";

    var h = document.createElement('h3');
    h.appendChild(document.createTextNode('CRTS ' +
            'Generic User Test Panel'));
    h.className = 'userPanel';
    userPanel.appendChild(h);

    if(typeof(parentElement) !== 'string' && parentElement !== null)
        parentElement.appendChild(userPanel);
    else if(typeof(parentElement) === 'string')
        parentElement = document.getElementById(parentElement);
    else
        parentElement = document.body;

    parentElement.appendChild(userPanel);

    // We user could add this  userPanel <div> as a child to whatever
    // they like.
    this.getElement = function() { return userPanel; };

    makeShowHide(userPanel, {header: h, startShow: true});


    io.Emit('getLauncherPrograms');

    io.On('receiveLauncherPrograms', function(programs, dummy) {

        AddLauncherPanel(programs);
    });



    console.log("Made user panel");
}


