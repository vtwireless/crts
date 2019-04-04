// Developer friendly utility functions fail() and assert() are not so
// user friendly.
//
function fail() {

    // TODO: add stack trace or is browser debugger enough?
    var text = "Something has gone wrong:\n";
    for(var i=0; i < arguments.length; ++i)
        text += "\n" + arguments[i];
    line = '\n--------------------------------------------------------';
    text += line + '\nCALL STACK' + line + '\n' +
        new Error().stack + line;
    console.log(text);
    alert(text);
    stop();
    throw text;
}

function assert(val, msg=null) {

    if(!val) {
        if(msg)
            fail(msg);
        else
            fail("JavaScript failed");
    }
}

function getElementById(id) {

    var element = document.getElementById(id);
    if(!element) fail("document.getElementById(" + id + ") failed");
    return element;
}


// The header must be in the container if it is not a string.  This will
// slide in a container tag element to hold the show/hide bottom.
//
function makeShowHide(container,
        opts = { header: null, startShow: true}) {

    var header=null, showing=true;

    if(opts.header !== undefined) {
        if(typeof(opts.header) === 'string') {
            header = document.createElement('h2').
                appendChild(document.createTextNode(opts.header));
            container.insertBefore(header, container.firstChild);
        } else
            header = opts.header;
    }

    if(typeof(opts.startShow) === 'boolean')
        showing = opts.startShow;

    // Create the show/hide tag as the first child
    var tab = document.createElement('div');
    var span = document.createElement('span');
    var tabText = document.createTextNode('');
    span.appendChild(tabText);
    tab.appendChild(span);
    container.insertBefore(tab, container.firstChild);

    function showChildren() {

        for(var child = container.firstChild; child;
                child = child.nextSibling) {
            if(child === header || child === tab) continue;
            child.style.display = 'block'; // use space on page
            child.style.visibility = 'visible';
        }
        tab.className = 'hide_tab';
    }

    function hideChildren() {

        for(var child = container.firstChild; child;
                child = child.nextSibling) {
            if(child === header || child === tab) continue;
            child.style.display = 'none'; // use no space on page
            child.style.visibility = 'hidden';
        }
        tab.className = 'show_tab';
    }


    tab.onclick = function() {
        if(showing) {
            // HIDE IT
            tabText.data = 'show';
            tab.className = 'show_tab';
            tab.title = 'show content';
            span.className = 'show_tab';
            hideChildren();
            // flip flag
            showing = false;
        } else {
            // SHOW IT
            tabText.data = 'hide';
            tab.className = 'hide_tab';
            tab.title = 'hide content';
            span.className = 'hide_tab';
            showChildren();
            // flip flag
            showing = true;
        }
    };

    if(showing === false)
        tab.onclick();
    else {
        showing = false;
        tab.onclick();
    }
}
