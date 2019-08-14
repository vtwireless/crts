// There are many other web desktops.  We wanted one that did not require
// a server; so it's just client side javaScript and CSS; and you can use
// it from a single small HTML file.  Not a full-featured desktop.  Just
// a very simple application "window" containers in a browser.
//
//
// https://en.wikipedia.org/wiki/Web_desktop
// https://en.wikipedia.org/wiki/Desktop_metaphor
//
//
// It likely that HTML5 and javaScript will add stuff to make some of this
// code obsolete in the near future.  The element resize and drag events
// are being added to the standards.


//
// WebDesktop App = WDApp
//
//
//  <div> elements put in each other like so:
//
//
//      -------------------------win--("window")-----------------
//      |                                                       |
//      | ---------header-------------------------------------- |
//      | |  --- ---------- titleSpan ----  ----- ----- ----- | |
//      | |  |@| |                       |  | - | | M | | X | | |
//      | |  --- -------------------------  ----- ----- ----- | |
//      | ----------------------------------------------------- |
//      |                                                       |
//      | ------------main------------------------------------- |
//      | |                                                   | |
//      | | --------------app-------------------------------- | |
//      | | |                                               | | |
//      | | |                                               | | |
//      | | |                                               | | |
//      | | |                                               | | |
//      | | |            User app content  ....             | | |
//      | | |                                               | | |
//      | | |                                               | | |
//      | | |                                               | | |
//      | | |                                               | | |
//      | | |                                               | | |
//      | | |                                               | | |
//      | | ------------------------------------------------- | |
//      | |                                                   | |
//      | | --------------?footer?--------------------------- | |
//      | | |                                               | | |
//      | | ------------------------------------------------- | |
//      | ----------------------------------------------------- |
//      ---------------------------------------------------------
//

//
// Draggable, Resizable, RollUpable, and Iconifible window app made with
// <div>
//
// This is the only interface provided, and one interface is a good thing.
//
// example:
//
//  var app = new WDApp('my app', myCanvas);
//
//
//  WDApp(headerText, app, onclose = null, opts = null) 
//
//  WDApp(headerText, app, opts = null)
//
//
//  opts = {
//
//          close: bool // false to not let it close the app with X
//          
//  }
//
//
//
function WDApp(headerText, app, onclose = null, opts = null) {

    if(typeof(onclose) === 'object') {
        opts = onclose;
        onclose = null;
    }
    if(!opts)
        opts = {};

    function WDRoot() {

        // This is a singleton.
        if(WDApp.root) return;

        WDApp.root = this;
        var rootWin = document.createElement('div');
        rootWin.className = 'WDRoot';
        document.body.appendChild(rootWin);

        var eventCatcher = document.createElement('div');
        eventCatcher.className = 'eventCatcher';
        rootWin.appendChild(eventCatcher);

        // list of events being catch by the eventCatcher <div>.
        var events = [];

        this.showCatcher = function(cursor, types, func) {
            // This sets up a net <div> that fills the whole root window
            // and catches events for the caller of this function.  This "net"
            // <div> is translucent or completely transparent.
            //eventCatcher.style.cssText += 'cursor: ' + cursor + ';';
            eventCatcher.style.cursor = cursor;
            eventCatcher.style.visibility =  'visible';
            eventCatcher.style.display = 'block';
            setTimeout(function() {
                eventCatcher.style.cursor = cursor;
                console.log('  cursor="' + cursor + '"');
            }, 4000);
            eventCatcher.style.cursor = cursor;
            console.log('cursor="' + cursor + '"');

            // remove the old handlers.
            events.forEach(function(obj) {
                document.removeEventListener(obj.type, obj.func);
            });
            events = [];

            if(typeof(types) === 'Array') {
                types.forEach(function(type) {
                    document.addEventListener(type, func);
                    events.push({type: func});
                });

            } else {
                document.addEventListener(types, func);
                events.push({type: types, func: func});
            }

            eventCatcher.style.cursor = cursor;
        };

        this.hideCatcher = function(cursor=null) {
            // Take down the event catcher net.
            events.forEach(function(obj) {
                document.removeEventListener(obj.type, obj.func);
            });
            events = [];
            eventCatcher.style.display = 'none';
            //eventCatcher.style.cursor = cursor?cursor:"default";
        };


        var topWin = document.createElement('div');
        topWin.className = 'WDTopWin';
        rootWin.appendChild(topWin);

        var panel = document.createElement('div');
        panel.className = 'WDPanel';
        rootWin.appendChild(panel);

        this.addToPanel = function(win, description,
            popFunc, isOnTop, imgSrc=null) {

            var showing = true;
            var icon = document.createElement('div');
            icon.className = 'WDPanelIcon';
            icon.title = 'toggle view: ' + description;
            icon.tabIndex = '0';
            var img = document.createElement('img');
            img.className = 'WDPanelIcon';
            img.src = imgSrc?imgSrc: WDApp.urlPrefix + 'defaultAppIcon.png';
            icon.appendChild(img);
            var title = document.createElement('span');
            title.className = 'WDPanelIcon';
            title.appendChild(document.createTextNode(description));
            icon.appendChild(title);
            panel.appendChild(icon);

            var retObj = {};

            retObj.hide = function() {
                win.style.display = 'none';
                win.style.visibility = 'invisible';
                showing = false;
            };

            retObj.show = function() {
                win.style.display = 'inline-block';
                win.style.visibility = 'visible';
                showing = true;
                win.focus();
                popFunc();
            };

            icon.onclick = function() {
                if(showing && isOnTop()) retObj.hide();
                else retObj.show();
            };

            retObj.removeFromPanel = function() {
                panel.removeChild(icon);
            };

            return retObj;
        };


        this.getTopWin = function() {
            return topWin;
        };


        // https://developer.mozilla.org/en-US/docs/Web/API/Fullscreen_API#Watching_for_the_Enter_key

        function findFocusedApp() {

            var el = document.activeElement;
            // see if this el is a child of a win app thingy
            for(; el; el = el.parentElement)
                if(el.WDApp != null)
                    return el.WDApp;
            return document.documentElement;
        }

        var requestFullscreen = null;
    
        [
            'requestFullscreen',
            'webkitRequestFullscreen',
            'mozRequestFullScreen',
            'msRequestFullscreen'
        ].forEach(function(rfs) {
            if(!requestFullscreen && topWin[rfs])
                requestFullscreen = rfs;
        });

        function fullscreenElement() {
            return (
                document.fullscreenElement ||
                document.webkitFullscreenElement ||
                document.mozFullScreenElement ||
                document.msFullscreenElement || null);
        }

        function exitFullscreen() {
            if(document.exitFullscreen)
	        document.exitFullscreen();
	    else if(document.mozCancelFullScreen)
	        document.mozCancelFullScreen();
	    else if(document.webkitExitFullscreen)
	        document.webkitExitFullscreen();
	    else if(document.msExitFullscreen)
	        document.msExitFullscreen();
        }

        function toggleFullScreen() {

            // Returns true if this takes action.

            let focusEl = findFocusedApp();
            let fullEl = fullscreenElement()

            //console.log('focused element=' + focusEl);

            if(fullEl) {
                if(focusEl !== fullEl) {
                    focusEl[requestFullscreen]();
                    return;
                }
                exitFullscreen();
                return;
            }
            // else no fullscreen
            focusEl[requestFullscreen]();
        }

        // Key bindings for this window manager thingy:
        document.addEventListener("keypress", function(e) {

            // TODO: Figure out other keys and mode keys like alt and control.
            //console.log("e.key=" + e.key);
            //
            // firefox 65.0 does not let this code get key 'F11'
            // it just goes fullscreen with the whole body.
            //
            //console.log("e.key=" + e.key);

            if(e.key === 'F11' || e.key === '+') {
                if(!e.repeat)
                    toggleFullScreen();
                e.preventDefault();
            }
        });

    } // WDRoot()


    if(!WDApp.root) new WDRoot;


    if(opts.appIcon === undefined)
        opts.appIcon = WDApp.urlPrefix + 'defaultAppIcon.png';


    ////////////////////////////////STATE//////////////////////////////////
    //
    // Possible state:
    //
    // Enumeration of the 3 possible "window" display size states as we
    // define them:
    const NORMAL_SIZE = 0, MAXIMIZED = 1, FULL_SCREEN = 2;
    // 
    var isHidden = false;   // in the docker
    var isRolledUp = false; // just header showing
    //
    // So ya, we have 3x2x2=12 possible states, but we disallow the 1
    // state: full screen with rolled up, leaving us 11 states.
    //
    //
    // Initial display state:
    var displayState = NORMAL_SIZE;
    //
    //
    // So there are 3 state variables: displayState, isHidden, isRolledUp,
    // and to save the window position and size when the "window" is not
    // in the "normal state":
    var normalX, normalY, normalWidth, normalHeight;
    //
    ///////////////////////////////////////////////////////////////////////


    var win = document.createElement('div');
    win.className = 'WDWin';
    win.WDApp = app;
    win.tabIndex = '0';

    var header = document.createElement('div');
    header.className = 'WDHeader';
    win.appendChild(header);

    var appIcon =  document.createElement('img');
    appIcon.className = 'WDAppIcon';
    appIcon.src = opts.appIcon;
    appIcon.setAttribute("tabIndex", 0);
    header.appendChild(appIcon);

    var titleSpan = document.createElement('span');
    titleSpan.className = 'WDTitleSpan';
    titleSpan.appendChild(document.createTextNode(headerText));
    header.appendChild(titleSpan);

    var html = document.documentElement;


    // Make the close button:
    if(opts.close === undefined || opts.close) {
        var xIcon = document.createElement('img');
        xIcon.className = 'WDXIcon';
        xIcon.src = WDApp.urlPrefix + 'x.png';
        xIcon.title = 'close';
        xIcon.setAttribute("tabIndex", 0);
        header.appendChild(xIcon);
    } else
        var xIcon = null;

    // Make the maximize button:
    var maxIcon = document.createElement('img');
    maxIcon.className = 'WDMaxIcon';
    maxIcon.src = WDApp.urlPrefix + 'max.png';
    maxIcon.title = 'maximize';
    maxIcon.setAttribute("tabIndex", 0);
    header.appendChild(maxIcon);

    // Make the minify button:
    var minIcon = document.createElement('img');
    minIcon.className = 'WDMinIcon';
    minIcon.src = WDApp.urlPrefix + 'min.png';
    minIcon.title = 'minify';
    minIcon.setAttribute("tabIndex", 0);
    header.appendChild(minIcon);

    //header.setAttribute("tabIndex", 0);

    var main = document.createElement('div');
    main.appendChild(app);
    main.className = 'WDMain';
    win.appendChild(main);

    var topWin = WDApp.root.getTopWin();


    function desktopWidth() {
        return topWin.offsetWidth;
    }

    function desktopHeight() {
        return topWin.offsetHeight;
    }

    const startingHeaderCursor = 'grab';
    header.style.cursor = startingHeaderCursor;

    topWin.appendChild(win);

    // We center this window thingy to start.
    //
    main.style.width = app.offsetWidth + 'px';
    main.style.height = app.offsetHeight + 'px';

    // ref:
    // https://developer.mozilla.org/en-US/docs/Web/API/HTMLElement/offsetHeight
    // NOTE: offsetWidth and offsetHeight include padding and border.
    win.style.width = app.offsetWidth + 'px';
    win.style.height = (app.offsetHeight + header.offsetHeight) + 'px';

    win.style.left = (desktopWidth() - win.clientWidth)/2 + 'px';
    win.style.top = (desktopHeight() - win.clientHeight)/2 + 'px';

    // Element offsetWidth and offsetHeight do not include the padding.

    let pdstyle = getComputedStyle(win);
    var winPaddingLeft = parseInt(pdstyle.getPropertyValue('padding-left'));
    var winPaddingTop = parseInt(pdstyle.getPropertyValue('padding-top'));
    var winPaddingWidth = winPaddingLeft +
            parseInt(pdstyle.getPropertyValue('padding-right'));
    var winPaddingHeight = winPaddingTop +
            parseInt(pdstyle.getPropertyValue('padding-bottom'));

    // Keep the app from inheriting the cursor from win
    main.style.cursor = 'default';

    var x0, y0, left0, top0;
    var marginX, marginY, startX, startY;
    var sx0, sy0;

    // initialize what is the normal size:
    normalX = win.offsetLeft;
    normalY = win.offsetTop;
    // The style width and height do not include padding,
    // but offsetWidth and offsetHeight do include padding.
    normalWidth = win.offsetWidth - winPaddingWidth;
    normalHeight = win.offsetHeight - winPaddingHeight;


    this.win = win;
    this.zIndex = ++(WDApp.zIndexMax);

    WDApp.stackingOrder[this.zIndex] = this;
    win.style.zIndex = this.zIndex;

    // We need access to this object in some of the functions here in.
    var This = this;
    win.focus();

    const minWidth = 110, minHeight = 70;

    ///////////////////////////////////////////////////////////////////////
    //  At this point all the elements are built, except panelIcon.
    ///////////////////////////////////////////////////////////////////////

    function pushBackOthers() {
        // Push back all the other "windows" that are in front of this one.
        for(var i=This.zIndex+1; i<=WDApp.zIndexMax; ++i) {
            var app = WDApp.stackingOrder[i];
            app.win.style.zIndex = i-1;
            app.zIndex = i-1;
            WDApp.stackingOrder[i-1] = app;
        }
    }

    function popForward() {

        // Change the "window" stacking order.

        if(This.zIndex === WDApp.zIndexMax)
            // This is the top "window" already.
            return;
        pushBackOthers();
        This.zIndex = WDApp.zIndexMax;
        WDApp.stackingOrder[This.zIndex] = This;
        // Put this "window" on top in the at the zIndexMax
        win.style.zIndex = This.zIndex;
        win.focus();
    }

    var panelIcon = WDApp.root.addToPanel(win, headerText, popForward,
        function() { /*is on top?*/return This.zIndex === WDApp.zIndexMax;},
        opts.appIcon);


    appIcon.onclick = function() {

        // Open a new same origin window that has access to the current
        // javaScript scope (context).  This is what google chat does to
        // put the chat in a window from the "parent browser tab" email
        // window.
        let newWindow = open('about:blank', headerText,
            'scrollbars=no,status=no,' +
            'location=no,toolbar=no,menubar=no,' +
            'width=700,height=400,left=100,top=100');
        newWindow.onload = function() {
            let body = newWindow.document.body;
            body.style.margin = '0px';
            body.appendChild(app);
            app.style.width = '100%';
            app.style.height = '100%';
            panelIcon.hide();
        };

        newWindow.onbeforeunload = function() {
            main.appendChild(app);
            panelIcon.show();
        };
    };

    var closed = false;

    this.close = function() {
        if(closed) return;
        closed = true;
        if(onclose) onclose();
        topWin.removeChild(win);
        // Fix the stacking order of all the "windows":
        pushBackOthers();
        delete WDApp.stackingOrder[WDApp.zIndexMax];
        --WDApp.zIndexMax;
        panelIcon.removeFromPanel();
    }

    if(xIcon)
        xIcon.onclick = this.close;


    header.ondblclick = function() {

        popForward();

        if(isRolledUp) {

            isRolledUp = false;
            main.style.display = 'block';
            minIcon.title === 'minify';
            main.style.visiblity = 'visible';
            if(displayState === NORMAL_SIZE)
                setToNormalSize();
            else if(displayState === MAXIMIZED)
                setToMaximized();
            return;
        }
        // else isRolledUp === false

        win.style.height = header.offsetHeight + 'px';

        isRolledUp = true;
        main.style.display = 'none';
        minIcon.title === 'show';
        main.style.visiblity = 'invisible';
    };

    minIcon.onclick = function() {
        panelIcon.hide();
    };


    function saveNormalSize() {
        normalWidth = win.offsetWidth - winPaddingWidth;
        normalHeight = win.offsetHeight - winPaddingHeight;
        normalX = win.offsetLeft;
        normalY = win.offsetTop;
    }


    function setToNormalSize() {

        // Set this "widow" to a normal size that is not maximized,
        // rolled up, and is showing.  The "resize" will redefine what
        // normal is.

        win.style.left = normalX + 'px';
        win.style.top = normalY + 'px';

        win.style.width = normalWidth + 'px';
        if(isRolledUp)
            win.style.height = header.offsetHeight + 'px';
        else
            win.style.height = normalHeight + 'px';

        main.style.width = normalWidth + 'px';
        main.style.height = (normalHeight - header.offsetHeight) + 'px';

        maxIcon.title = 'maximize';
        displayState = NORMAL_SIZE;
    }


    function setToMaximized() {

        var style = getComputedStyle(win);

        let h = topWin.offsetHeight;
            
        win.style.width = topWin.offsetWidth + 'px'

        if(isRolledUp)
            win.style.height = header.offsetHeight + 'px';
        else
            win.style.height = h + 'px';

        main.style.width = topWin.offsetWidth + 'px';
        main.style.height = (h - header.offsetHeight) + 'px';

        win.style.left = ( -winPaddingLeft) + 'px';
        win.style.top = (-winPaddingTop) + 'px';

        maxIcon.title = 'normal size';
        displayState = MAXIMIZED;
    }

    maxIcon.onclick = function() {
        if(displayState === NORMAL_SIZE)
            setToMaximized();
        else // displayState === MAXIMIZED || 
            //  displayState === FULL_SCREEN
            setToNormalSize();
    };


    // reference:
    // https://www.w3schools.com/howto/howto_js_draggable.asp

    ////////////////////////////////////////////////
    // Make the win element draggable by grabbing
    // part of the header, and also make it resizable.
    ////////////////////////////////////////////////


    // Prevent the "window top bar" icon buttons from being part of the
    // "grab bar" and let the onclick() callbacks that we set work.
    function stop(e) { e.stopPropagation(); }
    appIcon.onmousedown = stop;
    if(xIcon)
        xIcon.onmousedown = stop;
    minIcon.onmousedown = stop;
    maxIcon.onmousedown = stop;

    win.addEventListener('mousedown', function(e) {
        popForward();
    }, true);


    // Enumeration so we do not keep setting the cursor again and
    // again.  Values are X + Y*3 where 
    // X = [0,  1,  2] and Y = [0,  1,  2]
    //     min mid max         min mid max
    //      W       E           N       S
    //
    //  X increasing to the east and
    //  Y is increasing to the south
    //   (1,0)(2,1)(1,2)(0,1) (2,0) (0,0) (2,2) (0,2)
    const N=1, E=5, S=7, W=3, NE=2, NW=0, SE=8, SW=6;
    // We use these constants to select a cursor and
    // designate the mode of resize.
    var cursorIndex = 4;
    var cursors = [
        'nw-resize', 'n-resize', 'ne-resize',
        'w-resize', '4-notUsed', 'e-resize', 
        'sw-resize', 's-resize', 'se-resize'
    ];


    // onmouseover is like hover:
    win.onmouseover = function(e) {

        function checkChangeCursor(e)  {

            // This function just changes the cursor and it's index
            // while the mouse is not pressed.

            if(e.buttons&01/*left mouse down*/||!mouseIsInPadding(e)) return;
            // The left mouse down seems buggy so this:
            if(WDApp.activeTransitionState !== WDApp.STATE_NONE) return;

            /////////////////////////////////////////////////////
            //  We are on the padding edge of the whole "window"
            //  so we change the cursor to a resize cursor.

            const cornerSize = 20;

            let x = ((e.clientX - win.offsetLeft) < cornerSize)?0:
                (((e.clientX - win.offsetLeft) >
                    win.offsetWidth - cornerSize)?2:1);
            let y = ((e.clientY - win.offsetTop) < cornerSize)?0:
                (((e.clientY - win.offsetTop) >
                    win.offsetHeight - cornerSize)?2:1);
            let newCursorIndex = x + y * 3;

            // Don't change the cursor if we don't need to.
            if(cursorIndex != newCursorIndex)
                win.style.cursor = cursors[cursorIndex = newCursorIndex];
        }

        function mouseIsInPadding(e) {

            // We assume that the mouse (pointer) is in the win
            // somewhere.
            if(e.clientX < win.offsetLeft + winPaddingLeft ||
                e.clientX > win.offsetLeft + win.offsetWidth - winPaddingLeft ||
                e.clientY < win.offsetTop + winPaddingTop ||
                e.clientY > win.offsetTop + win.offsetHeight - winPaddingTop)
                // The mouse pointer is in the padding of the win
                // element.
                return true;
            return false;
        }
            
        function doResize(e) {

            // This does not seem to help:
            //e.stopPropagation();
            /////////////////////////// Do the RESIZE //////////////////////////
            //
            // Note: We are resizing x and y independently in these
            // two separate if blocks.
            //
            // Keep in mind:
            // element offsetWidth and offsetHeight include the element padding
            //
            // Find region that the mouse down event occurred one of 4
            // corners or one of the 4 sides.  cursorIndex = x + 3*y
            let xi = cursorIndex % 3; // xi = [ 0, 1, 2 ]
            let yi = (cursorIndex - xi)/3; // yi = [ 0, 1, 2 ]

            // Check for resize in X
            if(xi === 2 /*right side*/) {
                if(e.clientX > win.offsetLeft) {
                    // CASE right side moving.
                    let w = e.clientX - win.offsetLeft - winPaddingWidth;
                    if(w < minWidth) w = minWidth;
                    win.style.width = w + 'px';
                    normalWidth = win.offsetWidth - winPaddingWidth;
                    main.style.width = normalWidth + 'px';
                }
            } else if(xi === 0 /*left side*/) {
                // harder case because the left side is defined as the
                // anchor (left/top origin) side.
                if(e.clientX < win.offsetLeft + win.offsetWidth) {
                    // CASE left side moving.
                    let w = - e.clientX + win.offsetLeft +
                            win.offsetWidth - winPaddingWidth;
                    if(w < minWidth) {
                        w = minWidth;
                        win.style.left = (win.offsetLeft +
                                win.offsetWidth - winPaddingWidth - w) + 'px';
                    } else win.style.left = e.clientX + 'px';
                    win.style.width = w + 'px';
                    normalWidth = win.offsetWidth - winPaddingWidth;
                    main.style.width = normalWidth + 'px';
                }
            }
            // else xi === 1 do not resize in X

            // Check for resize in Y
            if(yi === 2 /*bottom side*/) {
                if(e.clientY > win.offsetTop) {
                    // CASE bottom side moving.
                    let h = e.clientY - win.offsetTop - winPaddingHeight;
                    if(h < minHeight) h = minHeight;
                    win.style.height = h + 'px';
                    normalHeight = win.offsetHeight - winPaddingHeight;
                    main.style.height = (normalHeight - header.offsetHeight) + 'px';
                }
            } else if(yi === 0 /*top side*/) {
                // harder case because the top side is defined as the
                // anchor (left/top origin) side.
                if(e.clientY < win.offsetTop + win.offsetHeight) {
                    // CASE top side moving.
                    let h = - e.clientY + win.offsetTop +
                            win.offsetHeight - winPaddingHeight;
                    if(h < minHeight) {
                        h = minHeight;
                        win.style.top = (win.offsetTop +
                            win.offsetHeight - winPaddingHeight - h) + 'px';
                    } else win.style.top = e.clientY + 'px';
                    win.style.height = h + 'px';
                    normalHeight = win.offsetHeight - winPaddingHeight;
                    main.style.height = (normalHeight - header.offsetHeight) + 'px';
                }
            }
            // else yi === 1 do not resize in Y
            //

            saveNormalSize();
        }


        if(isHidden || isRolledUp ||
            WDApp.activeTransitionState === WDApp.STATE_DRAG
            || displayState !== NORMAL_SIZE) return;

        function mousedown(e) {

            if(WDApp.activeTransitionState !== WDApp.STATE_NONE)
                return;

            if(!(e.buttons & 01))
                // not left button
                return;
            if(!mouseIsInPadding(e)) return;

            // The button is pressed and we will do a resize after mouseup.
            WDApp.activeTransitionState = WDApp.STATE_RESIZE;
            win.onmousemove = null;

            var x = e.clientX, y = e.clientY;


            function mouseup(e) {

                document.removeEventListener('mousemove', doResize);

                WDApp.root.hideCatcher();

                if(mouseIsInPadding(e)) {
                    // reset to mouseover state,
                    checkChangeCursor(e);
                    win.onmousemove = checkChangeCursor;
                }

                if(x === e.clientX && y === e.clientY) {
                    // There is no displacement in the pointer between
                    // mouse down and mouse up but the mouse is still in
                    // play (in win padding area) for a resize if we
                    // get another mouse and than down.
                    return;
                }

                doResize(e);
                // Check and make sure that some part of the app window is
                // showing in the desktop.
                fixShowing();

                WDApp.activeTransitionState = WDApp.STATE_NONE;
            }

            document.addEventListener('mousemove', doResize);

            WDApp.root.showCatcher(cursors[cursorIndex], 'mouseup', mouseup);
        }


        win.onmousedown = mousedown;

        checkChangeCursor(e);

        win.onmousemove = checkChangeCursor;

        win.onmouseout = function(e) {

            if(e.buttons&01/*left mouse down BUG lies on firefox*/ ||
                WDApp.activeTransitionState !== WDApp.STATE_NONE) {
                // We are waiting for a mouse up, at which time we will
                // resize.
                return;
            }

            // Put back the "default" cursors:
            win.onmousemove = null;
            win.onmousedown = null;
            win.onmouseout = null;
        };
    };


    // mouse down callback for dragging by grabbing the header.
    header.onmousedown = function(e) {

        if(WDApp.activeTransitionState !== WDApp.STATE_NONE)
            return;
        if(!(e.buttons & 01))
            // Not the left button.
            return;

        WDApp.activeTransitionState = WDApp.STATE_DRAG;

        e = e || window.event;
        e.preventDefault();
        // get the mouse pointer position at startup:
        x0 = e.clientX;
        y0 = e.clientY;
        // As we drag the "window" past a right or bottom edge the
        // dimensions of the desktop can change to allow the "window" to
        // move.  So we need to get the desktop dimensions not before the
        //

        let style = getComputedStyle(win);
        let hStyle = getComputedStyle(header);
        startX = parseInt(style.marginLeft);
        startY = parseInt(style.marginTop);

        sx0 = scrollX;
        sy0 = scrollY;

        ///////////////////////////////////////
        // For the grab and move case:
        ///////////////////////////////////////

        WDApp.root.showCatcher('move', 'mouseup', finishDrag);

        document.onmouseup = finishDrag;
        // call a function whenever the cursor moves:
        document.onmousemove = drag;
        win.focus();
    }

    function drag(e) {

        e = e || window.event;
        e.preventDefault();

        if(displayState === MAXIMIZED) {
            normalX = x0 - normalWidth*(x0/header.clientWidth);
            normalY = y0 - header.clientHeight;
            setToNormalSize();
        }

        // calculate the new pointer position:
        var dx = e.clientX - x0;
        var dy = e.clientY - y0;
        x0 = e.clientX;
        y0 = e.clientY;


        // set the element's new position:
        win.style.left = (win.offsetLeft + dx - startX -
                (sx0 - scrollX)) + "px";
        win.style.top = (win.offsetTop + dy - startY -
                (sy0 - scrollY)) + "px";

        // TODO: if we move the win to the right past all
        // the current content, the window scrolls, and then
        // when we move the win back so the scroll goes away
        // the pointer falls off the win.

        sx0 = scrollX;
        sy0 = scrollY;
    }

    function fixShowing() {
        // Make it so that we can see some of the app window on the
        // desktop.
        if(win.offsetLeft - startX + win.clientWidth < xshow) {
            // fix left
            win.style.left = (xshow + winPaddingWidth -
                winPaddingLeft - win.clientWidth) + 'px';
            // reinitialize what is the normal position:
            normalX = win.offsetLeft;
        } else if(win.offsetLeft - startX > desktopWidth() - xshow) {
            // fix right
            win.style.left = (desktopWidth() -
                winPaddingLeft - xshow) + 'px';
            // reinitialize what is the normal position:
            normalX = win.offsetLeft;
        }

        if(win.offsetTop - startY < 0) {
            // fix upper
            win.style.top = '0px';
            // reinitialize what is the normal position:
            normalY = win.offsetTop;
        } else if(win.offsetTop - startY > desktopHeight() -
                header.offsetHeight) {
            // fix lower
            win.style.top = (desktopHeight() - header.offsetTop -
                header.clientHeight) + 'px';
            // reinitialize what is the normal position:
            normalY = win.offsetTop;
        }
    }


    const xshow = 18;

    function finishDrag() {

        WDApp.activeTransitionState = WDApp.STATE_NONE;

        WDApp.root.hideCatcher();
        
        // stop moving when the mouse button is released:
        document.onmouseup = null;
        document.onmousemove = null;

        // Check and make sure that some part of the app window is showing
        // in the desktop.
        fixShowing();

        saveNormalSize();
    }
}

// Used to keep app window stacking order:
WDApp.zIndexMax = 4;
WDApp.stackingOrder = { };

// These are private constants that we need.  We didn't want to pollute
// the global name space with a const; nor did we want the high resource
// usage of using yet another object.
WDApp.STATE_NONE = 0;
WDApp.STATE_RESIZE = 1; // the pointer is busy resizing a window app
WDApp.STATE_DRAG = 2;   // the pointer is busy dragging a window app

// We define "Active States" where the desktop is in the midst of using
// the mouse pointer to "continuously" change an "app window" parameter
// like app window size (resize), or app window position (drag).
WDApp.activeTransitionState = WDApp.STATE_NONE;

// The main root window object
WDApp.root = null;

// So we may find src for image icons files that are in the same directory
// as this javaScript file.  Calling an anonymous function.
WDApp.urlPrefix = (function() {

    let script = document.getElementsByTagName('script');
    script = script[script.length-1];
    // Now script is the last script loaded.  Strip off the query part
    // (which could have '/' in it) and the filename (/webDesktop.js) part.
    //
    return script.src.replace(/\?.*$/,'').replace(/\/[^\/]*$/,'');
})() + '/';
