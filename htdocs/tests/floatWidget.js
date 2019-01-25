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


function createFloatWidget(startingValue, max, min, points, opts = { units: '' }) {


    /******************* parameters in this widget ******************/
    
    // These variables define the state of this widget.
    //
    // Some variables are derived and some are fundamental state (not
    // derived), as we define them.  We label the derived variables
    // with // derived.

    let value = startingValue; // derived in future.

    // TODO: remove these requirements:
    //
    assert(max > 1.0, 'max=' + max);
    assert(min > 0.0);
    assert(min < max);
    assert(points > -1);

    //
    // numDigits is the number of digits that are adjustable.
    let numDigits = max.toFixed(points);

    if(points > 0) --numDigits;
    

    // The array of digit object abstraction
    var digits = []; // array of fundamental parameters

    // The selected index into the digits array:
    var selectedDigit = parseInt(numDigits/2) - 1; // fundamental parameter
    if(selectedDigit < 0) selectedDigit = 0;

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

    let text = value.toString();
    let d = 0;

    for(let i=0; i<text.length; ++i) {

        if(text.charAt(i) === '.' || d >= numDigits) {
            span = document.createElement('span');
            span.className = 'deadDigit';
            span.appendChild(document.createTextNode(text.charAt(i));
        }

        let digit = digits[d++] = {
            span: document.createElement('span'),
            select: function() {
                span.className = 'selectedDigit';
            },
            unselect: function() {
                span.className = 'digit';
            },
            text: document.createTextNode(text.charAt(i))
        };
        let span = digit.span;
        span.appendChild(digit.text);

        if(i == selectedDigit)
            digit.select();
        else
            digit.unselect();

        div.appendChild(span);
    }

    if(typeof(opts.units) === 'string') {
        let span = document.createElement('span');
        span.appendChild(document.createTextNode(' ' + opts.units));
        span.className = 'units';
        div.appendChild(span);
    }

    div.setAttribute("tabIndex", 0);

    div.onfocus = function() {
        console.log('Widget is in focus');
    };
    div.onblur = function() {
        console.log('Widget is out of focus');
    };

    function selectDigit(fromI, toI) {

        digits[fromI].unselect();
        digits[toI].select();
        selectedDigit = toI;
    }

    div.addEventListener('keydown', function(e) {

        console.log('floatWidget got event keydown with (' +
            typeof(e.code) + ') code: "' +
            e.code + '" key="' + e.key + '"');

        switch(e.key) {

            case "ArrowLeft":
                if(selectedDigit > 0)
                    selectDigit(selectedDigit, selectedDigit-1);
                break;

            case "ArrowRight":
                if(selectedDigit < digits.length-1)
                    selectDigit(selectedDigit, selectedDigit+1);
                break;


        }
    });

    return div;
}
