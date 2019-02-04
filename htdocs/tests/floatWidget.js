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

// createFloatWidget() objects may set and unset this
// as they focus and blur.
var _joystickEventCB = null;
// Count the number of float widgets created.
var _floatWidgetCount = 0;
// The ID of the current float widget that is in focus.
var _floatWidgetId = null;
// This gets set when a gamepad becomes available
// and a float widget is in focus.
var _runFrames = null;

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
        _runFrames = null;
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
    // we have a floatWidget with focus, i.e. _joystickEventCB is
    // set, and we have a gamepad available, i.e. a joystick is plugged
    // in.

    // We need this flag because _runFrames() needs to do nothing
    // if we are already running animation frames.
    var frameRequested = false;

    function runFrame(frameTime) {

        frameRequested = false;

        if(!gamepad || !_joystickEventCB)
            // no reason to run.
            return;

        _joystickEventCB(frameTime, gamepad.axes[0], -gamepad.axes[1]);

        // keep running in the next frame.
        requestAnimationFrame(runFrame);
        frameRequested = true;
    }

    _runFrames = function() {
        if(frameRequested) return;
        requestAnimationFrame(runFrame);
        frameRequested = true;
    };

    if(_joystickEventCB)
        _runFrames();
}



if('GamepadEvent' in window) {
    addEventListener("gamepadconnected", _addGamepad);
}



function createFloatWidget(startingValue,
    max, min, points, opts = { units: '' }) {


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

        assert(val <= max);
        assert(val >= min);

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
            var val = getFloatValue();
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
            var val = getFloatValue();
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
                increaseDigit(selectedDigit);
                break;
            case "ArrowDown":
                decreaseDigit(selectedDigit);
                break;
         }
    });


    // widget controller model parameters
    var firstFrame = true;
    var id = _floatWidgetCount++;
    var lastTime = 0.0;

    // Joystick dynamic variables:
    var digitChange = 0.0;
    var digitIncrease = 0.0;


    // selectedDigit is a discrete dynamical variable that is
    // controlled.  selectedDigit ranges from 0 to digits.length-1.  It's
    // an index to the digits array.
    //
    // selected digit value is another discrete dynamical variable.
    //

    // widget controller model state variables: x, y change with time
    // based on our widget controller model.


    function joystickEventCB(frameTime, xAxis, yAxis) {

        var dt = frameTime - lastTime;
        
        /*console.log('animation frame[widgetId=' + id + ']' +
            ' time=' + frameTime +  ' dt=' + dt +
            ' axes=' + xAxis + ',' + yAxis);*/




        lastTime = frameTime;
    }

    function initJoystickEventCB(frameTime, xAxis, yAxis) {

        // This is the first frame after we got focus.
        console.log('widget init animation frame[widgetId=' + id + ']' +
            ' time=' + frameTime +
            ' axes=' + xAxis + ',' + yAxis);

        // This is initialed, so go the real action now.
        _joystickEventCB = joystickEventCB;
        lastTime = frameTime;
    }

    div.onfocus = function() {
        // Grab the joystick callback:
        _joystickEventCB = initJoystickEventCB;
        _floatWidgetId = id;
        console.log('Widget ' + id + ' is in focus');
        // We reset the frameCount
        frameCount = 0;
        if(_runFrames)
            // We have a gamepad to read.
            _runFrames();
    };

    div.onblur = function() {
        console.log('Widget ' + id + ' is out of focus');
        if(_floatWidgetId === id) {
            _joystickEventCB = null;
            _floatWidgetId = null;
        }
    };


    return div;
}
