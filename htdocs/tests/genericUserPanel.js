require('/session.js');
require('/showHide.js');
require('/tests/genricUserPanel.css');


function GenricUserPanel(io, parentElement=null) {

    var userPanel = document.createElement('div');
    userPanel.className = "userPanel";

    var h = document.createElement('h3');
    h.appendChild(document.createTextNode('CRTS ' +
            'Generic User Panel'));
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


    console.log("Made user panel");
}


