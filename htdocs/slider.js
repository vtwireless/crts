require('/controllers.js');
require('/d3.v5.min.js'); // for d3.format(".2f") ... etc

// Make this:
//
// <div class=slider><label/><input type=range/><output/></div>
//
// or just put <label/><input type=range/><output/> in parentNode


function Slider(parameter, opts = {}) {

    function getOpt(x, defaultVal) {
        if(opts[x] !== undefined)
            return opts[x];
        return defaultVal;
    };

    var digits     = getOpt('digits',     3     );
    var prefix     = getOpt('prefix',     ''    );
    if(!prefix)
        prefix     = getOpt('label',      ''    );
    var step       = getOpt('step',       false );
    var scale      = getOpt('scale',      1.0   );
    var suffix     = getOpt('suffix',     ''    );
    var parentNode = getOpt('parentNode', null  );


    if(typeof(parentNode) === "string") {
        let str = parentNode;
        // We'll assume that this is a CSS Selector string
        // for a page with HTML already built.
        parentNode = document.querySelector(str);
        if(!parentNode)
            throw("Slider element parent \"" + str + '" was not found');
        if(parentNode.className === undefined)
            parentNode.className = 'slider';
    } else if(parentNode === null) {
        parentNode = document.createElement("div");
        parentNode.className = 'slider';
        document.body.appendChild(parentNode);
    } // Else
      // the user passed in a parentNode that we will try to use.


    let inputId = 'inputSliderId_xzz_' + Slider.count++;

    if(prefix !== '') {
        let label = document.createElement('label');
        label.setAttribute("for", inputId);
        label.innerHTML = prefix;
        label.className = 'slider';
        parentNode.appendChild(label);
    }

    var input = document.createElement("input");
    input.type = "range";
    input.className = 'slider';
    parentNode.appendChild(input);


    let output = document.createElement("output");
    output.className = 'slider';
    output.setAttribute("for", inputId);
    parentNode.append(output);


    function initParameter(par) {

        // It is amazing how complex this code is required to be in order
        // to avoid a nasty client/server positive feed-back loop.  One
        // could write this with half of this code, but than there would
        // be a glitch in the value displayed due to the finite
        // server/client interaction time.

        // isActive is true when the slider is being actively changed by
        // the user; at which time we do not display server values that
        // are feed to the client; if we did it would be a very bad
        // widget.
        let isActive = false;
        // haveParChangeValue is to store the last value to display while
        // the slider is/was active.
        let haveParChangeValue = false;

        let timeoutId = null;


        input.min = par.min;
        input.max = par.max;
        if(step === false)
            input.step = (input.max - input.min)/1000;
        else
            input.step = step;

        function Value(value) {
            return d3.format('.' + digits + 'f')(value * scale);
        }

        function displayGetValue(value) {
            value =  Value(value);
            input.value = value;
            output.value = value.toString() + suffix;
            if(haveParChangeValue !== false)
                haveParChangeValue = false;
        }

        input.addEventListener('pointerdown', function() {
            isActive = true;
            if(timeoutId !== null) {
                // The user quickly returned to using the slider, and so
                // we need to cancel the display update that would have
                // been coming in the very near future.  We let the user
                // see what the user is currently "trying" to do,
                // instead of what the server may be feeding this slider
                // display.
                clearTimeout(timeoutId);
                timeoutId = null;
            }
        });


        function restartDisplay() {
            // This call is from a setTimeout().
            timeoutId = null;
            if(haveParChangeValue === false)
                // There where no value changes to display.
                return;

            displayGetValue(haveParChangeValue);
        }


        input.addEventListener('pointerup', function() {
            // The user has just finished moving the slider.
            isActive = false;
            if(timeoutId !== null) {
                // If this code runs than the browser had a 'pointerup'
                // without a 'pointerdown' before it.  I do not think that
                // is possible, but I don't trust browsers.
                clearTimeout(timeoutId);
                timeoutId = null;
            }
            // So now we wait and get the "last value" to display;
            // otherwise there may be 2 or 3 values displayed because of
            // the server delay which makes the user very confused.
            timeoutId = setTimeout(restartDisplay, 1300/*milliseconds*/);
        });


        input.oninput = function() {
            // This will send a request through the web socket to change
            // the value via the parameter setter (=).
            //console.log("ONINPUT");
            let value = Value(parseFloat(input.value));
            par.set(value);
            output.value = value.toString() + suffix;
        };

        par.addOnChange(function(value) {

            if(isActive || timeoutId !== null) {
                // We do not let the server feedback affect the output
                // displayed while the input slider is being moved by the
                // user; so we save the last value to set it after the
                // user lets go of the slider.  Without this the slider
                // has a nasty delayed client/server feedback loop that
                // makes using the slider difficult.
                haveParChangeValue = value;
                return;
            }

            displayGetValue(value);
        });
    }

    // This will finish setting up this slider to act with the web socket
    // data as it changes a parameter and displays changes.  If the
    // parameter is never defined on the server, it is never set up, and
    // that's correct behavior.
    //
    // initParameter() is an event handler that is called if the parameter
    // with name "program" "filter" "parameter" comes into existence.
    //
    parameter.onSetup(initParameter);
}


Slider.count = 0;

