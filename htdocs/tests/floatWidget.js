

function createFloatWidget(startingValue, opts = { units: '' }) {

    var div = document.createElement('div');
    div.className = 'floatWidget';

    // TODO: vary number of digits.
    //
    var text = startingValue.toString();
    var numDigits = text.length;
    var currentDigit = parseInt(numDigits/2);

    // TODO: set current selected digit.
    //
    for(let i=0; i<numDigits; ++i) {
        let span = document.createElement('span');
        span.appendChild(document.createTextNode(text.charAt(i)));
        if(i == currentDigit)
            span.className = 'selectedDigit';
        else
            span.className = 'digit';
        div.appendChild(span);
    }

    if(typeof(opts.units) === 'string') {
        let span = document.createElement('span');
        span.appendChild(document.createTextNode(' ' + opts.units));
        span.className = 'units';
        div.appendChild(span);
    }

    div.setAttribute("tabIndex", 1);

    div.onfocus = function() {
        console.log('Widget is in focus');
    };
    div.onblur = function() {
        console.log('Widget is out of focus');
    };


    return div;
}
