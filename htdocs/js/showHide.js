require('/css/showHide.css');

// The header must be in the container if it is not a string.  This will
// slide in a container tag element to hold the show/hide bottom.
//
function makeShowHide(container,
        opts = { header: null}) {

    var header=null;

    if(opts.header !== undefined) {
        if(typeof(opts.header) === 'string') {
            header = document.createElement('h2').
                appendChild(document.createTextNode(opts.header));
            container.insertBefore(header, container.firstChild);
        } else
            header = opts.header;
    }

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

        var count=0;
        for(var child = container.firstChild; child;
                child = child.nextSibling) {
            if(child === header || child === tab) continue;
            child.style.display = 'none'; // use no space on page
            child.style.visibility = 'hidden';
        }
        tab.className = 'show_tab';
    }

    tabText.data = 'hide';
    tab.className = 'hide_tab';
    tab.title = 'hide content';
    span.className = 'hide_tab';
    showChildren();

    var showing = true;

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

    return {
        // The user may call these show/hide functions after adding all
        // the children that they would like:
        //
        hide: function() { if(showing) tab.onclick(); },
        show: function() { if(!showing) tab.onclick(); }
    };
}
