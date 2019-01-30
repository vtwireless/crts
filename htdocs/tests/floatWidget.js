// https://github.com/luser/gamepadtest
//

if(fail === undefined)
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


function createFloatWidget(startingValue,
    max, min, points, opts = { units: '' }) {


    /******************* parameters in this widget ******************/
    
    // These variables define the state of this widget.
    //
    // Some variables are derived and some are fundamental state (not
    // derived), as we define them.  We label the derived variables
    // with // derived.

    // TODO: remove these requirements:
    //
    assert(max > 1.0, 'max=' + max);
    assert(min > 0.0);
    assert(min < max);
    assert(points > -1);

    //
    // numDigits is the number of digits that are adjustable.
    let totalDigits = max.toFixed(points).length;

    if(points > 0)
        var numDigits = totalDigits - 1;
    else
        var numDigits = totalDigits;

    // The array of digit object abstraction
    var digits = []; // array of fundamental parameters

    // The selected index into the digits array:
    var selectedDigit = parseInt(numDigits/2); // fundamental parameter
    if(selectedDigit < 0) selectedDigit = 0;

    var textArray = []; // text nodes in the widget.

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

    let text = startingValue.toFixed(points);
    while(text.length < totalDigits)
        text = '0' + text;

    let d = 0;

    for(let i=0; i<totalDigits; ++i) {

        textArray[i] = document.createTextNode(text.charAt(i));

        if(text.charAt(i) === '.') {
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
    meter.value = ((getFloatValue() - min)/(max - min)).toFixed(4);
    meter.className = 'floatWidget';
    // BUG: or not.  The <meter> seems to work with some
    // values.
    meter.max = '1.0';
    meter.min = '0.0';
    //meter.innerHTML = 'foo';
    meter.high = '1.0';
    meter.low = '0.0';

    div.appendChild(meter);

    div.setAttribute("tabIndex", 0);

    div.onfocus = function() {
        console.log('Widget is in focus');
    };
    div.onblur = function() {
        console.log('Widget is out of focus');
    };

    function selectDigit(toI) {

        digits[selectedDigit].unselect();
        selectedDigit = toI;
        digits[toI].select();
    }

    // We only use this to set to max or min because
    // we need to avoid float rounding errors.
    function setValue(value) {

        assert(value <= max);
        assert(value >= min);

        meter.value = ((value - min)/(max - min)).toFixed(2);

        let strValue = value.toFixed(points);
        let len = strValue.length;
        if(points > 0)
            --len;
        let numLeadingZeros = numDigits-len;

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
    }

    function increaseDigit(i, first=true) {

        function checkValue() {
            var val = getFloatValue();
            if(val > max)
                setValue(max);
            else 
                meter.value = ((val - min)/(max - min)).toFixed(2);
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
            checkValue();

        //console.log('value = ' + getFloatValue());
    }

    function decreaseDigit(i, first=true) {

        function checkValue() {
            var val = getFloatValue();
            if(val < min)
                setValue(min);
            else
                meter.value = ((val - min)/(max - min)).toFixed(2);
        }

        if(digits[i].text.data !== '0') {
            digits[i].text.data = (parseInt(digits[i].text.data) - 1).toString();
        } else if(i > 0) {
            // textArray[i].data === '0'
            digits[i].text.data = '9';
            // recurse
            decreaseDigit(i-1);
        } else {
            // i === 0 and it's a '0' so go to the minimum value.
            setValue(min);
            return;
        }

        if(first)
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

    return div;
}
