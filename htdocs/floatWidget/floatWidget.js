// Gamepad references:
//
// https://github.com/luser/gamepadtest
//
// https://w3c.github.io/gamepad/
//
// https://developer.mozilla.org/en-US/docs/Web/API/Gamepad_API/
//
//
// At the time of this code writing the gamepad API has only
// "gamepadconnected" and "gamepaddisconnected" events, and so we must
// poll to get the values from the "gamepad" joystick and buttons, and so
// there's a good chance we'll miss events.  This is a obvious deficiency
// in the Gamepad API.   Firefox may have a "gamepadchanged" and
// "gamepadaxischanged" or "gamepadaxismove" events soon (at the time I
// write this).
//

if(fail === undefined)
    function fail() {

        var text = "Something has gone wrong:\n";
        for(var i=0; i < arguments.length; ++i)
            text += "\n" + arguments[i];
        line = '\n--------------------------------------------------------';
        text += line + '\nCALL STACK' + line + '\n' +
            new Error().stack + line;
        console.log(text);
        alert(text);
        window.stop();
        throw text;
    }

if(assert === undefined)
    function assert(val, msg=null) {

        if(!val) {
            if(msg)
                fail(msg);
            else
                fail("JavaScript failed");
        }
    }




function _addGamepad(e) {
    // This is for adding joystick input.

    var gamepad = e.gamepad;

    console.log('Adding Gamepad[' +
            gamepad.index + ']: ' +
            gamepad.id);

    if(gamepad.axes === undefined || gamepad.axes.length < 2) {
        console.log('Gamepad[' +
            gamepad.index + ']: ' +
            gamepad.id + ' does not have 2 axes');
        gamepad = null;
        return;
    }


    addEventListener("gamepaddisconnected", function(e) {
    
        if(gamepad)
            console.log('Removing Gamepad[' +
                gamepad.index + ']: ' +
                gamepad.id);
        gamepad = null;
        createFloatWidget._runFrames = null;
        return;
    });



    /* Firefox may have this soon (not at the time of this writing):
    addEventListener("gamepadaxismove", function(e){
        // Axis move
        console.log(
            "Axes move",
            e.axis,
            e.value,
            e.gamepad
        );
    }); */

    // We do not bother keeping an Animation Frame callback unless
    // we have a floatWidget with focus, i.e.
    // createFloatWidget._joystickEventCB is set, and we have a gamepad
    // available, i.e. a joystick is plugged in.

    // We need this flag because createFloatWidget._runFrames() needs to do nothing
    // if we are already running animation frames.
    var frameRequested = false;

    function runFrame(frameTime) {

        frameRequested = false;

        if(!gamepad || !createFloatWidget._joystickEventCB)
            // no reason to run.
            return;

        let x = gamepad.axes[0], y = -gamepad.axes[1];

        // We must clip the values at a threshold or else we drift.
        if(Math.abs(x) < 0.3) x = 0.0;
        if(Math.abs(y) < 0.3) y = 0.0;

        if(x !== 0.0 || y !== 0.0)
            createFloatWidget._joystickEventCB(frameTime, x, y);

        // keep running in the next frame.
        requestAnimationFrame(runFrame);
        frameRequested = true;
    }

    createFloatWidget._runFrames = function() {
        if(frameRequested) return;
        requestAnimationFrame(runFrame);
        frameRequested = true;
    };

    if(createFloatWidget._joystickEventCB)
        createFloatWidget._runFrames();
}



if('GamepadEvent' in window) {
    addEventListener("gamepadconnected", _addGamepad);
}


// Set to true to add a div with text that spews X and Y axis values.
var _debugAxis = false;

/**
 *  @return a div
 *
 *
 *
 *
 *
 *
 *  @param points number of decimal points.  0 is like an int.
 *             -1 is like value 123 the 3 will not change.
 *             -2 is like value 123 the 2 will not change.
 *
 *
 *
 *
 *
 */

function createFloatWidget(startingValue,
    max, min, points, opts = {}) {

    let defaultOpts = { units: '', label: '', onchange: null};
    // setup user opts to defaults if not given
    Object.keys(defaultOpts).forEach(function(key) {
        if(opts[key] === undefined)
            opts[key] = defaultOpts[key];
    });


    /******************* parameters in this widget ******************/
    
    // Some of these variables define the state of this widget and some
    // are dummy variable that are used to calculate intermediate values.
    //
    // Some variables are derived and some are fundamental state (not
    // derived), as we define them.  We label the derived variables
    // with // derived.

    // TODO: remove these requirements:
    //
    assert(max > 1.0, 'max=' + max);
    assert(min > 0.0);
    assert(min < max);

    // totalDigits is all digits showing adjustable or not:
    //
    if(points > -1)
        var totalDigits = max.toFixed(points).length;
    else
        var totalDigits = max.toFixed(0).length;

    // numDigits is the number of digits that are adjustable.  We index an
    // array of digits that go from 0 to numDigits - 1.
    //
    if(points > 0)
        // We don't adjust the decimal point.
        var numDigits = totalDigits - 1;
    else
        // points can be negative in which case some non-fraction digits
        // are not adjustable
        var numDigits = totalDigits + points;

    // The array of digit object abstraction
    var digits = []; // array of fundamental parameters

    // The selected index into the digits array:
    var selectedDigit = parseInt(numDigits/2); // fundamental parameter
    if(selectedDigit < 0) selectedDigit = 0;

    //console.log("totalDigits=" + totalDigits + " numDigits=" + numDigits);

    // textArray is all digits and the decimal point that are showing
    // in the value as a decimal number.  Not all the characters in
    // this character array are adjustable, but they are all showing.
    // Like for example: [ '0', '3', '7', '.', '7', '3' ] for the float 
    // value 037.73 .  Note we need the leading zero so that we may
    // adjust that digit.
    //
    var textArray = [];

    /*****************************************************************/


    // div is the visual container:
    var div = document.createElement('div');
    div.className = 'floatWidget';

    if(typeof(opts.label) === 'string') {
        let span = document.createElement('span');
        span.appendChild(document.createTextNode(opts.label + ' '));
        span.className = 'label';
        div.appendChild(span);
    }

    let text = startingValue.toFixed(points>-1?points:0);
    while(text.length < totalDigits)
        text = '0' + text;

    let d = 0;

    for(let i=0; i<totalDigits; ++i) {

        textArray[i] = document.createTextNode(text.charAt(i));

        if(text.charAt(i) === '.' || d === numDigits) {
            let span = document.createElement('span');
            span.className = 'deadDigit';
            span.appendChild(document.createTextNode(text.charAt(i)));
            div.appendChild(span);
            continue;
        }

        let digit = digits[d] = {
            span: document.createElement('span'),
            select: function() {
                span.className = 'selectedDigit';
            },
            unselect: function() {
                span.className = 'digit';
            },
            text: textArray[i]
        };

        let span = digit.span;
        span.appendChild(digit.text);

        if(d == selectedDigit)
            digit.select();
        else
            digit.unselect();

        ++d;

        div.appendChild(span);
    }


    function getFloatValue() {

        var str = '';
        for(let i=0; i<totalDigits; ++i)
            str += textArray[i].data;
        return parseFloat(str);
    }


    if(typeof(opts.units) === 'string') {
        let span = document.createElement('span');
        span.appendChild(document.createTextNode(' ' + opts.units));
        span.className = 'units';
        div.appendChild(span);
    }

    var meter = document.createElement('meter');
    // TODO: How can we make getFloatValue() === startingValue
    // We can't control how the float is parsed and stored.
    meter.value = getFloatValue();
    meter.className = 'floatWidget';
    // BUG: or not.  The <meter> seems to work with some
    // values.
    meter.max = max;
    meter.min = min;
    //meter.innerHTML = 'foo';
    meter.high = max;
    meter.low = min;

    div.appendChild(meter);

    div.setAttribute("tabIndex", 0);


    function selectDigit(toI) {

        digits[selectedDigit].unselect();
        selectedDigit = toI;
        digits[toI].select();
    }

    function setValue(val) {

        assert(val <= max, "val=" + val + " > " + max + ' (=max)');
        assert(val >= min, "val=" + val + " < " + min + ' (=min)');

        meter.value = val;

        let strValue = val.toFixed(points>-1?points:0);
        let len = strValue.length;
        if(points > 0)
            --len;

        let numLeadingZeros = numDigits-len;
        if(points < 0)
            // There are digits that do not change that are before the
            // decimal point.  numDigits is the changeable digits.
            numLeadingZeros -= points;

        //console.log('len=' + len + ' numDigits =' + numDigits +
        //    ' numLeadingZeros = ' + numLeadingZeros);

        let i=0;
        let iStr=0;

        for(; i<numLeadingZeros;)
            digits[i++].text.data = '0';

        while(i<numDigits) {
            let ch = strValue.charAt(iStr++);
            if(ch === '.') ch = strValue.charAt(iStr++);
            digits[i++].text.data = ch;
        }

        //console.log("val=" + getFloatValue());
    }

    function increaseDigit(i, first=true) {

        function checkValue() {
            let val = getFloatValue();
            if(val > max)
                setValue(max);
            else 
                meter.value = val;
        }

        if(digits[i].text.data !== '9')
            digits[i].text.data = (parseInt(digits[i].text.data) + 1).toString();
        else if(i > 0) {
            // textArray[i].data === '9'
            digits[i].text.data = '0';
            // recurse
            increaseDigit(i-1, false);
        } else {
            // i === 0 and it's a '9' so go to the maximum value.
            setValue(max);
            return;
        }

        if(first)
            // This is the last recurse.
            checkValue();

        //console.log('value = ' + getFloatValue());
    }


    function decreaseDigit(i, first=true) {

        function checkValue() {
            let val = getFloatValue();
            if(val < min)
                setValue(min);
            else
                meter.value = val;
        }

        if(digits[i].text.data !== '0') {
            digits[i].text.data = (parseInt(digits[i].text.data) - 1).toString();
        } else if(i > 0) {
            // textArray[i].data === '0'
            digits[i].text.data = '9';
            // recurse
            decreaseDigit(i-1, false);
        } else {
            // i === 0 and it's a '0' so go to the minimum value.
            setValue(min);
            return;
        }

        if(first)
            // This is the last recurse.
            checkValue();

        //console.log('value = ' + getFloatValue());
    }

    // These three functions increase(), decrease(), and setValue(), are a
    // point at which all value changes happen.  If a value is "pegged" at
    // a max or min it will not change.

    function increase() {
        let value;
        if(opts.onchange)
            value = getFloatValue();
        increaseDigit(selectedDigit);
        if(value !== getFloatValue() && opts.onchange)
            opts.onchange(getFloatValue(), opts);
    }

    function decrease() {
        let value;
        if(opts.onchange)
            value = getFloatValue();
        decreaseDigit(selectedDigit);
        if(value !== getFloatValue() && opts.onchange)
            opts.onchange(getFloatValue(), opts);
    }

    assert(div.setValue === undefined, 'div.setValue is set');
    assert(div.getValue === undefined, 'div.getValue is set');


    div.setValue = function(val) {
        let value;
        if(val > max) val = max;
        else if(val < min) val = min;
        setValue(val);
    }


    div.getValue = function(val) {
        return getFloatValue();
    }


    div.addEventListener('keydown', function(e) {

        /*console.log('floatWidget got event keydown with (' +
            typeof(e.code) + ') code: "' +
            e.code + '" key="' + e.key + '"  float='+
            getFloatValue()); */

        switch(e.key) {

            case "ArrowLeft":
                if(selectedDigit > 0)
                    selectDigit(selectedDigit-1);
                break;
            case "ArrowRight":
                if(selectedDigit < digits.length-1)
                    selectDigit(selectedDigit+1);
                break;
            case "ArrowUp":
                increase();
                break;
            case "ArrowDown":
                decrease();
                break;
         }
    });


    var id =  createFloatWidget.floatWidgetCount++;
    var lastTime = 0.0;

    // Joystick dynamic variables:
    var x = 0; // digitChange
    var y = 0; // digitIncrease
    // Joystick  controller model parameters:
    var firstFrame = true;
    // These rates are the rates that the digits will change
    // when the joystick is at full input, like +1 or -1.
    var xRate = 0.003; // digits/millisecond
    var yRate = 0.015; // digits/millisecond


    // selectedDigit is a discrete dynamical variable that is
    // controlled.  selectedDigit ranges from 0 to digits.length-1.  It's
    // an index to the digits array.
    //
    // selected digit value is another discrete dynamical variable.
    //

    // widget controller model state variables: x, y change with time
    // based on our widget controller model.

    if(_debugAxis === true) {
        _debugAxis = document.createElement('div');
        document.body.appendChild(_debugAxis);
        _debugAxis.style.border = "2px solid black";
        _debugAxis.appendChild(_debugAxis = document.createTextNode('X, Y'));
    }

    function joystickEventCB(frameTime, xAxis, yAxis) {

        var dt = frameTime - lastTime;

        if(_debugAxis)
            console.log('animation frame[widgetId=' + id + ']' +
                ' time=' + frameTime +  ' dt=' + dt +
                ' axes=' + xAxis + ',' + yAxis);
        _debugAxis.data = ' axes=' + xAxis + ',' + yAxis;


        // We solve for the x(t) equations of motion.  Basically solving
        // via Euler's method.
        x += xRate * xAxis * dt;
        let xTick = Math.trunc(x);
        // We consume any discrete changes in x, be they positive or
        // negative, keeping any factional changes to keep smooth
        // motion:
        x -= xTick;
        while(xTick > 0) {
            if(selectedDigit < digits.length-1)
                selectDigit(selectedDigit+1);
            --xTick;
        }
        while(xTick < 0) {
            if(selectedDigit > 0)
                selectDigit(selectedDigit-1);
            ++xTick;
        }

        // We solve for the y(t) equations of motion.  Basically solving
        // via Euler's method (same as x).
        y += yRate * yAxis * dt;
        // We consume any discrete changes in y, be they positive or
        // negative, keeping any factional changes to keep smooth
        // motion:
        let yTick = Math.trunc(y);
        y -= yTick;
        while(yTick > 0) {
            increase();
            --yTick;
        }
        while(yTick < 0) {
            decrease();
            ++yTick;
        }

        // Keep time smooth:
        lastTime = frameTime;
    }

    function initJoystickEventCB(frameTime, xAxis, yAxis) {

        // This is the first frame after we got focus.
        console.log('floatWidget init animation frame[widgetId=' + id + ']' +
            ' time=' + frameTime +
            ' axes=' + xAxis + ',' + yAxis);

        // This is initialed, so go the real action now.
        createFloatWidget._joystickEventCB = joystickEventCB;
        lastTime = frameTime;
        digitChange = 0.0;
        digitIncrease = 0.0;
        x = 0.0;
        y = 0.0;
    }

    div.onfocus = function() {
        // Grab the joystick callback:
        createFloatWidget._joystickEventCB = initJoystickEventCB;
        createFloatWidget.floatWidgetId = id;
        console.log('floatWidget ' + id + ' is in focus');
        // We reset the frameCount
        frameCount = 0;
        if(createFloatWidget._runFrames)
            // We have a gamepad to read.
            createFloatWidget._runFrames();
    };

    div.onblur = function() {
        console.log('floatWidget ' + id + ' is out of focus');
        if(createFloatWidget.floatWidgetId === id) {
            createFloatWidget._joystickEventCB = null;
            createFloatWidget.floatWidgetId = null;
        }
    };

    return div;
}



// createFloatWidget() objects may set and unset this
// as they focus and blur.
createFloatWidget._joystickEventCB = null;

// Count the number of float widgets created.
createFloatWidget.floatWidgetCount = 0;

// The ID of the current float widget that is in focus.
createFloatWidget.floatWidgetId = null;

// This gets set when a gamepad becomes available
// and a float widget is in focus.
createFloatWidget._runFrames = null;
